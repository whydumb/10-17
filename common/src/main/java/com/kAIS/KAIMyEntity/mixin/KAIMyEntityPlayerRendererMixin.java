package com.kAIS.KAIMyEntity.mixin;

import com.kAIS.KAIMyEntity.KAIMyEntityClient;
import com.kAIS.KAIMyEntity.NativeFunc;
import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.kAIS.KAIMyEntity.renderer.MMDAnimManager;
import com.kAIS.KAIMyEntity.renderer.MMDModelManager;
import com.kAIS.KAIMyEntity.renderer.MMDModelManager.MMDModelData;
import com.kAIS.KAIMyEntity.renderer.MMDModelManager.URDFModelData;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.inventory.InventoryScreen;
import net.minecraft.client.model.PlayerModel;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.entity.EntityRendererProvider;
import net.minecraft.client.renderer.entity.LivingEntityRenderer;
import net.minecraft.client.renderer.entity.player.PlayerRenderer;
import net.minecraft.client.renderer.texture.OverlayTexture;
import net.minecraft.util.Mth;
import net.minecraft.world.InteractionHand;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.item.ItemDisplayContext;
import net.minecraft.world.level.GameType;
import org.joml.Matrix4f;
import org.joml.Quaternionf;
import org.joml.Vector3f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(PlayerRenderer.class)
public abstract class KAIMyEntityPlayerRendererMixin extends LivingEntityRenderer<AbstractClientPlayer, PlayerModel<AbstractClientPlayer>> {

    public KAIMyEntityPlayerRendererMixin(EntityRendererProvider.Context ctx, PlayerModel<AbstractClientPlayer> model, float shadowRadius) {
        super(ctx, model, shadowRadius);
    }

    @Inject(method = {"render"}, at = @At("HEAD"), cancellable = true)
    public void render(AbstractClientPlayer entityIn, float entityYaw, float tickDelta, PoseStack matrixStackIn, MultiBufferSource vertexConsumers, int packedLightIn, CallbackInfo ci) {
        Minecraft MCinstance = Minecraft.getInstance();
        
        // ✓ 모델 가져오기 (플레이어별 → 기본 순)
        MMDModelManager.Model m = MMDModelManager.GetModel("EntityPlayer_" + entityIn.getName().getString());
        if (m == null) {
            m = MMDModelManager.GetModel("EntityPlayer");
        }
        
        if (m == null) {
            // 모델 없으면 기본 렌더링
            super.render(entityIn, entityYaw, tickDelta, matrixStackIn, vertexConsumers, packedLightIn);
            return;
        }
        
        IMMDModel model = m.model;
        m.loadModelProperties(KAIMyEntityClient.reloadProperties);
        
        // ========== 모델 타입 체크 ==========
        boolean isMMD = m.isMMDModel();
        boolean isURDF = m.isURDFModel();
        
        // ========== 공통 파라미터 ==========
        float bodyYaw = Mth.rotLerp(tickDelta, entityIn.yBodyRotO, entityIn.yBodyRot);
        float bodyPitch = 0.0f;
        Vector3f entityTrans = new Vector3f(0.0f);
        float[] size = sizeOfModel(m);
        
        // ========== MMD 전용 애니메이션 처리 ==========
        if (isMMD) {
            MMDModelData mmdData = (MMDModelData) m;
            handleMMDAnimation(entityIn, mmdData, tickDelta);
            
            // Pose 조정
            float sleepingPitch = getFloatProperty(m, "sleepingPitch", 0.0f);
            Vector3f sleepingTrans = getVec3Property(m, "sleepingTrans");
            float flyingPitch = getFloatProperty(m, "flyingPitch", 0.0f);
            Vector3f flyingTrans = getVec3Property(m, "flyingTrans");
            float swimmingPitch = getFloatProperty(m, "swimmingPitch", 0.0f);
            Vector3f swimmingTrans = getVec3Property(m, "swimmingTrans");
            float crawlingPitch = getFloatProperty(m, "crawlingPitch", 0.0f);
            Vector3f crawlingTrans = getVec3Property(m, "crawlingTrans");
            
            if (entityIn.isFallFlying()) {
                bodyPitch = entityIn.getXRot() + flyingPitch;
                entityTrans = flyingTrans;
            } else if (entityIn.isSleeping()) {
                bodyYaw = entityIn.getBedOrientation().toYRot() + 180.0f;
                bodyPitch = sleepingPitch;
                entityTrans = sleepingTrans;
            } else if (entityIn.isSwimming()) {
                bodyPitch = entityIn.getXRot() + swimmingPitch;
                entityTrans = swimmingTrans;
            } else if (entityIn.isVisuallyCrawling()) {
                bodyPitch = crawlingPitch;
                entityTrans = crawlingTrans;
            }
        }
        
        // ========== 렌더링 ==========
        if (KAIMyEntityClient.calledFrom(6).contains("InventoryScreen") || 
            KAIMyEntityClient.calledFrom(6).contains("class_490")) {
            renderInInventory(entityIn, model, size, bodyYaw, entityYaw, tickDelta, matrixStackIn, packedLightIn, MCinstance);
        } else {
            matrixStackIn.scale(size[0], size[0], size[0]);
            RenderSystem.setShader(GameRenderer::getRendertypeEntityTranslucentShader);
            model.Render(entityIn, bodyYaw, bodyPitch, entityTrans, tickDelta, matrixStackIn, packedLightIn);
        }
        
        // ========== 손 아이템 렌더링 (MMD만) ==========
        if (isMMD) {
            MMDModelData mmdData = (MMDModelData) m;
            renderHandItems(entityIn, mmdData, matrixStackIn, vertexConsumers, packedLightIn);
        }
        
        ci.cancel(); // 기본 렌더링 취소
    }
    
    // ========== MMD 애니메이션 처리 ==========
    
    private void handleMMDAnimation(AbstractClientPlayer entityIn, MMDModelData mwed, float tickDelta) {
        IMMDModel model = mwed.model;
        
        if (!mwed.entityData.playCustomAnim) {
            // Layer 0 - 기본 동작
            if (entityIn.getHealth() == 0.0f) {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Die, 0);
            } else if (entityIn.isFallFlying()) {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.ElytraFly, 0);
            } else if (entityIn.isSleeping()) {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Sleep, 0);
            } else if (entityIn.isPassenger()) {
                if(entityIn.getVehicle().getType() == EntityType.HORSE && 
                   (entityIn.getX() - entityIn.xo != 0.0f || entityIn.getZ() - entityIn.zo != 0.0f)){
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.OnHorse, 0);
                } else {
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Ride, 0);
                }
            } else if (entityIn.isSwimming()) {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Swim, 0);
            } else if (entityIn.onClimbable()) {
                if(entityIn.getY() - entityIn.yo > 0){
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.OnClimbableUp, 0);
                } else if(entityIn.getY() - entityIn.yo < 0){
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.OnClimbableDown, 0);
                } else {
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.OnClimbable, 0);
                }
            } else if (entityIn.isSprinting() && !entityIn.isShiftKeyDown()) {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Sprint, 0);
            } else if (entityIn.isVisuallyCrawling()){
                if(entityIn.getX() - entityIn.xo != 0.0f || entityIn.getZ() - entityIn.zo != 0.0f){
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Crawl, 0);
                } else {
                    AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.LieDown, 0);
                }
            } else if (entityIn.getX() - entityIn.xo != 0.0f || entityIn.getZ() - entityIn.zo != 0.0f) {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Walk, 0);
            } else {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Idle, 0);
            }

            // Layer 1 - 손 동작
            if(!entityIn.isUsingItem() && !entityIn.swinging || entityIn.isSleeping()){
                if (mwed.entityData.stateLayers[1] != MMDModelManager.EntityData.EntityState.Idle) {
                    mwed.entityData.stateLayers[1] = MMDModelManager.EntityData.EntityState.Idle;
                    model.ChangeAnim(0, 1);
                }
            } else {
                if((entityIn.getUsedItemHand() == InteractionHand.MAIN_HAND) && entityIn.isUsingItem()){
                    String itemId = getItemId_in_ActiveHand(entityIn, InteractionHand.MAIN_HAND);
                    CustomItemActiveAnim(mwed, MMDModelManager.EntityData.EntityState.ItemRight, itemId, "Right", "using", 1);
                } else if((entityIn.swingingArm == InteractionHand.MAIN_HAND) && entityIn.swinging){
                    String itemId = getItemId_in_ActiveHand(entityIn, InteractionHand.MAIN_HAND);
                    CustomItemActiveAnim(mwed, MMDModelManager.EntityData.EntityState.SwingRight, itemId, "Right", "swinging", 1);
                } else if((entityIn.getUsedItemHand() == InteractionHand.OFF_HAND) && entityIn.isUsingItem()){
                    String itemId = getItemId_in_ActiveHand(entityIn, InteractionHand.OFF_HAND);
                    CustomItemActiveAnim(mwed, MMDModelManager.EntityData.EntityState.ItemLeft, itemId, "Left", "using", 1);
                } else if((entityIn.swingingArm == InteractionHand.OFF_HAND) && entityIn.swinging){
                    String itemId = getItemId_in_ActiveHand(entityIn, InteractionHand.OFF_HAND);
                    CustomItemActiveAnim(mwed, MMDModelManager.EntityData.EntityState.SwingLeft, itemId, "Left", "swinging", 1);
                }
            }

            // Layer 2 - 스니크
            if (entityIn.isShiftKeyDown() && !entityIn.isVisuallyCrawling()) {
                AnimStateChangeOnce(mwed, MMDModelManager.EntityData.EntityState.Sneak, 2);
            } else {
                if (mwed.entityData.stateLayers[2] != MMDModelManager.EntityData.EntityState.Idle) {
                    mwed.entityData.stateLayers[2] = MMDModelManager.EntityData.EntityState.Idle;
                    model.ChangeAnim(0, 2);
                }
            }
        }
    }
    
    // ========== 손 아이템 렌더링 ==========
    
    private void renderHandItems(AbstractClientPlayer entityIn, MMDModelData mwed, 
                                 PoseStack matrixStackIn, MultiBufferSource vertexConsumers, int packedLightIn) {
        NativeFunc nf = NativeFunc.GetInst();
        float rotationDegree;
        
        // 오른손
        nf.GetRightHandMat(mwed.model.GetModelLong(), mwed.entityData.rightHandMat);
        matrixStackIn.pushPose();
        matrixStackIn.last().pose().mul(DataToMat(nf, mwed.entityData.rightHandMat));
        rotationDegree = ItemRotaionDegree(entityIn, mwed, InteractionHand.MAIN_HAND, "z");
        matrixStackIn.mulPose(new Quaternionf().rotateZ(rotationDegree*((float)Math.PI / 180F)));
        rotationDegree = ItemRotaionDegree(entityIn, mwed, InteractionHand.MAIN_HAND, "x");
        matrixStackIn.mulPose(new Quaternionf().rotateX(rotationDegree*((float)Math.PI / 180F)));
        matrixStackIn.scale(10.0f, 10.0f, 10.0f);
        Minecraft.getInstance().getItemRenderer().renderStatic(
            entityIn, entityIn.getMainHandItem(), ItemDisplayContext.THIRD_PERSON_RIGHT_HAND, 
            false, matrixStackIn, vertexConsumers, entityIn.level(), packedLightIn, 
            OverlayTexture.NO_OVERLAY, 0);
        matrixStackIn.popPose();

        // 왼손
        nf.GetLeftHandMat(mwed.model.GetModelLong(), mwed.entityData.leftHandMat);
        matrixStackIn.pushPose();
        matrixStackIn.last().pose().mul(DataToMat(nf, mwed.entityData.leftHandMat));
        rotationDegree = ItemRotaionDegree(entityIn, mwed, InteractionHand.OFF_HAND, "z");
        matrixStackIn.mulPose(new Quaternionf().rotateZ(rotationDegree*((float)Math.PI / 180F)));
        rotationDegree = ItemRotaionDegree(entityIn, mwed, InteractionHand.OFF_HAND, "x");
        matrixStackIn.mulPose(new Quaternionf().rotateX(rotationDegree*((float)Math.PI / 180F)));
        matrixStackIn.scale(10.0f, 10.0f, 10.0f);
        Minecraft.getInstance().getItemRenderer().renderStatic(
            entityIn, entityIn.getOffhandItem(), ItemDisplayContext.THIRD_PERSON_LEFT_HAND, 
            true, matrixStackIn, vertexConsumers, entityIn.level(), packedLightIn, 
            OverlayTexture.NO_OVERLAY, 0);
        matrixStackIn.popPose();
    }
    
    // ========== 인벤토리 렌더링 ==========
    
    private void renderInInventory(AbstractClientPlayer entityIn, IMMDModel model, float[] size,
                                   float bodyYaw, float entityYaw, float tickDelta,
                                   PoseStack matrixStackIn, int packedLightIn, Minecraft MCinstance) {
        RenderSystem.setShader(GameRenderer::getPositionTexShader);
        PoseStack PTS_modelViewStack = new PoseStack();
        PTS_modelViewStack.setIdentity();
        PTS_modelViewStack.mulPose(RenderSystem.getModelViewMatrix());
        PTS_modelViewStack.pushPose();
        
        int PosX_in_inventory;
        int PosY_in_inventory;
        
        if(MCinstance.gameMode.getPlayerMode() != GameType.CREATIVE && 
           MCinstance.screen instanceof InventoryScreen){
            PosX_in_inventory = ((InventoryScreen) MCinstance.screen)
                .getRecipeBookComponent().updateScreenPosition(MCinstance.screen.width, 176);
            PosY_in_inventory = (MCinstance.screen.height - 166) / 2;
            PTS_modelViewStack.translate(PosX_in_inventory+51, PosY_in_inventory+75, 50);
            PTS_modelViewStack.scale(1.5f, 1.5f, 1.5f);
        } else {
            PosX_in_inventory = (MCinstance.screen.width - 121) / 2;
            PosY_in_inventory = (MCinstance.screen.height - 195) / 2;
            PTS_modelViewStack.translate(PosX_in_inventory+51, PosY_in_inventory+75, 50.0);
        }
        
        PTS_modelViewStack.scale(size[1], size[1], size[1]);
        PTS_modelViewStack.scale(20.0f,20.0f, -20.0f);
        
        Quaternionf quaternionf = (new Quaternionf()).rotateZ((float)Math.PI);
        Quaternionf quaternionf1 = (new Quaternionf()).rotateX(-entityIn.getXRot() * ((float)Math.PI / 180F));
        Quaternionf quaternionf2 = (new Quaternionf()).rotateY(-bodyYaw * ((float)Math.PI / 180F));
        quaternionf.mul(quaternionf1);
        quaternionf.mul(quaternionf2);
        PTS_modelViewStack.mulPose(quaternionf);
        
        RenderSystem.setShader(GameRenderer::getRendertypeEntityTranslucentShader);
        model.Render(entityIn, entityYaw, 0.0f, new Vector3f(0.0f), tickDelta, PTS_modelViewStack, packedLightIn);
        PTS_modelViewStack.popPose();
        
        matrixStackIn.mulPose(quaternionf2);
        matrixStackIn.scale(size[1], size[1], size[1]);
        matrixStackIn.scale(0.09f, 0.09f, 0.09f);
    }
    
    // ========== 유틸리티 메서드 ==========
    
    String getItemId_in_ActiveHand(AbstractClientPlayer entityIn, InteractionHand hand) {
        String descriptionId = entityIn.getItemInHand(hand).getItem().getDescriptionId();
        String result = descriptionId.substring(descriptionId.indexOf(".") + 1);
        return result;
    }

    void AnimStateChangeOnce(MMDModelData model, MMDModelManager.EntityData.EntityState targetState, Integer layer) {
        String Property = MMDModelManager.EntityData.stateProperty.get(targetState);
        if (model.entityData.stateLayers[layer] != targetState) {
            model.entityData.stateLayers[layer] = targetState;
            model.model.ChangeAnim(MMDAnimManager.GetAnimModel(model.model, Property), layer);
        }
    }

    void CustomItemActiveAnim(MMDModelData model, MMDModelManager.EntityData.EntityState targetState, 
                             String itemName, String activeHand, String handState, Integer layer) {
        long anim = MMDAnimManager.GetAnimModel(model.model, 
            String.format("itemActive_%s_%s_%s", itemName, activeHand, handState));
        if (anim != 0) {
            if (model.entityData.stateLayers[layer] != targetState) {
                model.entityData.stateLayers[layer] = targetState;
                model.model.ChangeAnim(anim, layer);
            }
            return;
        }
        if (targetState == MMDModelManager.EntityData.EntityState.ItemRight || 
            targetState == MMDModelManager.EntityData.EntityState.SwingRight) {
            AnimStateChangeOnce(model, MMDModelManager.EntityData.EntityState.SwingRight, layer);
        } else if (targetState == MMDModelManager.EntityData.EntityState.ItemLeft || 
                   targetState == MMDModelManager.EntityData.EntityState.SwingLeft) {
            AnimStateChangeOnce(model, MMDModelManager.EntityData.EntityState.SwingLeft, layer);
        }
    }
    
    float DataToFloat(NativeFunc nf, long data, long pos) {
        int temp = 0;
        temp |= nf.ReadByte(data, pos) & 0xff;
        temp |= (nf.ReadByte(data, pos + 1) & 0xff) << 8;
        temp |= (nf.ReadByte(data, pos + 2) & 0xff) << 16;
        temp |= (nf.ReadByte(data, pos + 3) & 0xff) << 24;
        return Float.intBitsToFloat(temp);
    }
    
    Matrix4f DataToMat(NativeFunc nf, long data) {
        Matrix4f result = new Matrix4f(
            DataToFloat(nf, data, 0),DataToFloat(nf, data, 16),DataToFloat(nf, data, 32),DataToFloat(nf, data, 48),
            DataToFloat(nf, data, 4),DataToFloat(nf, data, 20),DataToFloat(nf, data, 36),DataToFloat(nf, data, 52),
            DataToFloat(nf, data, 8),DataToFloat(nf, data, 24),DataToFloat(nf, data, 40),DataToFloat(nf, data, 56),
            DataToFloat(nf, data, 12),DataToFloat(nf, data, 28),DataToFloat(nf, data, 44),DataToFloat(nf, data, 60)
        );
        result.transpose();
        return result;
    }

    float ItemRotaionDegree(AbstractClientPlayer entityIn, MMDModelData mwed, 
                           InteractionHand iHand, String axis){
        float result = 0.0f;
        String itemId = getItemId_in_ActiveHand(entityIn,iHand);
        String strHand = (iHand == InteractionHand.MAIN_HAND) ? "Right" : "Left";
        
        String handState;
        if ((iHand == entityIn.getUsedItemHand()) && (entityIn.isUsingItem())){
            handState = "using";
        } else if ((iHand == entityIn.swingingArm) && (entityIn.swinging)){
            handState = "swinging";
        } else {
            handState = "idle";
        }

        String propKey = itemId + "_" + strHand + "_" + handState + "_" + axis;
        if (mwed.properties.getProperty(propKey) != null ){
            result = Float.valueOf(mwed.properties.getProperty(propKey));
        } else if (mwed.properties.getProperty("default_" + axis) != null){
            result = Float.valueOf(mwed.properties.getProperty("default_" + axis));
        }
        
        return result;
    }

    float[] sizeOfModel(MMDModelManager.Model model){
        float[] size = new float[2];
        size[0] = getFloatProperty(model, "size", 1.0f);
        size[1] = getFloatProperty(model, "size_in_inventory", 1.0f);
        return size;
    }
    
    float getFloatProperty(MMDModelManager.Model model, String key, float defaultValue) {
        String val = model.properties.getProperty(key);
        return (val == null) ? defaultValue : Float.valueOf(val);
    }
    
    Vector3f getVec3Property(MMDModelManager.Model model, String key) {
        String val = model.properties.getProperty(key);
        return (val == null) ? new Vector3f(0.0f) : KAIMyEntityClient.str2Vec3f(val);
    }
}
