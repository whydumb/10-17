package com.kAIS.KAIMyEntity.neoforge.register;

import com.kAIS.KAIMyEntity.renderer.KAIMyEntityRendererPlayerHelper;
import com.kAIS.KAIMyEntity.renderer.MMDModelManager;

// URDF 쪽
import com.kAIS.KAIMyEntity.neoforge.ClientTickLoop;
import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL;
import com.kAIS.KAIMyEntity.urdf.control.MotionEditorScreen;

import com.mojang.blaze3d.platform.InputConstants;
import net.minecraft.client.Minecraft;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.client.KeyMapping;
import net.minecraft.network.chat.Component;

import net.neoforged.api.distmarker.Dist;
import net.neoforged.api.distmarker.OnlyIn;
import net.neoforged.bus.api.SubscribeEvent;

import net.neoforged.neoforge.client.settings.KeyConflictContext;
import net.neoforged.neoforge.client.settings.KeyModifier;

import net.neoforged.neoforge.client.event.InputEvent;
import net.neoforged.neoforge.client.event.RegisterKeyMappingsEvent;
import net.neoforged.fml.common.EventBusSubscriber;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.lwjgl.glfw.GLFW;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Objects;

/**
 * 클라이언트 키 등록 & 처리
 * - V/B/N/M: 기존 커스텀 애니메이션 (유지)
 * - G: 모션 에디터 GUI 열기 (없으면 자동 로드 시도)
 * - Ctrl+G: URDF 리로드 + 자동 로드 시도
 * - H: 물리 리셋
 *
 * ⚠️ 별도의 /urdf load 커맨드 파일 없이, 여기서 자동 로드/주입까지 처리함.
 */
@EventBusSubscriber(value = Dist.CLIENT)
public class KAIMyEntityRegisterClient {
    static final Logger logger = LogManager.getLogger();

    // === 기존 키 + 동작 통합 ===
    static KeyMapping keyCustomAnim1 = new KeyMapping("key.customAnim1", KeyConflictContext.IN_GAME, KeyModifier.NONE, InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_V, "key.title");
    static KeyMapping keyCustomAnim2 = new KeyMapping("key.customAnim2", KeyConflictContext.IN_GAME, KeyModifier.NONE, InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_B, "key.title");
    static KeyMapping keyCustomAnim3 = new KeyMapping("key.customAnim3", KeyConflictContext.IN_GAME, KeyModifier.NONE, InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_N, "key.title");
    static KeyMapping keyCustomAnim4 = new KeyMapping("key.customAnim4", KeyConflictContext.IN_GAME, KeyModifier.NONE, InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_M, "key.title");
    static KeyMapping keyMotionGuiOrReload = new KeyMapping("key.motionGuiOrReload", KeyConflictContext.IN_GAME, KeyModifier.NONE, InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_G, "key.title");
    static KeyMapping keyResetPhysics     = new KeyMapping("key.resetPhysics",     KeyConflictContext.IN_GAME, KeyModifier.NONE, InputConstants.Type.KEYSYM, GLFW.GLFW_KEY_H, "key.title");

    // 정석: 이벤트로 키 등록
    @SubscribeEvent
    public static void onRegisterKeyMappings(RegisterKeyMappingsEvent e) {
        e.register(keyCustomAnim1);
        e.register(keyCustomAnim2);
        e.register(keyCustomAnim3);
        e.register(keyCustomAnim4);
        e.register(keyMotionGuiOrReload);
        e.register(keyResetPhysics);
        logger.info("KAIMyEntityRegisterClient: key mappings registered.");
    }

    // 구 Register()는 더 이상 필요 없지만, 외부에서 호출해도 부작용 없도록 no-op
    public static void Register() {
        logger.info("KAIMyEntityRegisterClient.Register() called (no-op). Use event-based registration instead.");
    }

    @OnlyIn(Dist.CLIENT)
    @SubscribeEvent
    public static void onKeyPressed(InputEvent.Key event) {
        Minecraft MC = Minecraft.getInstance();
        LocalPlayer player = MC.player;
        if (player == null) return;

        // ==== 기존 커스텀 애니메이션 유지 ====
        if (keyCustomAnim1.isDown()) {
            var m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
            if (m != null) {
                KAIMyEntityRendererPlayerHelper.CustomAnim(player, "1");
                net.neoforged.neoforge.network.PacketDistributor.sendToServer(
                        new com.kAIS.KAIMyEntity.neoforge.network.KAIMyEntityNetworkPack(1, player.getGameProfile(), 1));
            }
        }
        if (keyCustomAnim2.isDown()) {
            var m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
            if (m != null) {
                KAIMyEntityRendererPlayerHelper.CustomAnim(player, "2");
                net.neoforged.neoforge.network.PacketDistributor.sendToServer(
                        new com.kAIS.KAIMyEntity.neoforge.network.KAIMyEntityNetworkPack(1, player.getGameProfile(), 2));
            }
        }
        if (keyCustomAnim3.isDown()) {
            var m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
            if (m != null) {
                KAIMyEntityRendererPlayerHelper.CustomAnim(player, "3");
                net.neoforged.neoforge.network.PacketDistributor.sendToServer(
                        new com.kAIS.KAIMyEntity.neoforge.network.KAIMyEntityNetworkPack(1, player.getGameProfile(), 3));
            }
        }
        if (keyCustomAnim4.isDown()) {
            var m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
            if (m != null) {
                KAIMyEntityRendererPlayerHelper.CustomAnim(player, "4");
                net.neoforged.neoforge.network.PacketDistributor.sendToServer(
                        new com.kAIS.KAIMyEntity.neoforge.network.KAIMyEntityNetworkPack(1, player.getGameProfile(), 4));
            }
        }

        // ==== G: 모션 GUI / Ctrl+G: 리로드 (+ 자동 로드 시도) ====
        if (keyMotionGuiOrReload.consumeClick()) { // 한 번만 처리
            long win = MC.getWindow().getWindow();
            boolean ctrl = org.lwjgl.glfw.GLFW.glfwGetKey(win, GLFW.GLFW_KEY_LEFT_CONTROL)  == GLFW.GLFW_PRESS
                        || org.lwjgl.glfw.GLFW.glfwGetKey(win, GLFW.GLFW_KEY_RIGHT_CONTROL) == GLFW.GLFW_PRESS;

            if (ctrl) {
                // Ctrl+G → 기존 리로드 + 렌더러 자동 세팅 시도
                try {
                    MMDModelManager.ReloadModel();
                    MC.gui.getChat().addMessage(Component.literal("[URDF] models reloaded"));
                } catch (Throwable t) {
                    MC.gui.getChat().addMessage(Component.literal("[URDF] reload failed: " + t.getMessage()));
                }
                ensureActiveRenderer(MC); // 🔹 리로드 후 렌더러 없으면 자동 세팅 시도
            } else {
                // G → GUI 열기 (없으면 자동 세팅 시도)
                if (ClientTickLoop.renderer == null) {
                    ensureActiveRenderer(MC);
                }
                if (ClientTickLoop.renderer != null) {
                    MC.setScreen(new MotionEditorScreen(ClientTickLoop.renderer));
                } else {
                    MC.gui.getChat().addMessage(Component.literal("[URDF] No active renderer. Put a *.urdf under ./KAIMyEntity or ./config and press G again."));
                }
            }
        }

        // ==== H: 물리 리셋 ====
        if (keyResetPhysics.isDown()) {
            var m = MMDModelManager.GetModel("EntityPlayer_" + player.getName().getString());
            if (m != null) {
                KAIMyEntityRendererPlayerHelper.ResetPhysics(player);
                net.neoforged.neoforge.network.PacketDistributor.sendToServer(
                        new com.kAIS.KAIMyEntity.neoforge.network.KAIMyEntityNetworkPack(2, player.getGameProfile(), 0));
                MC.gui.getChat().addMessage(Component.literal("URDF physics reset"));
            }
        }
    }

    // =========================================================
    // 헬퍼: 렌더러 자동 세팅 (/urdf load 없이도 동작)
    // - ./KAIMyEntity, ./config/kaimyentity, ./config 에서 첫 번째 *.urdf 검색
    // - modelDir 는 urdf 파일이 있는 폴더 또는 ./meshes 하위 폴더 우선
    // =========================================================
    private static void ensureActiveRenderer(Minecraft mc) {
        if (ClientTickLoop.renderer != null) return;
        File gameDir = mc.gameDirectory;

        File[] candidates = new File[] {
                new File(gameDir, "KAIMyEntity"),
                new File(gameDir, "config/kaimyentity"),
                new File(gameDir, "config"),
                gameDir
        };

        File urdf = null;
        for (File root : candidates) {
            urdf = findFirstUrdf(root, 2); // 깊이 2까지 탐색
            if (urdf != null) break;
        }

        if (urdf == null) {
            mc.gui.getChat().addMessage(Component.literal("[URDF] No *.urdf found under ./KAIMyEntity or ./config"));
            return;
        }

        File modelDir = guessModelDir(urdf);
        if (modelDir == null || !modelDir.isDirectory()) {
            mc.gui.getChat().addMessage(Component.literal("[URDF] Guessing modelDir failed: " + (modelDir == null ? "null" : modelDir)));
            return;
        }

        URDFModelOpenGLWithSTL r = URDFModelOpenGLWithSTL.Create(urdf.getAbsolutePath(), modelDir.getAbsolutePath());
        if (r == null) {
            mc.gui.getChat().addMessage(Component.literal("[URDF] Parse failed: " + urdf.getAbsolutePath()));
            return;
        }
        ClientTickLoop.renderer = r;
        mc.gui.getChat().addMessage(Component.literal("[URDF] Active renderer set: " + urdf.getName()));
    }

    private static File findFirstUrdf(File root, int maxDepth) {
        if (root == null || !root.exists() || maxDepth < 0) return null;
        File[] list = root.listFiles();
        if (list == null) return null;
        for (File f : Objects.requireNonNull(list)) {
            if (f.isFile() && f.getName().toLowerCase().endsWith(".urdf")) return f;
        }
        for (File f : list) {
            if (f.isDirectory()) {
                File r = findFirstUrdf(f, maxDepth - 1);
                if (r != null) return r;
            }
        }
        return null;
    }

    private static File guessModelDir(File urdfFile) {
        // 우선순위: <urdf폴더>/meshes → <urdf폴더>
        File parent = urdfFile.getParentFile();
        if (parent == null) return null;
        File meshes = new File(parent, "meshes");
        if (meshes.exists() && meshes.isDirectory()) return meshes;
        return parent;
    }
}
