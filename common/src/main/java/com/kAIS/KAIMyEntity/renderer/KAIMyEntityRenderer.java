package com.kAIS.KAIMyEntity.renderer;

import com.kAIS.KAIMyEntity.KAIMyEntityClient;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.culling.Frustum;
import net.minecraft.client.renderer.entity.EntityRenderer;
import net.minecraft.client.renderer.entity.EntityRendererProvider;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.util.Mth;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.LivingEntity;
import org.joml.Quaternionf;
import org.joml.Vector3f;

public class KAIMyEntityRenderer<T extends Entity> extends EntityRenderer<T> {
    protected String modelName;
    protected EntityRendererProvider.Context context;

    public KAIMyEntityRenderer(EntityRendererProvider.Context renderManager, String entityName) {
        super(renderManager);
        this.modelName = entityName.replace(':', '.');
        this.context = renderManager;
    }

    @Override
    public boolean shouldRender(T livingEntityIn, Frustum camera, double camX, double camY, double camZ) {
        return super.shouldRender(livingEntityIn, camera, camX, camY, camZ);
    }

    @Override
    public void render(T entityIn, float entityYaw, float tickDelta, PoseStack matrixStackIn, 
                      MultiBufferSource bufferIn, int packedLightIn) {
        Minecraft MCinstance = Minecraft.getInstance();
        super.render(entityIn, entityYaw, tickDelta, matrixStackIn, bufferIn, packedLightIn);
        
        String animName = "";
        float bodyYaw = entityYaw;
        if(entityIn instanceof LivingEntity){
            bodyYaw = Mth.rotLerp(tickDelta, ((LivingEntity)entityIn).yBodyRotO, ((LivingEntity)entityIn).yBodyRot);
        }
        float bodyPitch = 0.0f;
        Vector3f entityTrans = new Vector3f(0.0f);
        
        MMDModelManager.Model model = MMDModelManager.GetModel(modelName, entityIn.getStringUUID());
        if(model == null){
            return; // 모델 없으면 렌더링 안 함
        }
        
        model.loadModelProperties(false);
        float[] size = sizeOfModel(model);
        
        matrixStackIn.pushPose();
        
        // ========== MMD 모델 애니메이션 처리 ==========
        if (model.isMMDModel()) {
            MMDModelManager.MMDModelData mwed = (MMDModelManager.MMDModelData) model;
            
            if(entityIn instanceof LivingEntity){
                LivingEntity living = (LivingEntity) entityIn;
                
                if(living.getHealth() <= 0.0F){
                    animName = "die";
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Die, 0);
                } else if(living.isSleeping()){
                    animName = "sleep";
                    bodyYaw = living.getBedOrientation().toYRot() + 180.0f;
                    bodyPitch = getFloatProperty(model, "sleepingPitch", 0.0f);
                    entityTrans = getVec3Property(model, "sleepingTrans");
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Sleep, 0);
                }
                
                if(living.isBaby()){
                    matrixStackIn.scale(0.5f, 0.5f, 0.5f);
                }
            }
            
            if(animName.isEmpty()){
                if (entityIn.isVehicle() && 
                    (entityIn.getX() - entityIn.xo != 0.0f || entityIn.getZ() - entityIn.zo != 0.0f)) {
                    animName = "driven";
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Driven, 0);
                } else if (entityIn.isVehicle()) {
                    animName = "ridden";
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Ridden, 0);
                } else if (entityIn.isSwimming()) {
                    animName = "swim";
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Swim, 0);
                } else if ((entityIn.getX() - entityIn.xo != 0.0f || entityIn.getZ() - entityIn.zo != 0.0f) && 
                           entityIn.getVehicle() == null) {
                    animName = "walk";
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Walk, 0);
                } else {
                    animName = "idle";
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Idle, 0);
                }
            }
        }
        // URDF 모델은 애니메이션 없으므로 그냥 렌더링
        
        // ========== 인벤토리 렌더링 체크 ==========
        if(KAIMyEntityClient.calledFrom(6).contains("Inventory") || 
           KAIMyEntityClient.calledFrom(6).contains("class_490")){
            renderInInventory(entityIn, model, size, entityYaw, bodyYaw, tickDelta, 
                            matrixStackIn, packedLightIn, MCinstance);
        } else {
            matrixStackIn.scale(size[0], size[0], size[0]);
            RenderSystem.setShader(GameRenderer::getRendertypeEntityCutoutNoCullShader);
            model.model.Render(entityIn, bodyYaw, bodyPitch, entityTrans, tickDelta, matrixStackIn, packedLightIn);
        }
        
        matrixStackIn.popPose();
    }
    
    private void renderInInventory(T entityIn, MMDModelManager.Model model, float[] size,
                                   float entityYaw, float bodyYaw, float tickDelta,
                                   PoseStack matrixStackIn, int packedLightIn, Minecraft MCinstance) {
        RenderSystem.setShader(GameRenderer::getPositionTexShader);
        PoseStack PTS_modelViewStack = new PoseStack();
        PTS_modelViewStack.setIdentity();
        PTS_modelViewStack.mulPose(RenderSystem.getModelViewMatrix());
        
        int PosX_in_inventory = (MCinstance.screen.width - 176) / 2;
        int PosY_in_inventory = (MCinstance.screen.height - 166) / 2;
        PTS_modelViewStack.translate(PosX_in_inventory+51, PosY_in_inventory+60, 50.0);
        PTS_modelViewStack.pushPose();
        PTS_modelViewStack.scale(20.0f,20.0f, -20.0f);
        PTS_modelViewStack.scale(size[1], size[1], size[1]);
        
        Quaternionf quaternionf = (new Quaternionf()).rotateZ((float)Math.PI);
        Quaternionf quaternionf1 = (new Quaternionf()).rotateX(-entityIn.getXRot() * ((float)Math.PI / 180F));
        Quaternionf quaternionf2 = (new Quaternionf()).rotateY(-entityIn.getYRot() * ((float)Math.PI / 180F));
        quaternionf.mul(quaternionf1);
        quaternionf.mul(quaternionf2);
        PTS_modelViewStack.mulPose(quaternionf);
        
        RenderSystem.setShader(GameRenderer::getRendertypeEntityCutoutNoCullShader);
        model.model.Render(entityIn, entityYaw, 0.0f, new Vector3f(0.0f), tickDelta, 
                          PTS_modelViewStack, packedLightIn);
        PTS_modelViewStack.popPose();
    }

    float[] sizeOfModel(MMDModelManager.Model model){
        float[] size = new float[2];
        size[0] = getFloatProperty(model, "size", 1.0f);
        size[1] = getFloatProperty(model, "size_in_inventory", 1.0f);
        return size;
    }

    void AnimStateChangeOnce(MMDModelManager.MMDModelData model, 
                            MMDModelManager.EntityData.EntityState targetState, Integer layer) {
        if (model.entityData.stateLayers[layer] != targetState) {
            String Property = MMDModelManager.EntityData.stateProperty.get(targetState);
            model.entityData.stateLayers[layer] = targetState;
            model.model.ChangeAnim(MMDAnimManager.GetAnimModel(model.model, Property), layer);
        }
    }
    
    float getFloatProperty(MMDModelManager.Model model, String key, float defaultValue) {
        String val = model.properties.getProperty(key);
        return (val == null) ? defaultValue : Float.valueOf(val);
    }
    
    Vector3f getVec3Property(MMDModelManager.Model model, String key) {
        String val = model.properties.getProperty(key);
        return (val == null) ? new Vector3f(0.0f) : KAIMyEntityClient.str2Vec3f(val);
    }

    @Override
    public ResourceLocation getTextureLocation(T entity) {
        return null;
    }
}
