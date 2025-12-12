package com.kAIS.KAIMyEntity.neoforge;

import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL;
import com.kAIS.KAIMyEntity.urdf.control.PosePipeline;
import net.neoforged.api.distmarker.Dist;
import net.neoforged.bus.api.SubscribeEvent;
import net.neoforged.fml.common.EventBusSubscriber;
import net.neoforged.neoforge.client.event.ClientTickEvent;

import java.util.ArrayList;
import java.util.List;

@EventBusSubscriber(modid = "kaimyentity", value = Dist.CLIENT)
public final class ClientTickLoop {

    public static URDFModelOpenGLWithSTL renderer;
    public static final List<URDFModelOpenGLWithSTL> renderers = new ArrayList<>();

    @SubscribeEvent
    public static void onClientTick(ClientTickEvent.Post event) {
        float dt = 1.0f / 20.0f;
        PosePipeline.getInstance().onClientTick(dt, renderer, renderers);
    }

    // Webots 관련 기존 외부 호출 호환(원하면 UI가 이걸 부를 수 있음)
    public static void reconnectWebots(String ip, int port) {
        PosePipeline.getInstance().reconnectWebots(ip, port);
    }

    public static boolean isWebotsConnected() {
        return PosePipeline.getInstance().isWebotsConnected();
    }

    public static String getWebotsAddress() {
        return PosePipeline.getInstance().getWebotsAddress();
    }
}
