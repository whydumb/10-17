package com.kAIS.KAIMyEntity.mixin;

import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.kAIS.KAIMyEntity.renderer.MMDModelManager;
import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.blaze3d.vertex.VertexConsumer;

import net.minecraft.client.model.PlayerModel;
import net.minecraft.client.player.AbstractClientPlayer;
import net.minecraft.client.renderer.MultiBufferSource;
import net.minecraft.client.renderer.RenderType;
import net.minecraft.client.renderer.entity.EntityRendererProvider;
import net.minecraft.client.renderer.entity.LivingEntityRenderer;
import net.minecraft.client.renderer.entity.player.PlayerRenderer;
import net.minecraft.client.renderer.texture.OverlayTexture;
import net.minecraft.resources.ResourceLocation;

import org.joml.Vector3f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(PlayerRenderer.class)
public abstract class KAIMyEntityPlayerRendererMixin
        extends LivingEntityRenderer<AbstractClientPlayer, PlayerModel<AbstractClientPlayer>> {

    public KAIMyEntityPlayerRendererMixin(EntityRendererProvider.Context ctx,
                                          PlayerModel<AbstractClientPlayer> model,
                                          float shadowRadius) {
        super(ctx, model, shadowRadius);
    }

    @Inject(method = "render", at = @At("HEAD"), cancellable = true)
    public void render(AbstractClientPlayer player, float entityYaw, float tickDelta,
                       PoseStack pose, MultiBufferSource buffers, int packedLight, CallbackInfo ci) {

        // 모델 조회
        MMDModelManager.Model m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
        if (m == null) m = MMDModelManager.GetModel("EntityPlayer");
        if (m == null || m.model == null) return;

        IMMDModel model = m.model;

        // 텍스처: 없으면 minecraft의 화이트 텍스처로
        ResourceLocation tex = model.getTexture();
        if (tex == null) {
            tex = ResourceLocation.parse("minecraft:textures/misc/white.png");
        }

        // NoCull 경로(뒷면 보이게)
        RenderType rt = RenderType.entityTranslucent(tex);
        VertexConsumer vc = buffers.getBuffer(rt);

        pose.pushPose();
        // URDF는 Y-up으로 통일했으므로 추가 회전 보정 불필요
        model.renderToBuffer(player, entityYaw, 0f, new Vector3f(0f, 0f, 0f),
                             tickDelta, pose, vc, packedLight, OverlayTexture.NO_OVERLAY);
        pose.popPose();

        ci.cancel(); // 우리가 직접 렌더했으니 바닐라 렌더 캔슬
    }
}
