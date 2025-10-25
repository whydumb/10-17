package com.kAIS.KAIMyEntity.urdf;

import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.*;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.world.entity.Entity;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joml.Matrix4f;
import org.joml.Vector3f;

import java.io.File;

/**
 * URDF 모델 렌더링 + STL 메시 지원
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
                       Vector3f entityTrans, float tickDelta, PoseStack mat, int packedLight) {

        renderCount++;

        logger.info("=== URDF RENDER CALLED #" + renderCount + " ===");
        logger.info("Entity: " + entityIn.getName().getString());

        //
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.enableDepthTest();
        RenderSystem.disableDull();

        Tesselater tesselator = Tesselator.getInstance();
        BufferBuilder bufferBuilder = tesselator.begin(
            VertexFormat.Mode.QUADS,
            DefaultVertexFormat.POSITION_COLOR
        );

        Matrix4f matrix = mat.last().pose();
        float size = 1.0f;
        float r = 1.0f, g = 0.0f, b = 0.0f, a = 1.0f;


        // 6면 박스
        // Front
        bufferBuilder.addVertex(matrix, -size, 0, size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 0, size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 2*size, size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -size, 2*size, size).setColor(r, g, b, a);

        //back
        bufferBuilder.addVertex(matrix, size, 0, -size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -size, 0, -size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -size, 2*size, -size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 2*size, -size).setColor(r, g, b, a);

        //Top
        bufferBuilder.addVertex(matrix, -size, 2*size, -size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -size, 2*size, size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 2*size, size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 2*size, -size).setColor(r, g, b, a);

        //Bottom 
        bufferBuilder.addVertex(matrix, -size, 0, size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, -size, 0, -size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 0, -size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 0, size).setColor(r, g, b, a);

        //RIght
        bufferBuilder.addVertex(matrix, size, 0, size).setColor(r, g, b, a);
        bufferBuilder.addVertex(matrix, size, 0, -size).setColor(r, g, b, a);
        bufferBuilder.addvertex(matrix, size, 2*size, -size).setColor(r, g, b, a);
        bufferBuilder.addvertex(matrix, size, 2*size, size).setColor(r, g, b, a);
        
        BufferUploader.darwWithSharder(bufferBuilder.buildOrThrow());
        RenderSystem.enableCull();

        logger.info("Red Box render done");
    }

    @Override
    public void ChangeAnim(long anim, long layer) {
        // 암것도 안함
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
        
        
    
