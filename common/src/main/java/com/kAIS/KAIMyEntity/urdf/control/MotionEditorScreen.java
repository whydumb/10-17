package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL;
import net.minecraft.client.Minecraft;

import java.util.Map;

/**
 * VMC 입력 -> URDF 관절값 계산/적용
 * Webots 전송은 PosePipeline에서만 수행 (중복 제거)
 */
public final class MotionEditorScreen {
    private MotionEditorScreen() {}



    public static void open(URDFModelOpenGLWithSTL renderer) { open(renderer, 39539); }

    public static void open(URDFModelOpenGLWithSTL renderer, int vmcPort) {
        VMCListenerController.VmcListener listener = VMCListenerController.VmcListener.getInstance();
        listener.start("0.0.0.0", vmcPort);
        Minecraft.getInstance().setScreen(new VMCListenerController(Minecraft.getInstance().screen, renderer));
    }

    /** 기존 호출 호환 */
    public static void tick(URDFModelOpenGLWithSTL renderer) {
        VmcDrive.tick(renderer, null);
    }

    /** PosePipeline용: outFrame에 "이번 틱에 계산된 관절값"만 채워줌 */
    public static void tick(URDFModelOpenGLWithSTL renderer, Map<String, Float> outFrame) {
        VmcDrive.tick(renderer, outFrame);
    }
}