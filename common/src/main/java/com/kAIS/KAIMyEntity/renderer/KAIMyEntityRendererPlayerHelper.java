package com.kAIS.KAIMyEntity.renderer;

import net.minecraft.world.entity.player.Player;

public class KAIMyEntityRendererPlayerHelper {

    KAIMyEntityRendererPlayerHelper() {
    }

    public static void ResetPhysics(Player player) {
        MMDModelManager.Model m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
        if (m == null)
            m = MMDModelManager.GetModel("EntityPlayer");
        
        if (m != null) {
            IMMDModel model = m.model;
            
            // ✓ MMD 모델인 경우에만 애니메이션 리셋
            if (m.isMMDModel()) {
                MMDModelManager.MMDModelData mmdData = (MMDModelManager.MMDModelData) m;
                mmdData.entityData.playCustomAnim = false;
                model.ChangeAnim(MMDAnimManager.GetAnimModel(model, "idle"), 0);
                model.ChangeAnim(0, 1);
                model.ChangeAnim(0, 2);
            }
            
            // 모든 모델 타입에 대해 물리 리셋
            model.ResetPhysics();
        }
    }

    public static void CustomAnim(Player player, String id) {
        MMDModelManager.Model m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
        if (m == null)
            m = MMDModelManager.GetModel("EntityPlayer");
        
        if (m != null) {
            IMMDModel model = m.model;
            
            // ✓ MMD 모델인 경우에만 커스텀 애니메이션 재생
            if (m.isMMDModel()) {
                MMDModelManager.MMDModelData mmdData = (MMDModelManager.MMDModelData) m;
                mmdData.entityData.playCustomAnim = true;
                model.ChangeAnim(MMDAnimManager.GetAnimModel(model, "custom_" + id), 0);
                model.ChangeAnim(0, 1);
                model.ChangeAnim(0, 2);
            }
            // URDF 모델은 애니메이션 없으므로 스킵
        }
    }
}
