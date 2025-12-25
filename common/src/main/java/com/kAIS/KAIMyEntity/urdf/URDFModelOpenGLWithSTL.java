package com.kAIS.KAIMyEntity.urdf;

import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.blaze3d.vertex.VertexConsumer;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.RenderType;
import net.minecraft.world.entity.Entity;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joml.Matrix3f;
import org.joml.Matrix3fc;
import org.joml.Matrix4f;
import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.io.File;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * URDF 모델 렌더링 (STL 메시 포함)
 *
 * - 루트에서만 업라이트 보정(ROS/STL 좌표 → Minecraft) 1회 적용
 *   · Up을 먼저 정확히 맞추고 → Up에 수직인 평면에서 Forward만 정렬 (롤 꼬임 방지)
 * - 링크/조인트 원점/축은 원본 좌표 기준 (추가 보정 없음)
 *
 * + (추가) Tracking(VMC) Space -> URDF Base Space 변환 모듈
 *   · VMC(보통 Unity 좌표계)에서 온 rotation/position을 URDF 기준으로 변환하는 기저(Basis)를
 *     URDFModelOpenGLWithSTL 안에서 명시적으로 관리
 */
public class URDFModelOpenGLWithSTL implements IMMDModel {
    private static final Logger logger = LogManager.getLogger();
    private static int renderCount = 0;

    private URDFModel robotModel;
    private String modelDir;

    // 메시 캐시: link.name -> mesh
    private final Map<String, STLLoader.STLMesh> meshCache = new HashMap<>();

    // 전역 스케일
    private static final float GLOBAL_SCALE = 5.0f;

    // 법선 반전(필요 시)
    private static final boolean FLIP_NORMALS = true;

    // ------------ 업라이트 보정 설정 (URDF/STL -> Minecraft, "고정 회전") ------------
    /** URDF/STL 소스 좌표계 가정 */
    private static final Vector3f SRC_UP  = new Vector3f(0, 0, 1); // 보통 Z-up (ROS)
    private static final Vector3f SRC_FWD = new Vector3f(1, 0, 0); // 보통 +X forward (ROS)

    /** Minecraft 타깃 좌표계 (Up=+Y, Forward=±Z) */
    private static final boolean FORWARD_NEG_Z = true;
    private static final Vector3f DST_UP  = new Vector3f(0, 1, 0);
    private static final Vector3f DST_FWD = FORWARD_NEG_Z ? new Vector3f(0, 0, -1)
            : new Vector3f(0, 0,  1);

    /** 루트에서 1회만 적용하는 업라이트 보정 */
    private static final Quaternionf Q_ROS2MC = makeUprightQuat(SRC_UP, SRC_FWD, DST_UP, DST_FWD);

    // ============================
    // Tracking Space -> URDF Base Space 변환 ("입력 변환")
    // ============================

    /**
     * 트래킹 좌표계(VMC)에서 들어온 포즈를 URDF base 좌표계로 바꾸는 기저변환(3x3).
     * - 기본: Identity (변환 없음)
     * - 반사(미러, det=-1)도 가능 (좌/우 반전 문제를 여기서 해결 가능)
     */
    private final Matrix3f M_TRACKING_TO_URDF = new Matrix3f().identity();
    private final Matrix3f M_URDF_TO_TRACKING = new Matrix3f().identity();

    /** position 스케일 보정(예: meter 기반이면 1.0 그대로) */
    private float trackingPosScale = 1.0f;

    public URDFModelOpenGLWithSTL(URDFModel robotModel, String modelDir) {
        this.robotModel = robotModel;
        this.modelDir = modelDir;
        logger.info("=== URDF renderer Created ===");
        loadAllMeshes();

        // (선택) 기본 프리셋: VMC/Unity -> ROS(URDF base) 변환을 원하면 켜두면 편함
        // setTrackingBasisPreset_VMC_UnityToROS();
    }

    private void loadAllMeshes() {
        logger.info("=== Loading STL meshes ===");
        int loadedCount = 0;
        for (URDFLink link : robotModel.links) {
            if (link.visual != null && link.visual.geometry != null) {
                URDFLink.Geometry g = link.visual.geometry;
                if (g.type == URDFLink.Geometry.GeometryType.MESH && g.meshFilename != null) {
                    File f = new File(g.meshFilename);
                    if (f.exists()) {
                        STLLoader.STLMesh mesh = STLLoader.load(g.meshFilename);
                        if (mesh != null) {
                            if (g.scale != null && (g.scale.x != 1f || g.scale.y != 1f || g.scale.z != 1f)) {
                                STLLoader.scaleMesh(mesh, g.scale);
                            }
                            meshCache.put(link.name, mesh);
                            loadedCount++;
                            logger.info("  ✓ Loaded mesh for '{}': {} tris", link.name, mesh.getTriangleCount());
                        } else {
                            logger.error("  ✗ Failed to load mesh: {}", g.meshFilename);
                        }
                    } else {
                        logger.warn("  ✗ Mesh file not found: {}", g.meshFilename);
                    }
                }
            }
        }
        logger.info("=== STL Loading Complete: {}/{} meshes ===", loadedCount, robotModel.getLinkCount());
    }

    // ===== 틱 업데이트 (20Hz 권장) =====
    public void tickUpdate(float dt) {
        // 현재 버전에서는 컨트롤러 로직이 외부(PosePipeline/MotionEditorScreen)로 이동되어 빈 메서드일 수 있음
    }

    // ===== 외부 제어용 =====
    public void setJointTarget(String name, float value) { /* 제어 로직 제거됨 */ }
    public void setJointTargets(Map<String, Float> values) { /* 제어 로직 제거됨 */ }

    /** 즉시 반영(프리뷰): 현재 프레임에서 바로 보이게 currentPosition을 덮어씀 */
    public void setJointPreview(String name, float value) {
        URDFJoint j = getJointByName(name);
        if (j != null) j.currentPosition = value;
    }

    // ============================
    // Tracking(VMC) -> URDF 변환 API
    // ============================

    /**
     * 트래킹->URDF base 기저 설정.
     * R' = B * R * B^-1, p' = B * p * scale
     */
    public synchronized void setTrackingToUrdfBasis(Matrix3fc basisTrackingToUrdf) {
        if (basisTrackingToUrdf == null) {
            M_TRACKING_TO_URDF.identity();
            M_URDF_TO_TRACKING.identity();
            return;
        }
        M_TRACKING_TO_URDF.set(basisTrackingToUrdf);
        M_URDF_TO_TRACKING.set(basisTrackingToUrdf).invert();
    }

    public synchronized void setTrackingPositionScale(float scale) {
        this.trackingPosScale = scale;
    }

    /**
     * VMC(VirtualMotionCapture=Unity 좌표계)에서 자주 쓰는 기본 프리셋.
     *
     * Unity(VMC) 축: X=Right, Y=Up, Z=Forward
     * ROS/URDF base 축(관례): X=Forward, Y=Left, Z=Up
     *
     * 따라서:
     *   x_ros =  z_unity
     *   y_ros = -x_unity
     *   z_ros =  y_unity
     */
    public void setTrackingBasisPreset_VMC_UnityToROS() {
        Matrix3f b = new Matrix3f();
        // setRow(row, x, y, z)
        b.setRow(0, 0f, 0f,  1f); // x_ros
        b.setRow(1, -1f, 0f, 0f); // y_ros
        b.setRow(2, 0f,  1f, 0f); // z_ros
        setTrackingToUrdfBasis(b);
    }

    /**
     * tracking rotation -> URDF rotation
     * R' = B * R * B^-1
     */
    public Quaternionf trackingRotToUrdf(Quaternionf trackingRot, Quaternionf out) {
        if (out == null) out = new Quaternionf();
        if (trackingRot == null) return out.identity();

        Matrix3f B, Binv;
        float _scale;
        synchronized (this) {
            B = new Matrix3f(M_TRACKING_TO_URDF);
            Binv = new Matrix3f(M_URDF_TO_TRACKING);
            _scale = trackingPosScale; // (unused here) keep for symmetry
        }

        Matrix3f R = new Matrix3f().set(trackingRot);
        Matrix3f R2 = B.mul(R, new Matrix3f()).mul(Binv);

        out.setFromUnnormalized(R2).normalize();
        return out;
    }

    /**
     * tracking position -> URDF position
     * p' = B * p * scale
     */
    public Vector3f trackingPosToUrdf(Vector3f trackingPos, Vector3f out) {
        if (out == null) out = new Vector3f();
        if (trackingPos == null) return out.zero();

        Matrix3f B;
        float s;
        synchronized (this) {
            B = new Matrix3f(M_TRACKING_TO_URDF);
            s = trackingPosScale;
        }

        out.set(trackingPos);
        B.transform(out);
        out.mul(s);
        return out;
    }

    // ============================
    // Joint dump API (요청사항)
    // ============================

    /**
     * 현재 관절 상태를 라디안 기준으로 뽑아옵니다.
     * - REVOLUTE / CONTINUOUS만 포함 (rad)
     * - PRISMATIC 제외 (meter 단위라 rad 아님)
     */
    public Map<String, Float> getJointPositionsRad() {
        if (robotModel == null || robotModel.joints == null) return Collections.emptyMap();
        Map<String, Float> out = new LinkedHashMap<>(robotModel.joints.size());
        fillJointPositionsRad(out);
        return out;
    }

    /** 재사용 가능한 out 맵에 라디안 관절만 채움(할당 최소화) */
    public void fillJointPositionsRad(Map<String, Float> out) {
        if (out == null) return;
        out.clear();
        if (robotModel == null || robotModel.joints == null) return;

        for (URDFJoint j : robotModel.joints) {
            if (j == null || j.name == null) continue;
            switch (j.type) {
                case REVOLUTE:
                case CONTINUOUS:
                    out.put(j.name, j.currentPosition);
                    break;
                default:
                    break;
            }
        }
    }

    /** (선택) movable 전부 포함: revolute(rad) + prismatic(m) */
    public Map<String, Float> getJointPositionsAll() {
        if (robotModel == null || robotModel.joints == null) return Collections.emptyMap();
        Map<String, Float> out = new LinkedHashMap<>(robotModel.joints.size());
        for (URDFJoint j : robotModel.joints) {
            if (j == null || j.name == null) continue;
            if (j.isMovable()) out.put(j.name, j.currentPosition);
        }
        return out;
    }

    // ============================
    // Render
    // ============================

    @Override
    public void Render(Entity entityIn, float entityYaw, float entityPitch,
                       Vector3f entityTrans, float tickDelta, PoseStack poseStack, int packedLight) {

        renderCount++;
        if (renderCount % 120 == 1) {
            logger.info("=== URDF RENDER #{} ===", renderCount);
        }

        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.enableDepthTest();
        RenderSystem.disableCull(); // 양면

        MultiBufferSource.BufferSource bufferSource = Minecraft.getInstance().renderBuffers().bufferSource();
        VertexConsumer vc = bufferSource.getBuffer(RenderType.solid());

        if (robotModel.rootLinkName != null) {
            poseStack.pushPose();

            // 전역 스케일
            poseStack.scale(GLOBAL_SCALE, GLOBAL_SCALE, GLOBAL_SCALE);

            // ★ 루트에만 업라이트 보정 적용 (고정 회전)
            poseStack.mulPose(new Quaternionf(Q_ROS2MC));

            renderLinkRecursive(robotModel.rootLinkName, poseStack, vc, packedLight);

            poseStack.popPose();
        }

        bufferSource.endBatch(RenderType.solid());
        RenderSystem.enableCull();
    }

    private void renderLinkRecursive(String linkName, PoseStack poseStack, VertexConsumer vc, int packedLight) {
        URDFLink link = robotModel.getLink(linkName);
        if (link == null) return;

        poseStack.pushPose();

        if (link.visual != null) {
            renderVisual(link, poseStack, vc, packedLight);
        }

        for (URDFJoint childJoint : robotModel.getChildJoints(linkName)) {
            poseStack.pushPose();
            applyJointTransform(childJoint, poseStack);
            renderLinkRecursive(childJoint.childLinkName, poseStack, vc, packedLight);
            poseStack.popPose();
        }

        poseStack.popPose();
    }

    private void renderVisual(URDFLink link, PoseStack poseStack, VertexConsumer vc, int packedLight) {
        if (link.visual == null || link.visual.geometry == null) return;

        poseStack.pushPose();

        if (link.visual.origin != null) {
            applyLinkOriginTransform(link.visual.origin, poseStack);
        }

        STLLoader.STLMesh mesh = meshCache.get(link.name);
        if (mesh != null) {
            renderMesh(mesh, link, poseStack, vc, packedLight);
        }
        poseStack.popPose();
    }

    private void renderMesh(STLLoader.STLMesh mesh, URDFLink link, PoseStack poseStack,
                            VertexConsumer vc, int packedLight) {
        Matrix4f matrix = poseStack.last().pose();

        int r = 220, g = 220, b = 220, a = 255;
        if (link.visual.material != null && link.visual.material.color != null) {
            URDFLink.Material.Vector4f color = link.visual.material.color;
            r = (int)(color.x * 255);
            g = (int)(color.y * 255);
            b = (int)(color.z * 255);
            a = (int)(color.w * 255);
        }

        int blockLight = (packedLight & 0xFFFF);
        int skyLight   = (packedLight >> 16) & 0xFFFF;
        blockLight = Math.max(blockLight, 0xA0);
        skyLight  = Math.max(skyLight,  0xA0);

        for (STLLoader.Triangle tri : mesh.triangles) {
            for (int i = 2; i >= 0; i--) {
                Vector3f v = tri.vertices[i];
                Vector3f n = tri.normal;

                float nx = FLIP_NORMALS ? -n.x : n.x;
                float ny = FLIP_NORMALS ? -n.y : n.y;
                float nz = FLIP_NORMALS ? -n.z : n.z;

                vc.addVertex(matrix, v.x, v.y, v.z)
                        .setColor(r, g, b, a)
                        .setUv(0.5f, 0.5f)
                        .setUv2(blockLight, skyLight)
                        .setNormal(nx, ny, nz);
            }
        }
    }

    // ============================
    // Transform helpers
    // ============================

    private void applyLinkOriginTransform(URDFLink.Origin origin, PoseStack poseStack) {
        poseStack.translate(origin.xyz.x, origin.xyz.y, origin.xyz.z);
        if (origin.rpy.x != 0f || origin.rpy.y != 0f || origin.rpy.z != 0f) {
            poseStack.mulPose(origin.getQuaternion());
        }
    }

    private void applyJointOriginTransform(URDFJoint.Origin origin, PoseStack poseStack) {
        poseStack.translate(origin.xyz.x, origin.xyz.y, origin.xyz.z);
        if (origin.rpy.x != 0f || origin.rpy.y != 0f || origin.rpy.z != 0f) {
            Quaternionf qx = new Quaternionf().rotateX(origin.rpy.x);
            Quaternionf qy = new Quaternionf().rotateY(origin.rpy.y);
            Quaternionf qz = new Quaternionf().rotateZ(origin.rpy.z);
            poseStack.mulPose(qz.mul(qy).mul(qx));
        }
    }

    private void applyJointTransform(URDFJoint joint, PoseStack poseStack) {
        if (joint.origin != null) {
            applyJointOriginTransform(joint.origin, poseStack);
        }
        if (joint.isMovable()) {
            applyJointMotion(joint, poseStack);
        }
    }

    /** 축 기본값/정규화 포함(비었거나 0-벡터면 X축) */
    private void applyJointMotion(URDFJoint joint, PoseStack poseStack) {
        if (joint == null) return;

        switch (joint.type) {
            case REVOLUTE:
            case CONTINUOUS: {
                Vector3f axis;
                if (joint.axis == null || joint.axis.xyz == null ||
                        joint.axis.xyz.lengthSquared() < 1e-12f) {
                    axis = new Vector3f(1, 0, 0);
                } else {
                    axis = new Vector3f(joint.axis.xyz);
                    if (axis.lengthSquared() < 1e-12f) axis.set(1, 0, 0);
                    else axis.normalize();
                }
                Quaternionf quat = new Quaternionf().rotateAxis(joint.currentPosition, axis.x, axis.y, axis.z);
                poseStack.mulPose(quat);
                break;
            }
            case PRISMATIC: {
                Vector3f axis;
                if (joint.axis == null || joint.axis.xyz == null ||
                        joint.axis.xyz.lengthSquared() < 1e-12f) {
                    axis = new Vector3f(1, 0, 0);
                } else {
                    axis = new Vector3f(joint.axis.xyz);
                    if (axis.lengthSquared() < 1e-12f) axis.set(1, 0, 0);
                    else axis.normalize();
                }
                Vector3f t = axis.mul(joint.currentPosition);
                poseStack.translate(t.x, t.y, t.z);
                break;
            }
            default:
                break;
        }
    }

    // ============================
    // IMMDModel
    // ============================

    @Override public void ChangeAnim(long anim, long layer) { }
    @Override public void ResetPhysics() { logger.info("ResetPhysics called"); }
    @Override public long GetModelLong() { return 0; }
    @Override public String GetModelDir() { return modelDir; }

    public static URDFModelOpenGLWithSTL Create(String urdfPath, String modelDir) {
        File urdfFile = new File(urdfPath);
        if (!urdfFile.exists()) return null;
        URDFModel robot = URDFParser.parse(urdfFile);
        if (robot == null || robot.rootLinkName == null) return null;
        return new URDFModelOpenGLWithSTL(robot, modelDir);
    }

    public URDFModel getRobotModel() {
        return robotModel;
    }

    // ============================
    // internal
    // ============================

    private URDFJoint getJointByName(String name) {
        if (name == null) return null;
        for (URDFJoint j : robotModel.joints) {
            if (name.equals(j.name)) return j;
        }
        return null;
    }

    // ============================
    // Upright utilities
    // ============================

    /** Up을 먼저 맞추고 → Up에 수직인 평면에서 Forward만 정렬 (롤 꼬임 방지) */
    private static Quaternionf makeUprightQuat(Vector3f srcUp, Vector3f srcFwd,
                                               Vector3f dstUp, Vector3f dstFwd) {
        Vector3f su = new Vector3f(srcUp).normalize();
        Vector3f sf = new Vector3f(srcFwd).normalize();
        Vector3f du = new Vector3f(dstUp).normalize();
        Vector3f df = new Vector3f(dstFwd).normalize();

        // 1) Up 정렬
        Quaternionf qUp = fromToQuat(su, du);
        Vector3f sf1 = sf.rotate(new Quaternionf(qUp));

        // 2) Up에 수직인 평면에서 Forward 정렬
        Vector3f sf1p = new Vector3f(sf1).sub(new Vector3f(du).mul(sf1.dot(du)));
        Vector3f dfp  = new Vector3f(df ).sub(new Vector3f(du).mul(df .dot(du)));
        if (sf1p.lengthSquared() < 1e-10f || dfp.lengthSquared() < 1e-10f) {
            return qUp.normalize();
        }
        sf1p.normalize();
        dfp.normalize();

        float cos = clamp(sf1p.dot(dfp), -1f, 1f);
        float angle = (float)Math.acos(cos);
        Vector3f cross = sf1p.cross(dfp, new Vector3f());
        if (cross.dot(du) < 0) angle = -angle;

        Quaternionf qFwd = new Quaternionf().fromAxisAngleRad(du, angle);
        return qFwd.mul(qUp).normalize();
    }

    private static Quaternionf fromToQuat(Vector3f a, Vector3f b) {
        Vector3f v1 = new Vector3f(a).normalize();
        Vector3f v2 = new Vector3f(b).normalize();
        float dot = clamp(v1.dot(v2), -1f, 1f);

        if (dot > 1.0f - 1e-6f) return new Quaternionf();
        if (dot < -1.0f + 1e-6f) {
            Vector3f axis = pickAnyPerp(v1).normalize();
            return new Quaternionf().fromAxisAngleRad(axis, (float)Math.PI);
        }
        Vector3f axis = v1.cross(v2, new Vector3f()).normalize();
        float angle = (float)Math.acos(dot);
        return new Quaternionf().fromAxisAngleRad(axis, angle);
    }

    private static Vector3f pickAnyPerp(Vector3f v) {
        Vector3f x = new Vector3f(1,0,0), y = new Vector3f(0,1,0), z = new Vector3f(0,0,1);
        float dx = Math.abs(v.dot(x)), dy = Math.abs(v.dot(y)), dz = Math.abs(v.dot(z));
        return (dx < dy && dx < dz) ? x : ((dy < dz) ? y : z);
    }

    private static float clamp(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
}