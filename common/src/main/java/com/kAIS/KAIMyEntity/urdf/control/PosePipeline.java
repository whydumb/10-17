package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL;
import com.kAIS.KAIMyEntity.webots.WebotsController;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.util.Collections;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;

public final class PosePipeline {
    private static final Logger LOGGER = LogManager.getLogger();
    private static volatile PosePipeline instance;

    // 옵션
    // - false: 기존 너 ClientTickLoop 순서 유지 (tickUpdate -> MotionEditorScreen.tick)
    // - true : (권장) MotionEditorScreen.tick -> tickUpdate
    private volatile boolean applyInputsBeforeModelUpdate = false;

    private volatile int statsIntervalTicks = 100; // 5초
    private int statsTickCounter = 0;

    private volatile boolean enableWebotsStats = true;
    private volatile boolean enableWebotsSend = true; // Webots 전송은 여기서만

    // ✅ VMC(VirtualMotionCapture) 입력을 URDF base 좌표계로 맞추는 프리셋 자동 적용
    private volatile boolean applyVmcBasisPresetToUrdf = true;

    private WebotsController webots;
    private boolean webotsInitialized = false;

    // MotionEditorScreen이 채워주는 frame 버퍼(재사용)
    private final Map<String, Float> frameScratch = new HashMap<>();

    // ✅ URDF 인스턴스별 1회 초기화(메모리 누수 방지 위해 WeakHashMap 기반)
    private final Set<URDFModelOpenGLWithSTL> configuredUrdf =
            Collections.newSetFromMap(new WeakHashMap<>());

    private PosePipeline() {
        initVmcNormalizerOnce();
    }

    private void initVmcNormalizerOnce() {
        VMCListenerController.VmcListener listener = VMCListenerController.VmcListener.getInstance();
        listener.setBoneNameNormalizer(original -> {
            if (original == null) return null;
            String lower = original.toLowerCase().trim();

            return switch (lower) {
                // 팔
                case "leftupperarm", "leftarm", "left_arm", "upperarm_left", "arm.l", "leftshoulder", "larm"
                        -> "LeftUpperArm";
                case "leftlowerarm", "leftforearm", "lowerarm_left", "forearm.l", "leftelbow"
                        -> "LeftLowerArm";
                case "lefthand", "hand.l", "hand_left", "left_wrist", "left_hand"
                        -> "LeftHand";

                case "rightupperarm", "rightarm", "right_arm", "upperarm_right", "arm.r", "rightshoulder", "rarm"
                        -> "RightUpperArm";
                case "rightlowerarm", "rightforearm", "lowerarm_right", "forearm.r", "rightelbow"
                        -> "RightLowerArm";
                case "righthand", "hand.r", "hand_right", "right_wrist", "right_hand"
                        -> "RightHand";

                // Chest
                case "chest", "upperchest", "spine", "spine1", "spine2", "spine3",
                        "torso", "upper_chest", "chest2"
                        -> "Chest";

                // 머리: Neck / Head 분리
                case "neck", "neck1", "neck2" -> "Neck";
                case "head" -> "Head";

                default -> original;
            };
        });
    }

    public static PosePipeline getInstance() {
        if (instance == null) {
            synchronized (PosePipeline.class) {
                if (instance == null) instance = new PosePipeline();
            }
        }
        return instance;
    }

    public void setApplyInputsBeforeModelUpdate(boolean v) { this.applyInputsBeforeModelUpdate = v; }
    public void setStatsIntervalTicks(int ticks) { this.statsIntervalTicks = Math.max(1, ticks); }
    public void setEnableWebotsStats(boolean v) { this.enableWebotsStats = v; }
    public void setEnableWebotsSend(boolean v) { this.enableWebotsSend = v; }

    /** ✅ VMC(=Unity) -> URDF(ROS) basis 프리셋 적용 on/off */
    public void setApplyVmcBasisPresetToUrdf(boolean v) { this.applyVmcBasisPresetToUrdf = v; }

    public void onClientTick(float dt, URDFModelOpenGLWithSTL single, List<URDFModelOpenGLWithSTL> many) {
        // 중복 인스턴스 제거(single + list 겹침 방지)
        Map<URDFModelOpenGLWithSTL, Boolean> uniq = new IdentityHashMap<>();
        if (single != null) uniq.put(single, Boolean.TRUE);
        if (many != null) for (URDFModelOpenGLWithSTL r : many) if (r != null) uniq.put(r, Boolean.TRUE);

        for (URDFModelOpenGLWithSTL urdf : uniq.keySet()) {
            tickOne(dt, urdf);
        }

        if (enableWebotsStats && ++statsTickCounter >= statsIntervalTicks) {
            statsTickCounter = 0;
            printWebotsStats();
        }
    }

    private void tickOne(float dt, URDFModelOpenGLWithSTL urdf) {
        if (urdf == null) return;

        // ✅ URDF 인스턴스당 1회만 VMC basis 프리셋 적용
        if (applyVmcBasisPresetToUrdf) {
            boolean firstTime;
            synchronized (configuredUrdf) {
                firstTime = configuredUrdf.add(urdf);
            }
            if (firstTime) {
                urdf.setTrackingBasisPreset_VMC_UnityToROS();
                LOGGER.info("[PosePipeline] Applied VMC(Unity)->URDF(ROS) basis preset to URDF instance#{}",
                        System.identityHashCode(urdf));
            }
        }

        frameScratch.clear();

        if (applyInputsBeforeModelUpdate) {
            MotionEditorScreen.tick(urdf, frameScratch);
            urdf.tickUpdate(dt);
        } else {
            urdf.tickUpdate(dt);
            MotionEditorScreen.tick(urdf, frameScratch);
        }

        if (enableWebotsSend && !frameScratch.isEmpty()) {
            WebotsController wc = getWebots();
            if (wc != null) {
                // ✅ connected 여부와 상관없이 전송 시도하게 해야 "초기 false"에서 영구 차단이 안 걸림
                wc.sendFrame(frameScratch);
            }
        }
    }

    private WebotsController getWebots() {
        if (!webotsInitialized) {
            webotsInitialized = true;
            try {
                webots = WebotsController.getInstance();
            } catch (Exception e) {
                webots = null;
                LOGGER.debug("WebotsController init failed (ignored): {}", e.getMessage());
            }
        }
        return webots;
    }

    private void printWebotsStats() {
        WebotsController wc = getWebots();
        if (wc != null && wc.isConnected()) wc.printStats();
    }

    // ClientTickLoop에서 기존에 제공하던 API 호환용
    public void reconnectWebots(String ip, int port) {
        WebotsController wc = getWebots();
        if (wc != null) wc.reconnect(ip, port);
        else {
            try {
                webots = WebotsController.getInstance(ip, port);
                webotsInitialized = true;
            } catch (Exception ignored) {}
        }
    }

    public boolean isWebotsConnected() {
        WebotsController wc = getWebots();
        return wc != null && wc.isConnected();
    }

    public String getWebotsAddress() {
        WebotsController wc = getWebots();
        return wc != null ? wc.getRobotAddress() : "Not initialized";
    }
}
