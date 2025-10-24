package com.kAIS.KAIMyEntity.urdf;

import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.world.entity.Entity;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joml.Vector3f;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * URDF 로딩 테스트용 - 렌더링은 나중에
 * 일단 파싱이 제대로 되는지만 확인
 */
public class URDFModelTest implements IMMDModel {
    private static final Logger logger = LogManager.getLogger();
    
    private URDFRobotModel robotModel;
    private String modelDir;
    private boolean hasWarned = false;
    
    public URDFModelTest(URDFRobotModel robotModel, String modelDir) {
        this.robotModel = robotModel;
        this.modelDir = modelDir;
        
        logger.info("=== URDF Model Loaded ===");
        logger.info("Robot: " + robotModel.name);
        logger.info("Links: " + robotModel.getLinkCount());
        logger.info("Joints: " + robotModel.getJointCount());
        logger.info("Root: " + robotModel.rootLinkName);
        
        // 각 링크 정보 출력
        for (URDFLink link : robotModel.links) {
            logger.info("  Link: " + link.name);
            if (link.visual != null && link.visual.geometry != null) {
                logger.info("    Geometry: " + link.visual.geometry.type);
            }
        }
        
        // 각 조인트 정보 출력
        for (URDFJoint joint : robotModel.joints) {
            logger.info("  Joint: " + joint.name + " (" + joint.type + ")");
            logger.info("    " + joint.parentLinkName + " -> " + joint.childLinkName);
        }
    }
    
    @Override
    public void Render(Entity entityIn, float entityYaw, float entityPitch, 
                      Vector3f entityTrans, float tickDelta, PoseStack mat, int packedLight) {
        // 렌더링 대신 경고만 한번 출력
        if (!hasWarned) {
            logger.warn("URDF Rendering called for " + robotModel.name + " but not implemented yet!");
            logger.warn("This is just a loading test. Check logs to see if URDF was parsed correctly.");
            hasWarned = true;
        }
        
        // TODO: 실제 렌더링 구현
        // 지금은 아무것도 안 그림
    }
    
    @Override
    public void ChangeAnim(long anim, long layer) {
        // URDF는 애니메이션 없음
    }
    
    @Override
    public void ResetPhysics() {
        // Reset all joints to 0
        for (URDFJoint joint : robotModel.joints) {
            joint.currentPosition = 0.0f;
        }
        logger.info("Reset physics for " + robotModel.name);
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
    
    /**
     * Factory method - 이걸로 생성
     */
    public static URDFModelTest Create(String urdfPath, String modelDir) {
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
        return new URDFModelTest(robot, modelDir);
    }
    
    /**
     * 조인트 상태 출력 (디버깅용)
     */
    public void printJointStates() {
        logger.info("=== Joint States for " + robotModel.name + " ===");
        for (URDFJoint joint : robotModel.joints) {
            if (joint.isMovable()) {
                logger.info(String.format("  %s: %.3f rad", joint.name, joint.currentPosition));
            }
        }
    }
}
