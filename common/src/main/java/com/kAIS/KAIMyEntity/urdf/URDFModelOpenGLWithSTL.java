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
import org.joml.Matrix4f;
import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * URDF 모델 렌더링 (STL 메시 포함)
 */
public class URDFModelOpenGLWithSTL implements IMMDModel {
    private static final Logger logger = LogManager.getLogger();
    private static int renderCount = 0;

    private URDFRobotModel robotModel;
    private String modelDir;
    
    // STL 메시 캐시 (링크 이름 -> 메시)
    private Map<String, STLLoader.STLMesh> meshCache;
    
    // 전역 스케일 (URDF는 미터 단위, 마인크래프트는 블록 단위)
    private static final float GLOBAL_SCALE = 5.0f;  // 5배 확대
    
    // 법선 벡터 반전 (STL이 뒤집혀 있으면 true)
    private static final boolean FLIP_NORMALS = true;

    public URDFModelOpenGLWithSTL(URDFRobotModel robotModel, String modelDir) {
        this.robotModel = robotModel;
        this.modelDir = modelDir;
        this.meshCache = new HashMap<>();
        logger.info("=== URDF renderer Created ===");
        
        // STL 메시 로딩
        loadAllMeshes();
    }
    
    /**
     * 모든 링크의 STL 메시를 미리 로딩
     */
    private void loadAllMeshes() {
        logger.info("=== Loading STL meshes ===");
        int loadedCount = 0;
        
        for (URDFLink link : robotModel.links) {
            if (link.visual != null && link.visual.geometry != null) {
                URDFLink.Geometry geom = link.visual.geometry;
                
                if (geom.type == URDFLink.Geometry.GeometryType.MESH && geom.meshFilename != null) {
                    File meshFile = new File(geom.meshFilename);
                    
                    if (meshFile.exists()) {
                        logger.info("Loading mesh for link '" + link.name + "': " + meshFile.getName());
                        STLLoader.STLMesh mesh = STLLoader.load(geom.meshFilename);
                        
                        if (mesh != null) {
                            // 스케일 적용
                            if (geom.scale != null && 
                                (geom.scale.x != 1.0f || geom.scale.y != 1.0f || geom.scale.z != 1.0f)) {
                                STLLoader.scaleMesh(mesh, geom.scale);
                                logger.debug("  Applied scale: " + geom.scale);
                            }
                            
                            meshCache.put(link.name, mesh);
                            loadedCount++;
                            logger.info("  ✓ Loaded " + mesh.getTriangleCount() + " triangles");
                        } else {
                            logger.error("  ✗ Failed to load mesh: " + geom.meshFilename);
                        }
                    } else {
                        logger.warn("  ✗ Mesh file not found: " + geom.meshFilename);
                    }
                }
            }
        }
        
        logger.info("=== STL Loading Complete: " + loadedCount + "/" + robotModel.getLinkCount() + " meshes ===");
    }

    @Override
    public void Render(Entity entityIn, float entityYaw, float entityPitch,
                       Vector3f entityTrans, float tickDelta, PoseStack poseStack, int packedLight) {

        renderCount++;
        if (renderCount % 20 == 1) {  // 로그 스팸 방지
            logger.info("=== URDF RENDER #" + renderCount + " ===");
        }

        // 렌더 상태 설정
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.enableDepthTest();
        RenderSystem.disableCull();  // 양면 렌더링

        // 버퍼 가져오기
        MultiBufferSource.BufferSource bufferSource = Minecraft.getInstance().renderBuffers().bufferSource();
        VertexConsumer vc = bufferSource.getBuffer(RenderType.solid());

        // 루트 링크부터 재귀 렌더링
        if (robotModel.rootLinkName != null) {
            poseStack.pushPose();
            
            // 전역 스케일 적용
            poseStack.scale(GLOBAL_SCALE, GLOBAL_SCALE, GLOBAL_SCALE);
            
            renderLinkRecursive(robotModel.rootLinkName, poseStack, vc, packedLight);
            poseStack.popPose();
        }

        // 버퍼 플러시
        bufferSource.endBatch(RenderType.solid());

        RenderSystem.enableCull();
    }
    
    /**
     * 링크를 재귀적으로 렌더링 (계층 구조 반영)
     */
    private void renderLinkRecursive(String linkName, PoseStack poseStack, VertexConsumer vc, int packedLight) {
        URDFLink link = robotModel.getLink(linkName);
        if (link == null) return;
        
        poseStack.pushPose();
        
        // 링크의 비주얼 렌더링
        if (link.visual != null) {
            renderVisual(link, poseStack, vc, packedLight);
        }
        
        // 자식 조인트들 렌더링
        for (URDFJoint childJoint : robotModel.getChildJoints(linkName)) {
            poseStack.pushPose();
            
            // 조인트 변환 적용
            applyJointTransform(childJoint, poseStack);
            
            // 자식 링크 재귀 렌더링
            renderLinkRecursive(childJoint.childLinkName, poseStack, vc, packedLight);
            
            poseStack.popPose();
        }
        
        poseStack.popPose();
    }
    
    /**
     * 링크의 비주얼 렌더링
     */
    private void renderVisual(URDFLink link, PoseStack poseStack, VertexConsumer vc, int packedLight) {
        if (link.visual == null || link.visual.geometry == null) return;
        
        poseStack.pushPose();
        
        // Visual origin 변환 적용
        if (link.visual.origin != null) {
            applyLinkOriginTransform(link.visual.origin, poseStack);
        }
        
        // 메시 렌더링
        STLLoader.STLMesh mesh = meshCache.get(link.name);
        if (mesh != null) {
            renderMesh(mesh, link, poseStack, vc, packedLight);
        } else {
            // 메시가 없으면 프리미티브 렌더링
            renderPrimitiveGeometry(link.visual.geometry, link, poseStack, vc, packedLight);
        }
        
        poseStack.popPose();
    }
    
    /**
     * STL 메시 렌더링
     */
    private void renderMesh(STLLoader.STLMesh mesh, URDFLink link, PoseStack poseStack, 
                           VertexConsumer vc, int packedLight) {
        Matrix4f matrix = poseStack.last().pose();
        
        // 색상 결정 (material에서 가져오거나 기본값)
        int r = 200, g = 200, b = 200, a = 255;
        
        if (link.visual.material != null && link.visual.material.color != null) {
            URDFLink.Material.Vector4f color = link.visual.material.color;
            r = (int)(color.x * 255);
            g = (int)(color.y * 255);
            b = (int)(color.z * 255);
            a = (int)(color.w * 255);
        }
        
        // 조명을 최대로 설정 (어둡지 않게)
        int fullBright = 0xF000F0;  // 최대 밝기
        int lightU = fullBright & 0xFFFF;
        int lightV = (fullBright >> 16) & 0xFFFF;
        
        // 모든 삼각형 렌더링
        for (STLLoader.Triangle tri : mesh.triangles) {
            for (int i = 0; i < 3; i++) {
                Vector3f v = tri.vertices[i];
                Vector3f n = tri.normal;
                
                // 법선 반전 (필요시)
                float nx = FLIP_NORMALS ? -n.x : n.x;
                float ny = FLIP_NORMALS ? -n.y : n.y;
                float nz = FLIP_NORMALS ? -n.z : n.z;
                
                vc.addVertex(matrix, v.x, v.y, v.z)
                  .setColor(r, g, b, a)
                  .setUv(0f, 0f)
                  .setUv2(lightU, lightV)
                  .setNormal(nx, ny, nz);
            }
        }
    }
    
    /**
     * 프리미티브 지오메트리 렌더링 (Box, Cylinder, Sphere)
     */
    private void renderPrimitiveGeometry(URDFLink.Geometry geom, URDFLink link, 
                                        PoseStack poseStack, VertexConsumer vc, int packedLight) {
        // 간단한 박스만 구현 (나중에 확장 가능)
        if (geom.type == URDFLink.Geometry.GeometryType.BOX && geom.boxSize != null) {
            renderBox(geom.boxSize, link, poseStack, vc, packedLight);
        }
    }
    
    /**
     * 박스 렌더링
     */
    private void renderBox(Vector3f size, URDFLink link, PoseStack poseStack, 
                          VertexConsumer vc, int packedLight) {
        Matrix4f matrix = poseStack.last().pose();
        float hx = size.x / 2.0f;
        float hy = size.y / 2.0f;
        float hz = size.z / 2.0f;
        
        // 색상
        int r = 150, g = 150, b = 150, a = 255;
        if (link.visual.material != null && link.visual.material.color != null) {
            URDFLink.Material.Vector4f color = link.visual.material.color;
            r = (int)(color.x * 255);
            g = (int)(color.y * 255);
            b = (int)(color.z * 255);
            a = (int)(color.w * 255);
        }
        
        // 6개 면 렌더링
        addQuad(vc, matrix, -hx, -hy, hz, hx, -hy, hz, hx, hy, hz, -hx, hy, hz, 0, 0, 1, r, g, b, a, packedLight);  // Front
        addQuad(vc, matrix, hx, -hy, -hz, -hx, -hy, -hz, -hx, hy, -hz, hx, hy, -hz, 0, 0, -1, r, g, b, a, packedLight);  // Back
        addQuad(vc, matrix, -hx, hy, -hz, -hx, hy, hz, hx, hy, hz, hx, hy, -hz, 0, 1, 0, r, g, b, a, packedLight);  // Top
        addQuad(vc, matrix, -hx, -hy, hz, -hx, -hy, -hz, hx, -hy, -hz, hx, -hy, hz, 0, -1, 0, r, g, b, a, packedLight);  // Bottom
        addQuad(vc, matrix, hx, -hy, hz, hx, -hy, -hz, hx, hy, -hz, hx, hy, hz, 1, 0, 0, r, g, b, a, packedLight);  // Right
        addQuad(vc, matrix, -hx, -hy, -hz, -hx, -hy, hz, -hx, hy, hz, -hx, hy, -hz, -1, 0, 0, r, g, b, a, packedLight);  // Left
    }
    
    /**
     * URDFLink.Origin 변환 적용
     */
    private void applyLinkOriginTransform(URDFLink.Origin origin, PoseStack poseStack) {
        // 위치 이동
        poseStack.translate(origin.xyz.x, origin.xyz.y, origin.xyz.z);
        
        // RPY 회전 적용
        if (origin.rpy.x != 0.0f || origin.rpy.y != 0.0f || origin.rpy.z != 0.0f) {
            Quaternionf quat = origin.getQuaternion();
            poseStack.mulPose(quat);
        }
    }
    
    /**
     * URDFJoint.Origin 변환 적용
     */
    private void applyJointOriginTransform(URDFJoint.Origin origin, PoseStack poseStack) {
        // 위치 이동
        poseStack.translate(origin.xyz.x, origin.xyz.y, origin.xyz.z);
        
        // RPY 회전 적용
        if (origin.rpy.x != 0.0f || origin.rpy.y != 0.0f || origin.rpy.z != 0.0f) {
            // URDFJoint.Origin에도 getQuaternion() 필요
            // RPY를 Quaternion으로 변환
            Quaternionf qx = new Quaternionf().rotateX(origin.rpy.x);
            Quaternionf qy = new Quaternionf().rotateY(origin.rpy.y);
            Quaternionf qz = new Quaternionf().rotateZ(origin.rpy.z);
            Quaternionf quat = qz.mul(qy).mul(qx);
            poseStack.mulPose(quat);
        }
    }
    
    /**
     * 조인트 변환 적용
     */
    private void applyJointTransform(URDFJoint joint, PoseStack poseStack) {
        if (joint.origin != null) {
            applyJointOriginTransform(joint.origin, poseStack);
        }
        
        // 조인트 움직임 적용
        if (joint.isMovable() && joint.currentPosition != 0.0f) {
            applyJointMotion(joint, poseStack);
        }
    }
    
    /**
     * 조인트 모션 적용 (Revolute, Prismatic 등)
     */
    private void applyJointMotion(URDFJoint joint, PoseStack poseStack) {
        if (joint.axis == null || joint.axis.xyz == null) return;
        
        switch (joint.type) {
            case REVOLUTE:
            case CONTINUOUS:
                // 회전 적용
                Vector3f axis = joint.axis.xyz;
                Quaternionf quat = new Quaternionf().rotateAxis(joint.currentPosition, axis.x, axis.y, axis.z);
                poseStack.mulPose(quat);
                break;
                
            case PRISMATIC:
                // 직선 이동 적용
                Vector3f trans = new Vector3f(joint.axis.xyz).mul(joint.currentPosition);
                poseStack.translate(trans.x, trans.y, trans.z);
                break;
                
            default:
                break;
        }
    }
    
    /**
     * 사각형 추가 (4개 정점 + 법선)
     */
    private void addQuad(VertexConsumer vc, Matrix4f matrix,
                         float x1, float y1, float z1,
                         float x2, float y2, float z2,
                         float x3, float y3, float z3,
                         float x4, float y4, float z4,
                         float nx, float ny, float nz,
                         int r, int g, int b, int a,
                         int light) {
        
        int lightU = light & 0xFFFF;
        int lightV = (light >> 16) & 0xFFFF;
        
        vc.addVertex(matrix, x1, y1, z1).setColor(r, g, b, a).setUv(0f, 0f).setUv2(lightU, lightV).setNormal(nx, ny, nz);
        vc.addVertex(matrix, x2, y2, z2).setColor(r, g, b, a).setUv(1f, 0f).setUv2(lightU, lightV).setNormal(nx, ny, nz);
        vc.addVertex(matrix, x3, y3, z3).setColor(r, g, b, a).setUv(1f, 1f).setUv2(lightU, lightV).setNormal(nx, ny, nz);
        vc.addVertex(matrix, x4, y4, z4).setColor(r, g, b, a).setUv(0f, 1f).setUv2(lightU, lightV).setNormal(nx, ny, nz);
    }

    @Override
    public void ChangeAnim(long anim, long layer) {
        // URDF는 애니메이션 없음
    }

    @Override
    public void ResetPhysics() {
        logger.info("ResetPhysics called");
    }

    @Override
    public long GetModelLong() {
        return 0;
    }

    @Override
    public String GetModelDir() {
        return modelDir;
    }

    public static URDFModelOpenGLWithSTL Create(String urdfPath, String modelDir) {
        logger.info("=== Creating URDF Renderer ===");
        logger.info("URDF: " + urdfPath);

        File urdfFile = new File(urdfPath);
        if (!urdfFile.exists()) {
            logger.error("URDF not found!");
            return null;
        }

        URDFRobotModel robot = URDFParser.parse(urdfFile);
        if (robot == null || robot.rootLinkName == null) {
            logger.error("URDF parse failed!");
            return null;
        }

        logger.info("✓ URDF Renderer created");
        return new URDFModelOpenGLWithSTL(robot, modelDir);
    }
}
