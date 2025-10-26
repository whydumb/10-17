package com.kAIS.KAIMyEntity.mixin;

import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.kAIS.KAIMyEntity.renderer.MMDModelManager;
import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.model.PlayerModel;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.entity.EntityRendererProvider;
import net.minecraft.client.renderer.entity.LivingEntityRenderer;
import net.minecraft.client.renderer.entity.player.PlayerRenderer;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joml.Vector3f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(PlayerRenderer.class)
public abstract class KAIMyEntityPlayerRendererMixin extends LivingEntityRenderer<AbstractClientPlayer, PlayerModel<AbstractClientPlayer>> {
    private static final Logger logger = LogManager.getLogger();

    public KAIMyEntityPlayerRendererMixin(EntityRendererProvider.Context ctx, PlayerModel<AbstractClientPlayer> model, float shadowRadius) {
        super(ctx, model, shadowRadius);
    }

    @Inject(method = {"render"}, at = @At("HEAD"), cancellable = true)
    public void render(AbstractClientPlayer entityIn, float entityYaw, float tickDelta, 
                      PoseStack matrixStackIn, MultiBufferSource vertexConsumers, int packedLightIn, CallbackInfo ci) {
        
        // 모델 가져오기
        MMDModelManager.Model m = MMDModelManager.GetModel("EntityPlayer");
        
        if (m == null) {
            // 모델 없으면 기본 렌더링
            return;
        }
        
        IMMDModel model = m.model;
        
        // URDF면 렌더링
        if (m.isURDFModel()) {
            matrixStackIn.pushPose();
            model.Render(entityIn, entityYaw, 0f, new Vector3f(0f), tickDelta, matrixStackIn, packedLightIn);
            matrixStackIn.popPose();
            ci.cancel();
            return;
        }
        
        // MMD는 기존 로직 (나중에 추가)
    }
}