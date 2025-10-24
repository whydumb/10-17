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
 * URDF 모델을 간단한 박스로 렌더링
 * Forward Kinematics 적용
 */
public class URDFModelOpenGL implements IMMDModel {
    private static final Logger logger = LogManager.getLogger();
    
    private URDFRobotModel robotModel;
    private String modelDir;
    
    // Forward kinematics cache
    private Map<String, Matrix4f> linkTransforms;
    
    public URDFModelOpenGL(URDFRobotModel robotModel, String modelDir) {
        this.robotModel = robotModel;
        this.modelDir = modelDir;
        this.linkTransforms = new HashMap<>();
        
        logger.info("URDFModelOpenGL initialized for " + robotModel.name);
        logger.info("Links: " + robotModel.getLinkCount() + ", Joints: " + robotModel.getJointCount());
    }
    
    @Override
    public void Render(Entity entityIn, float entityYaw, float entityPitch, 
                      Vector3f entityTrans, float tickDelta, PoseStack mat, int packedLight) {
        
        // Forward kinematics 계산
        updateForwardKinematics();
        
        // 각 링크 렌더링
        for (URDFLink link : robotModel.links) {
            // visual이나 geometry가 없으면 스킵
            if (link.visual == null) {
                continue;
            }
            if (link.visual.geometry == null) {
                continue;
            }
            
            Matrix4f linkTransform = linkTransforms.getOrDefault(link.name, new Matrix4f());
            
            mat.pushPose();
            
            // 링크 변환 적용
            mat.last().pose().mul(linkTransform);
            
            // Visual origin 적용
            if (link.visual.origin != null) {
                mat.translate(link.visual.origin.xyz.x, link.visual.origin.xyz.y, link.visual.origin.xyz.z);
                mat.mulPose(link.visual.origin.getQuaternion());
            }
            
            // 지오메트리 렌더링
            renderGeometry(link.visual.geometry, link.visual.material, mat);
            
            mat.popPose();
        }
    }
    
    private void updateForwardKinematics() {
        linkTransforms.clear();
        
        // Root부터 시작
        Matrix4f rootTransform = new Matrix4f().identity();
        linkTransforms.put(robotModel.rootLinkName, rootTransform);
        
        // 재귀적으로 계산
        computeLinkTransform(robotModel.rootLinkName, rootTransform);
    }
    
    private void computeLinkTransform(String linkName, Matrix4f parentTransform) {
        List<URDFJoint> childJoints = robotModel.getChildJoints(linkName);
        
        for (URDFJoint joint : childJoints) {
            Matrix4f jointTransform = new Matrix4f(parentTransform);
            
            // Joint origin 적용
            jointTransform.translate(joint.origin.xyz.x, joint.origin.xyz.y, joint.origin.xyz.z);
            jointTransform.rotate(new Quaternionf().rotateZYX(
                joint.origin.rpy.z, joint.origin.rpy.y, joint.origin.rpy.x));
            
            // Joint 회전 적용
            if (joint.isMovable()) {
                Quaternionf rotation = new Quaternionf().rotateAxis(
                    joint.currentPosition, 
                    joint.axis.xyz.x, 
                    joint.axis.xyz.y, 
                    joint.axis.xyz.z
                );
                jointTransform.rotate(rotation);
            }
            
            linkTransforms.put(joint.childLinkName, new Matrix4f(jointTransform));
            
            // 재귀
            computeLinkTransform(joint.childLinkName, jointTransform);
        }
    }
    
    private void renderGeometry(URDFLink.Geometry geom, URDFLink.Material material, PoseStack mat) {
        if (geom == null) {
            // geometry가 없으면 작은 디버그 박스
            renderBox(new Vector3f(0.05f, 0.05f, 0.05f), 1.0f, 0.0f, 0.0f, 0.5f, mat);
            return;
        }
        
        // 색상 결정
        float r = 0.7f, g = 0.7f, b = 0.7f, a = 1.0f;
        if (material != null && material.color != null) {
            r = material.color.x;
            g = material.color.y;
            b = material.color.z;
            a = material.color.w;
        }
        
        switch (geom.type) {
            case BOX:
                renderBox(geom.boxSize, r, g, b, a, mat);
                break;
            case CYLINDER:
                renderCylinder(geom.cylinderRadius, geom.cylinderLength, r, g, b, a, mat);
                break;
            case SPHERE:
                renderSphere(geom.sphereRadius, r, g, b, a, mat);
                break;
            case MESH:
                // TODO: STL 로딩
                renderBox(new Vector3f(0.1f, 0.1f, 0.1f), r, g, b, a, mat);
                break;
        }
    }
    
    private void renderBox(Vector3f size, float r, float g, float b, float a, PoseStack mat) {
        RenderSystem.setShader(GameRenderer::getPositionColorShader);
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        
        BufferBuilder bufferBuilder = Tesselator.getInstance().begin(
            VertexFormat.Mode.QUADS, 
            DefaultVertexFormat.POSITION_COLOR
        );
        
        Matrix4f matrix = mat.last().pose();
        
        float hx = size.x / 2.0f;
        float hy = size.y / 2.0f;
        float hz = size.z / 2.0f;
        
        // 6면 그리기
        // Front face
        bufferBuilder.addVertex(matrix, -hx, -hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, -hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, hy, hz).setColor(r, g, b, a);
        
        // Back face
        bufferBuilder.addVertex(matrix, -hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, -hy, -hz).setColor(r, g, b, a);
        
        // Top face
        bufferBuilder.addVertex(matrix, -hx, hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, hy, -hz).setColor(r, g, b, a);
        
        // Bottom face
        bufferBuilder.addVertex(matrix, -hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, -hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, -hy, hz).setColor(r, g, b, a);
        
        // Right face
        bufferBuilder.addVertex(matrix, hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, hx, -hy, hz).setColor(r, g, b, a);
        
        // Left face
        bufferBuilder.addVertex(matrix, -hx, -hy, -hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, -hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, hy, hz).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -hx, hy, -hz).setColor(r, g, b, a);
        
        BufferUploader.drawWithShader(bufferBuilder.buildOrThrow());
    }
    
    private void renderCylinder(float radius, float length, float r, float g, float b, float a, PoseStack mat) {
        // 간단히 박스로 대체
        renderBox(new Vector3f(radius * 2, radius * 2, length), r, g, b, a, mat);
    }
    
    private void renderSphere(float radius, float r, float g, float b, float a, PoseStack mat) {
        // 간단히 박스로 대체
        renderBox(new Vector3f(radius * 2, radius * 2, radius * 2), r, g, b, a, mat);
    }
    
    @Override
    public void ChangeAnim(long anim, long layer) {
        // URDF는 애니메이션 없음
    }
    
    @Override
    public void ResetPhysics() {
        for (URDFJoint joint : robotModel.joints) {
            joint.currentPosition = 0.0f;
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
    
    public URDFRobotModel getRobotModel() {
        return robotModel;
    }
    
    public void updateJointPositions(Map<String, Float> positions) {
        robotModel.updateJointPositions(positions);
    }
    
    public static URDFModelOpenGL Create(String urdfPath, String modelDir) {
        logger.info("Loading URDF from: " + urdfPath);
        
        File urdfFile = new File(urdfPath);
        if (!urdfFile.exists()) {
            logger.error("URDF file not found: " + urdfPath);
            return null;
        }
        
        URDFRobotModel robot = URDFParser.parse(urdfFile);
        if (robot == null) {
            logger.error("Failed to parse URDF: " + urdfPath);
            return null;
        }
        
        logger.info("✓ URDF parsed successfully!");
        return new URDFModelOpenGL(robot, modelDir);
    }
}