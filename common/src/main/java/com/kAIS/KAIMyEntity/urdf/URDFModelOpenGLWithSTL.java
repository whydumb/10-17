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
import org.joml.Vector3f;

import java.io.File;

/**
 * URDF 모델 렌더링 (완전한 Vertex 데이터)
 */
public class URDFModelOpenGLWithSTL implements IMMDModel {
    private static final Logger logger = LogManager.getLogger();
    private static int renderCount = 0;

    private URDFRobotModel robotModel;
    private String modelDir;

    public URDFModelOpenGLWithSTL(URDFRobotModel robotModel, String modelDir) {
        this.robotModel = robotModel;
        this.modelDir = modelDir;
        logger.info("=== URDF renderer Created ===");
    }

    @Override
    public void Render(Entity entityIn, float entityYaw, float entityPitch,
                       Vector3f entityTrans, float tickDelta, PoseStack poseStack, int packedLight) {

        renderCount++;
        logger.info("=== URDF RENDER CALLED #" + renderCount + " ===");

        // 렌더 상태 설정
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.enableDepthTest();
        RenderSystem.disableCull();

        // 버퍼 가져오기
        MultiBufferSource.BufferSource bufferSource = Minecraft.getInstance().renderBuffers().bufferSource();
        VertexConsumer vc = bufferSource.getBuffer(RenderType.solid());

        Matrix4f matrix = poseStack.last().pose();
        float size = 1.0f;
        
        // 색상 (RGBA 0-255)
        int red = 255, green = 0, blue = 0, alpha = 255;
        
        // ===== 육면체 렌더링 (각 면마다 4개 정점 + Normal) =====
        
        // Front face (+Z) - Normal: (0, 0, 1)
        addQuad(vc, matrix,
            -size, 0f,    size,  // v1
             size, 0f,    size,  // v2
             size, 2*size, size,  // v3
            -size, 2*size, size,  // v4
            0, 0, 1,  // normal
            red, green, blue, alpha, packedLight);
        
        // Back face (-Z) - Normal: (0, 0, -1)
        addQuad(vc, matrix,
             size, 0f,   -size,
            -size, 0f,   -size,
            -size, 2*size,-size,
             size, 2*size,-size,
            0, 0, -1,
            red, green, blue, alpha, packedLight);
        
        // Top face (+Y) - Normal: (0, 1, 0)
        addQuad(vc, matrix,
            -size, 2*size, -size,
            -size, 2*size,  size,
             size, 2*size,  size,
             size, 2*size, -size,
            0, 1, 0,
            red, green, blue, alpha, packedLight);
        
        // Bottom face (-Y) - Normal: (0, -1, 0)
        addQuad(vc, matrix,
            -size, 0f,    size,
            -size, 0f,   -size,
             size, 0f,   -size,
             size, 0f,    size,
            0, -1, 0,
            red, green, blue, alpha, packedLight);
        
        // Right face (+X) - Normal: (1, 0, 0)
        addQuad(vc, matrix,
             size, 0f,    size,
             size, 0f,   -size,
             size, 2*size,-size,
             size, 2*size, size,
            1, 0, 0,
            red, green, blue, alpha, packedLight);
        
        // Left face (-X) - Normal: (-1, 0, 0)
        addQuad(vc, matrix,
            -size, 0f,   -size,
            -size, 0f,    size,
            -size, 2*size, size,
            -size, 2*size,-size,
            -1, 0, 0,
            red, green, blue, alpha, packedLight);

        // 버퍼 플러시
        bufferSource.endBatch(RenderType.solid());

        RenderSystem.enableCull();
        logger.info("✓ Red Box rendered");
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
        
        // Light를 UV2로 변환 (Minecraft 1.21 형식)
        int lightU = light & 0xFFFF;        // Block light
        int lightV = (light >> 16) & 0xFFFF; // Sky light
        
        // Vertex 1
        vc.addVertex(matrix, x1, y1, z1)
          .setColor(r, g, b, a)
          .setUv(0f, 0f)
          .setUv2(lightU, lightV)
          .setNormal(nx, ny, nz);
        
        // Vertex 2
        vc.addVertex(matrix, x2, y2, z2)
          .setColor(r, g, b, a)
          .setUv(1f, 0f)
          .setUv2(lightU, lightV)
          .setNormal(nx, ny, nz);
        
        // Vertex 3
        vc.addVertex(matrix, x3, y3, z3)
          .setColor(r, g, b, a)
          .setUv(1f, 1f)
          .setUv2(lightU, lightV)
          .setNormal(nx, ny, nz);
        
        // Vertex 4
        vc.addVertex(matrix, x4, y4, z4)
          .setColor(r, g, b, a)
          .setUv(0f, 1f)
          .setUv2(lightU, lightV)
          .setNormal(nx, ny, nz);
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
