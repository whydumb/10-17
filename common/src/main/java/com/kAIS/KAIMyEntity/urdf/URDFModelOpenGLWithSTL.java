package com.kAIS.KAIMyEntity.urdf;

import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.*;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.world.entity.Entity;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joml.Matrix4f;
import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * URDF 모델 렌더링 + STL 메시 지원
 */
public class URDFModelOpenGLWithSTL implements IMMDModel {
    private static final Logger logger = LogManager.getLogger();

    // ========== 설정 ==========
    private static final float URDF_TO_MC_SCALE = 10.0f;
    private static final boolean USE_STL_MESHES = true; // STL 사용 여부
    private static final boolean DEBUG_RENDER = false; // 디버그 박스 표시
    
    private URDFRobotModel robotModel;
    private String modelDir;
    private Map<String, Matrix4f> linkTransforms;
    private int renderCallCount = 0;

    public URDFModelOpenGLWithSTL(URDFRobotModel robotModel, String modelDir) {
        this.robotModel = robotModel;
        this.modelDir = modelDir;
        this.linkTransforms = new HashMap<>();

        logger.info("=== URDFModelOpenGL Created (with STL) ===");
        logger.info("Robot: " + robotModel.name);
        logger.info("Links: " + robotModel.getLinkCount());
        logger.info("Joints: " + robotModel.getJointCount());
        logger.info("Root: " + robotModel.rootLinkName);
        logger.info("STL Support: " + USE_STL_MESHES);
    }

    @Override
    public void Render(Entity entityIn, float entityYaw, float entityPitch,
                       Vector3f entityTrans, float tickDelta, PoseStack mat, int packedLight) {

        renderCallCount++;

        updateForwardKinematics();

        int renderedCount = 0;
        int stlCount = 0;
        int boxCount = 0;

        for (URDFLink link : robotModel.links) {
            if (link.visual == null) {
                continue;
            }

            Matrix4f linkTransform = linkTransforms.get(link.name);
            if (linkTransform == null) {
                linkTransform = new Matrix4f().identity();
            }

            mat.pushPose();
            mat.last().pose().mul(linkTransform);

            if (link.visual.origin != null) {
                mat.translate(
                    link.visual.origin.xyz.x,
                    link.visual.origin.xyz.y,
                    link.visual.origin.xyz.z
                );
                mat.mulPose(link.visual.origin.getQuaternion());
            }

            if (link.visual.geometry != null) {
                boolean usedSTL = renderGeometry(
                    link.visual.geometry, 
                    link.visual.material, 
                    mat, 
                    link.name
                );
                
                if (usedSTL) {
                    stlCount++;
                } else {
                    boxCount++;
                }
                renderedCount++;
            }

            mat.popPose();
        }

        if (renderCallCount == 1) {
            logger.info(String.format("First render: %d links (%d STL, %d boxes)", 
                renderedCount, stlCount, boxCount));
        }
    }

    // ========== Forward Kinematics ==========

    private void updateForwardKinematics() {
        linkTransforms.clear();

        Matrix4f rootTransform = new Matrix4f().identity();
        rootTransform.scale(URDF_TO_MC_SCALE, URDF_TO_MC_SCALE, URDF_TO_MC_SCALE);
        
        linkTransforms.put(robotModel.rootLinkName, rootTransform);
        computeLinkTransform(robotModel.rootLinkName, rootTransform);
    }

    private void computeLinkTransform(String linkName, Matrix4f parentTransform) {
        List<URDFJoint> childJoints = robotModel.getChildJoints(linkName);

        for (URDFJoint joint : childJoints) {
            Matrix4f jointTransform = new Matrix4f(parentTransform);

            jointTransform.translate(
                joint.origin.xyz.x,
                joint.origin.xyz.y,
                joint.origin.xyz.z
            );

            if (joint.origin.rpy.lengthSquared() > 0.0001f) {
                Quaternionf originRot = new Quaternionf()
                    .rotateZ(joint.origin.rpy.z)
                    .rotateY(joint.origin.rpy.y)
                    .rotateX(joint.origin.rpy.x);
                jointTransform.rotate(originRot);
            }

            if (joint.isMovable() && Math.abs(joint.currentPosition) > 0.0001f) {
                Vector3f axis = joint.axis.xyz;
                
                float axisLength = axis.length();
                if (axisLength > 0.0001f) {
                    axis = new Vector3f(axis).normalize();
                    
                    Quaternionf jointRot = new Quaternionf().rotateAxis(
                        joint.currentPosition,
                        axis.x,
                        axis.y,
                        axis.z
                    );
                    jointTransform.rotate(jointRot);
                }
            }

            linkTransforms.put(joint.childLinkName, new Matrix4f(jointTransform));
            computeLinkTransform(joint.childLinkName, jointTransform);
        }
    }

    // ========== Geometry 렌더링 (STL 지원) ==========

    /**
     * @return true if STL was used, false if fallback
     */
    private boolean renderGeometry(URDFLink.Geometry geom, URDFLink.Material material, 
                                    PoseStack mat, String linkName) {
        if (geom == null) {
            if (DEBUG_RENDER) {
                renderBox(new Vector3f(0.02f, 0.02f, 0.02f), 1.0f, 1.0f, 0.0f, 0.5f, mat);
            }
            return false;
        }

        // 색상
        float r = 0.7f, g = 0.7f, b = 0.7f, a = 1.0f;
        if (material != null && material.color != null) {
            r = material.color.x;
            g = material.color.y;
            b = material.color.z;
            a = material.color.w;
        }

        switch (geom.type) {
            case MESH:
                if (USE_STL_MESHES && geom.meshFilename != null) {
                    File meshFile = new File(geom.meshFilename);
                    if (meshFile.exists()) {
                        // STL 렌더링
                        try {
                            STLMeshRenderer.renderSTLImmediate(
                                geom.meshFilename,
                                geom.scale,
                                r, g, b, a,
                                mat
                            );
                            return true;
                        } catch (Exception e) {
                            logger.warn("STL render failed for " + linkName + ": " + e.getMessage());
                            // Fallback to box
                            renderBox(new Vector3f(0.05f, 0.05f, 0.05f), r, g, b, a, mat);
                            return false;
                        }
                    }
                }
                // Fallback: 박스
                Vector3f size = geom.scale != null ? 
                    new Vector3f(geom.scale).mul(0.05f) : 
                    new Vector3f(0.05f, 0.05f, 0.05f);
                renderBox(size, r, g, b, a, mat);
                return false;
                
            case BOX:
                renderBox(geom.boxSize, r, g, b, a, mat);
                return false;
                
            case CYLINDER:
                renderCylinder(geom.cylinderRadius, geom.cylinderLength, r, g, b, a, mat);
                return false;
                
            case SPHERE:
                renderSphere(geom.sphereRadius, r, g, b, a, mat);
                return false;
                
            default:
                return false;
        }
    }

    // ========== 기본 도형 렌더링 ==========

    private void renderBox(Vector3f size, float r, float g, float b, float a, PoseStack mat) {
        RenderSystem.setShader(GameRenderer::getPositionColorShader);
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.disableCull();

        BufferBuilder bufferBuilder = Tesselator.getInstance().begin(
            VertexFormat.Mode.QUADS,
            DefaultVertexFormat.POSITION_COLOR
        );

        Matrix4f matrix = mat.last().pose();

        float hx = size.x / 2.0f;
        float hy = size.y / 2.0f;
        float hz = size.z / 2.0f;

        // Front (+Z)
        bufferBuilder.addVertex(matrix, -hx, -hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx, -hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx,  hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx,  hy,  hz).setColor(r, g, b, a);

        // Back (-Z)
        bufferBuilder.addVertex(matrix,  hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx,  hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx,  hy, -hz).setColor(r, g, b, a);

        // Top (+Y)
        bufferBuilder.addVertex(matrix, -hx,  hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx,  hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx,  hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx,  hy, -hz).setColor(r, g, b, a);

        // Bottom (-Y)
        bufferBuilder.addVertex(matrix, -hx, -hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx, -hy,  hz).setColor(r, g, b, a);

        // Right (+X)
        bufferBuilder.addVertex(matrix,  hx, -hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx,  hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix,  hx,  hy,  hz).setColor(r, g, b, a);

        // Left (-X)
        bufferBuilder.addVertex(matrix, -hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, -hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx,  hy,  hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx,  hy, -hz).setColor(r, g, b, a);

        BufferUploader.drawWithShader(bufferBuilder.buildOrThrow());
        RenderSystem.enableCull();
    }

    private void renderCylinder(float radius, float length, float r, float g, float b, float a, PoseStack mat) {
        RenderSystem.setShader(GameRenderer::getPositionColorShader);
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.disableCull();

        Matrix4f matrix = mat.last().pose();
        int segments = 16;
        float halfLen = length / 2.0f;

        BufferBuilder bufferBuilder = Tesselator.getInstance().begin(
            VertexFormat.Mode.QUADS,
            DefaultVertexFormat.POSITION_COLOR
        );

        for (int i = 0; i < segments; i++) {
            float angle1 = (float)(2 * Math.PI * i / segments);
            float angle2 = (float)(2 * Math.PI * (i + 1) / segments);
            
            float x1 = radius * (float)Math.cos(angle1);
            float z1 = radius * (float)Math.sin(angle1);
            float x2 = radius * (float)Math.cos(angle2);
            float z2 = radius * (float)Math.sin(angle2);

            bufferBuilder.addVertex(matrix, x1, -halfLen, z1).setColor(r, g, b, a);
            bufferBuilder.addVertex(matrix, x2, -halfLen, z2).setColor(r, g, b, a);
            bufferBuilder.addVertex(matrix, x2,  halfLen, z2).setColor(r, g, b, a);
            bufferBuilder.addVertex(matrix, x1,  halfLen, z1).setColor(r, g, b, a);
        }

        BufferUploader.drawWithShader(bufferBuilder.buildOrThrow());
        RenderSystem.enableCull();
    }

    private void renderSphere(float radius, float r, float g, float b, float a, PoseStack mat) {
        renderBox(new Vector3f(radius * 2, radius * 2, radius * 2), r, g, b, a, mat);
    }

    // ========== IMMDModel 구현 ==========

    @Override
    public void ChangeAnim(long anim, long layer) {
    }

    @Override
    public void ResetPhysics() {
        for (URDFJoint joint : robotModel.joints) {
            joint.currentPosition = 0.0f;
            joint.currentVelocity = 0.0f;
        }
    }

    @Override
    public long GetModelLong() {
        return 0;
    }

    @Override
    public String GetModelDir() {
        return modelDir;
    }

    // ========== Public API ==========

    public URDFRobotModel getRobotModel() {
        return robotModel;
    }

    public void updateJointPositions(Map<String, Float> positions) {
        robotModel.updateJointPositions(positions);
    }

    public void setJointPosition(String jointName, float position) {
        URDFJoint joint = robotModel.getJoint(jointName);
        if (joint != null) {
            joint.updatePosition(position);
        }
    }

    // ========== Factory ==========

    public static URDFModelOpenGLWithSTL Create(String urdfPath, String modelDir) {
        logger.info("Loading URDF: " + urdfPath);

        File urdfFile = new File(urdfPath);
        if (!urdfFile.exists()) {
            logger.error("URDF not found: " + urdfPath);
            return null;
        }

        URDFRobotModel robot = URDFParser.parse(urdfFile);
        if (robot == null || robot.rootLinkName == null) {
            logger.error("Failed to parse URDF");
            return null;
        }

        logger.info("✓ URDF loaded: " + robot.name);
        return new URDFModelOpenGLWithSTL(robot, modelDir);
    }
}
