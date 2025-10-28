package com.kAIS.KAIMyEntity.urdf;

import com.kAIS.KAIMyEntity.urdf.control.URDFMotionEditor;
import com.kAIS.KAIMyEntity.urdf.control.URDFMotionPlayer;
import com.kAIS.KAIMyEntity.urdf.control.URDFSimpleController;
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
import org.joml.Matrix4f;
import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * URDF 모델 렌더링 (STL 메시 포함)
 *
 * - 루트에서만 ROS(Z-up, +X forward) → Minecraft(Y-up, ±Z forward) 보정
 *   * rotateX(-90°) 로 "눕는 현상" 해결
 *   * rotateY(±90°) 로 전진축 정렬
 *   * (옵션) rotateZ(180°) 로 거꾸로 나올 때 뒤집기
 *
 * - 링크/조인트 원점과 축은 "원본 좌표계 그대로" 사용 (추가 보정 없음)
 * - setJointPreview(...) 즉시 반영 + tickUpdate(...)에서 컨트롤러 추종
 */
public class URDFModelOpenGLWithSTL implements IMMDModel {
    private static final Logger logger = LogManager.getLogger();
    private static int renderCount = 0;

    private URDFRobotModel robotModel;
    private String modelDir;

    // 메시 캐시: link.name -> mesh
    private final Map<String, STLLoader.STLMesh> meshCache = new HashMap<>();

    // 전역 스케일
    private static final float GLOBAL_SCALE = 5.0f;

    // 법선 반전(필요 시)
    private static final boolean FLIP_NORMALS = true;

    // === 방향/보정 스위치 ===
    /** 마인크래프트 정면을 -Z 로 둘지(+Z 로 반전할지) */
    private static final boolean FORWARD_NEG_Z = true;   // 기본: -Z 정면
    /** 모델이 거꾸로 나오면 true로: Z축으로 180° 추가 롤 */
    private static final boolean ROLL_180_Z = false;

    // 각도 상수
    private static final float HALF_PI = (float)(Math.PI / 2.0);
    private static final float PI      = (float)(Math.PI);

    /** 루트에서 1회만 적용하는 ROS→MC 업라이트 보정 */
    private static final Quaternionf Q_ROS2MC =
        new Quaternionf()
            .rotateX(-HALF_PI)                                   // Z-up → Y-up (눕는 문제 해결 핵심)
            .rotateY(FORWARD_NEG_Z ? +HALF_PI : -HALF_PI)        // +X → -Z(기본) / +Z(반대)
            .rotateZ(ROLL_180_Z ? PI : 0f);                      // (옵션) 뒤집힘 보정

    // ====== 모션/컨트롤 ======
    private final URDFSimpleController ctrl;
    private final URDFMotionEditor motionEditor;
    private final URDFMotionPlayer motionPlayer = new URDFMotionPlayer();

    public URDFModelOpenGLWithSTL(URDFRobotModel robotModel, String modelDir) {
        this.robotModel = robotModel;
        this.modelDir = modelDir;
        logger.info("=== URDF renderer Created ===");
        loadAllMeshes();

        // 컨트롤/모션 초기화
        this.ctrl = new URDFSimpleController(robotModel.joints);
        this.motionEditor = new URDFMotionEditor(robotModel, ctrl);
    }

    private void loadAllMeshes() {
        logger.info("=== Loading STL meshes ===");
        int loadedCount = 0;
        for (URDFLink link : robotModel.links) {
            if (link.visual != null && link.visual.geometry != null) {
                URDFLink.Geometry geom = link.visual.geometry;
                if (geom.type == URDFLink.Geometry.GeometryType.MESH && geom.meshFilename != null) {
                    File meshFile = new File(geom.meshFilename);
                    if (meshFile.exists()) {
                        STLLoader.STLMesh mesh = STLLoader.load(geom.meshFilename);
                        if (mesh != null) {
                            if (geom.scale != null &&
                                (geom.scale.x != 1f || geom.scale.y != 1f || geom.scale.z != 1f)) {
                                STLLoader.scaleMesh(mesh, geom.scale);
                            }
                            meshCache.put(link.name, mesh);
                            loadedCount++;
                            logger.info("  ✓ Loaded mesh for '{}': {} tris", link.name, mesh.getTriangleCount());
                        } else {
                            logger.error("  ✗ Failed to load mesh: {}", geom.meshFilename);
                        }
                    } else {
                        logger.warn("  ✗ Mesh file not found: {}", geom.meshFilename);
                    }
                }
            }
        }
        logger.info("=== STL Loading Complete: {}/{} meshes ===", loadedCount, robotModel.getLinkCount());
    }

    // ===== 틱 업데이트 (20Hz 권장) =====
    public void tickUpdate(float dt) {
        if (motionPlayer.isPlaying()) {
            motionPlayer.update(dt, this::setJointTarget);
        }
        ctrl.update(dt);
    }

    // ===== 외부 제어용 편의 API =====
    public void setJointTarget(String name, float value) { ctrl.setTarget(name, value); }
    public void setJointTargets(Map<String, Float> values) { ctrl.setTargets(values); }

    /** 즉시 반영(프리뷰): 현재 프레임에서 바로 보이게 currentPosition을 덮어씀 */
    public void setJointPreview(String name, float value) {
        URDFJoint j = getJointByName(name);
        if (j != null) {
            j.currentPosition = value;   // 화면 즉시 반영
            // j.currentVelocity = 0f;
        }
    }

    public URDFMotionEditor getMotionEditor() { return motionEditor; }
    public URDFMotionPlayer getMotionPlayer() { return motionPlayer; }

    @Override
    public void Render(Entity entityIn, float entityYaw, float entityPitch,
                       Vector3f entityTrans, float tickDelta, PoseStack poseStack, int packedLight) {

        renderCount++;
        if (renderCount % 120 == 1) {
            logger.info("=== URDF RENDER #{} ===", renderCount);
        }

        // 전역 렌더 상태
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.enableDepthTest();
        RenderSystem.disableCull(); // 양면

        // 버퍼
        MultiBufferSource.BufferSource bufferSource = Minecraft.getInstance().renderBuffers().bufferSource();
        VertexConsumer vc = bufferSource.getBuffer(RenderType.solid());

        if (robotModel.rootLinkName != null) {
            poseStack.pushPose();

            // 전역 스케일
            poseStack.scale(GLOBAL_SCALE, GLOBAL_SCALE, GLOBAL_SCALE);

            // ★ 루트에만 업라이트 보정 적용 — "눕는 현상"은 여기서 해결
            poseStack.mulPose(new Quaternionf(Q_ROS2MC));

            // 루트부터 렌더
            renderLinkRecursive(robotModel.rootLinkName, poseStack, vc, packedLight);

            poseStack.popPose();
        }

        // 필요 시 충돌하면 임시로 주석 처리
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

    // ===== 변환 유틸 =====
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
                    axis = new Vector3f(1, 0, 0); // 기본축 X
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
                    axis = new Vector3f(1, 0, 0); // 기본축 X
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

    // ===== IMMDModel 구현 =====
    @Override public void ChangeAnim(long anim, long layer) { }
    @Override public void ResetPhysics() { logger.info("ResetPhysics called"); }
    @Override public long GetModelLong() { return 0; }
    @Override public String GetModelDir() { return modelDir; }

    public static URDFModelOpenGLWithSTL Create(String urdfPath, String modelDir) {
        File urdfFile = new File(urdfPath);
        if (!urdfFile.exists()) return null;
        URDFRobotModel robot = URDFParser.parse(urdfFile);
        if (robot == null || robot.rootLinkName == null) return null;
        return new URDFModelOpenGLWithSTL(robot, modelDir);
    }

    public URDFRobotModel getRobotModel() {
        return robotModel;
    }

    // ===== 내부 유틸 =====
    private URDFJoint getJointByName(String name) {
        if (name == null) return null;
        for (URDFJoint j : robotModel.joints) {
            if (name.equals(j.name)) return j;
        }
        return null;
    }
}
