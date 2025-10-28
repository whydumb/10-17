package com.kAIS.KAIMyEntity.neoforge.register;

import com.kAIS.KAIMyEntity.neoforge.ClientTickLoop;
import com.kAIS.KAIMyEntity.urdf.control.MotionEditorScreen;

import net.minecraft.client.Minecraft;
import net.minecraft.commands.Commands;
import net.minecraft.network.chat.Component;

import net.neoforged.api.distmarker.Dist;
import net.neoforged.bus.api.SubscribeEvent;
import net.neoforged.fml.common.EventBusSubscriber;
import net.neoforged.neoforge.client.event.RegisterClientCommandsEvent;

/**
 * /motion gui : 인게임 모션 에디터 열기
 */
@EventBusSubscriber(modid = "kaimyentity", value = Dist.CLIENT)
public final class OpenMotionGuiCommand {
    @SubscribeEvent
    public static void onRegisterClientCommands(RegisterClientCommandsEvent e) {
        e.getDispatcher().register(
            Commands.literal("motion")
                .then(Commands.literal("gui").executes(ctx -> {
                    if (ClientTickLoop.renderer != null) {
                        Minecraft.getInstance().setScreen(new MotionEditorScreen(ClientTickLoop.renderer));
                        return 1;
                    } else {
                        if (Minecraft.getInstance().player != null) {
                            Minecraft.getInstance().player.sendSystemMessage(
                                Component.literal("[URDF] No active renderer. Set ClientTickLoop.renderer first.")
                            );
                        }
                        return 0;
                    }
                }))
        );
    }
}
