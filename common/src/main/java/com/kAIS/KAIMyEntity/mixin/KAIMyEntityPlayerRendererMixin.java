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

import org.joml.Quaternionf;
import org.joml.Vector3f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

@Mixin(PlayerRenderer.class)
public abstract class KAIMyEntityPlayerRendererMixin
        extends LivingEntityRenderer<AbstractClientPlayer, PlayerModel<AbstractClientPlayer>> {

    private static final Logger logger = LogManager.getLogger();
    private static int renderCallCount = 0;

    public KAIMyEntityPlayerRendererMixin(EntityRendererProvider.Context ctx,
                                          PlayerModel<AbstractClientPlayer> model,
                                          float shadowRadius) {
        super(ctx, model, shadowRadius);
    }

    @Inject(method = "render", at = @At("HEAD"), cancellable = true)
    public void render(AbstractClientPlayer player, float entityYaw, float tickDelta,
                       PoseStack pose, MultiBufferSource buffers, int packedLight, CallbackInfo ci) {

        // 모델 조회 (플레이어 이름별 우선, 없으면 기본 모델)
        String playerName = player.getName().getString();
        MMDModelManager.Model m = MMDModelManager.GetModel("EntityPlayer_" + playerName);
        if (m == null) {
            m = MMDModelManager.GetModel("EntityPlayer");
        }
        
        // URDF 모델이 없으면 바닐라 렌더링 진행
        if (m == null || m.model == null) {
            return;
        }

        // 디버그 로깅 (20틱마다)
        renderCallCount++;
        if (renderCallCount % 20 == 0) {
            logger.debug("Rendering URDF model for player: " + playerName);
        }

        IMMDModel model = m.model;

        // URDF는 텍스처 없음 - 흰색 텍스처 사용 (버텍스 컬러로 색상 표현)
        ResourceLocation whiteTexture = ResourceLocation.parse("minecraft:textures/misc/white.png");
        
        // RenderType 선택 - Solid로 변경 (URDF는 불투명)
        RenderType renderType = RenderType.entitySolid(whiteTexture);
        VertexConsumer vertexConsumer = buffers.getBuffer(renderType);

        pose.pushPose();
        
        // ===== 플레이어 위치 조정 =====
        // Minecraft 플레이어 기준점은 발이므로 모델을 위로 올려줌
        pose.translate(0, 1.5f, 0);  // 플레이어 높이에 맞게 조정
        
        // ===== ROS (Z-up) → Minecraft (Y-up) 좌표계 변환 =====
        // X축 기준 -90도 회전하여 Z-up을 Y-up으로 변환
        pose.mulPose(new Quaternionf().rotateX((float)(-Math.PI / 2)));
        
        // ===== 추가 스케일 조정 (필요시) =====
        // URDF 모델이 너무 크거나 작으면 여기서 조정
        float modelScale = 1.0f;  // 필요시 조정
        if (m.properties != null && m.properties.containsKey("modelScale")) {
            try {
                modelScale = Float.parseFloat(m.properties.getProperty("modelScale"));
            } catch (NumberFormatException e) {
                // 기본값 사용
            }
        }
        pose.scale(modelScale, modelScale, modelScale);
        
        // ===== 플레이어 회전 적용 =====
        // 플레이어가 바라보는 방향으로 모델 회전
        pose.mulPose(new Quaternionf().rotateY((float)Math.toRadians(-entityYaw)));
        
        // ===== 애니메이션 오프셋 (선택사항) =====
        // 걷기, 달리기 등의 애니메이션에 따른 미세 조정
        float walkAnimOffset = 0.0f;
        if (player.walkAnimation.speed() > 0.1f) {
            walkAnimOffset = (float)Math.sin(player.walkAnimation.position() * 0.6662F) * 0.1f;
        }
        pose.translate(0, walkAnimOffset, 0);
        
        // ===== 조명 개선 =====
        // URDF 모델이 너무 어둡게 보이지 않도록 최소 밝기 보장
        int blockLight = (packedLight & 0xFFFF);
        int skyLight = (packedLight >> 16) & 0xFFFF;
        
        // 최소 밝기 설정 (0xF0 = 거의 최대 밝기)
        blockLight = Math.max(blockLight, 0xA0);  // 블록 조명 최소값
        skyLight = Math.max(skyLight, 0xA0);      // 하늘 조명 최소값
        int adjustedLight = (skyLight << 16) | blockLight;
        
        // ===== 모델 렌더링 호출 =====
        // IMMDModel 인터페이스의 renderToBuffer 메서드 호출
        if (model.getTexture() != null) {
            // 텍스처가 있는 경우 (향후 확장용)
            ResourceLocation modelTexture = model.getTexture();
            renderType = RenderType.entitySolid(modelTexture);
            vertexConsumer = buffers.getBuffer(renderType);
        }
        
        // 실제 렌더링 수행
        model.renderToBuffer(
            player,           // 엔티티
            entityYaw,        // Yaw 회전
            0f,               // Pitch (URDF는 보통 pitch 사용 안 함)
            new Vector3f(0f, 0f, 0f),  // 추가 변환 (없음)
            tickDelta,        // 틱 델타
            pose,             // PoseStack
            vertexConsumer,   // VertexConsumer
            adjustedLight,    // 조명 (수정된 값)
            OverlayTexture.NO_OVERLAY  // 오버레이 (대미지 효과 등)
        );
        
        pose.popPose();
        
        // ===== 추가 렌더링 요소 (선택사항) =====
        // 플레이어 이름표, 그림자 등은 바닐라 렌더러에서 처리되도록
        // 필요하면 여기서 super.render() 일부 호출 가능
        
        // 디버그 정보 (개발 중일 때만)
        if (renderCallCount % 100 == 0) {
            logger.info("URDF Model rendered successfully for: " + playerName);
            logger.info("  - Model directory: " + model.GetModelDir());
            logger.info("  - Adjusted lighting: block=" + Integer.toHexString(blockLight) + 
                       ", sky=" + Integer.toHexString(skyLight));
        }
        
        // 바닐라 플레이어 렌더링 취소 (우리가 직접 렌더했으므로)
        ci.cancel();
    }
}
