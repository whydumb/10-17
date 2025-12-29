package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL;
import com.kAIS.KAIMyEntity.urdf.vmd.VMDLoader;
import net.minecraft.client.Minecraft;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * - VMC 입력 -> URDF 관절값 계산/적용: VmcDrive
 * - VMD 로드/재생 -> URDF 관절값 적용: VMDPlayer
 *
 * Webots 전송은 PosePipeline에서만 수행 (중복 제거)
 */
public final class MotionEditorScreen {
    private static final Logger logger = LogManager.getLogger();
    private static final Map<String, Float> vmcScratch = new HashMap<>();

    private MotionEditorScreen() {}

    // =========================
    //  VMC GUI 진입점
    // =========================
    public static void open(URDFModelOpenGLWithSTL renderer) { 
        open(renderer, 39539); 
    }

    public static void open(URDFModelOpenGLWithSTL renderer, int vmcPort) {
        VMCListenerController.VmcListener listener = VMCListenerController.VmcListener.getInstance();
        listener.start("0.0.0.0", vmcPort);
        Minecraft.getInstance().setScreen(new VMCListenerController(Minecraft.getInstance().screen, renderer));
    }

    // =========================
    //  통합 Tick (VMD + VMC)
    // =========================

    /** 기존 호환: VMC만 tick */
    public static void tick(URDFModelOpenGLWithSTL renderer) {
        VmcDrive.tick(renderer, null);
    }

    /** PosePipeline용 (dt=1/20): VMD 베이스 + VMC 덮어쓰기 */
    public static void tick(URDFModelOpenGLWithSTL renderer, Map<String, Float> outFrame) {
        tick(renderer, outFrame, 1f / 20f);
    }

    /** 
     * PosePipeline용 메인 진입점
     * 1) VMD를 outFrame에 기본으로 채움
     * 2) VMC를 별도 맵에 받아 outFrame에 merge (VMC 우선권)
     */
    public static void tick(URDFModelOpenGLWithSTL renderer, Map<String, Float> outFrame, float dt) {
        if (renderer == null) return;

        if (outFrame == null) {
            // 기존 직접 적용 모드 (렌더 preview에 즉시 반영)
            VmcDrive.tick(renderer, null);
            VMDPlayer.getInstance().tick(renderer, dt, true, false);
            return;
        }

        // 1) VMD 베이스 레이어 (playing 상태일 때만)
        VMDPlayer.getInstance().sampleTo(outFrame, dt);

        // 2) VMC 오버라이드 레이어 (항상 우선권)
        vmcScratch.clear();
        VmcDrive.tick(renderer, vmcScratch);
        if (!vmcScratch.isEmpty()) {
            outFrame.putAll(vmcScratch);
            
            // 디버깅용
            if (logger.isDebugEnabled() && Math.random() < 0.05) { // 5% 확률로 로깅
                logger.debug("VMC override: {} joints", vmcScratch.size());
            }
        }
    }

    // =========================
    //  VMD Tick (개별 사용)
    // =========================

    /** VMDPlayer 직접 접근용 */
    public static VMDPlayer vmd() {
        return VMDPlayer.getInstance();
    }

    /** VMD만 직접 렌더에 적용 (레거시 호환) */
    public static void tickVmd(URDFModelOpenGLWithSTL renderer) {
        if (renderer == null) return;
        VMDPlayer.getInstance().tick(renderer, 1f / 20f, true, false);
    }

    public static void tickVmd(URDFModelOpenGLWithSTL renderer, float deltaTime, 
                              boolean applyPreview, boolean applyTarget) {
        if (renderer == null) return;
        VMDPlayer.getInstance().tick(renderer, deltaTime, applyPreview, applyTarget);
    }

    // =========================
    //  VMD 플레이어
    // =========================
    public static final class VMDPlayer {
        private static final Logger logger = LogManager.getLogger();
        private static volatile VMDPlayer instance;

        private volatile boolean playing = false;
        private volatile URDFMotion currentMotion = null;

        private float currentTime = 0f;
        private int activeJointCount = 0;
        private int debugCounter = 0;

        private VMDPlayer() {}

        public static VMDPlayer getInstance() {
            if (instance == null) {
                synchronized (VMDPlayer.class) {
                    if (instance == null) instance = new VMDPlayer();
                }
            }
            return instance;
        }

        public boolean hasMotion() { return currentMotion != null; }
        public boolean isPlaying() { return playing; }

        public void loadMotion(URDFMotion motion) {
            currentMotion = motion;
            currentTime = 0f;
            playing = false;

            if (motion == null) {
                logger.warn("VMD Motion cleared (null).");
                return;
            }
            int keys = (motion.keys == null) ? 0 : motion.keys.size();
            logger.info("VMD Motion loaded: {} ({} keyframes)", motion.name, keys);
        }

        public void loadFromFile(File vmdFile) {
            if (vmdFile == null) {
                logger.warn("loadFromFile called with null file");
                return;
            }
            URDFMotion motion = VMDLoader.load(vmdFile);
            loadMotion(motion);
        }

        public void play() {
            if (currentMotion != null) {
                playing = true;
                logger.info("VMD Playback started");
            }
        }

        public void pause() { 
            playing = false; 
        }

        public void stop() {
            playing = false;
            currentTime = 0f;
        }

        /**
         * 현재 시간의 VMD 포즈를 outFrame에 샘플링 (렌더 직접 적용 없이)
         * PosePipeline용 - VMD 베이스 레이어 제공
         */
        public void sampleTo(Map<String, Float> outFrame, float deltaTime) {
            if (outFrame == null) return;
            if (!playing) return;

            URDFMotion motion = currentMotion;
            if (motion == null || motion.keys == null || motion.keys.isEmpty()) return;

            currentTime += deltaTime;

            float maxTime = motion.keys.get(motion.keys.size() - 1).t;
            if (maxTime <= 0f) maxTime = 1f;

            if (motion.loop && currentTime > maxTime) {
                currentTime = currentTime % maxTime;
            } else if (!motion.loop && currentTime > maxTime) {
                playing = false;
                return;
            }

            URDFMotion.Key prevKey = null, nextKey = null;
            for (URDFMotion.Key key : motion.keys) {
                if (key.t <= currentTime) prevKey = key;
                else { nextKey = key; break; }
            }
            if (prevKey == null) prevKey = motion.keys.get(0);
            if (prevKey.pose == null || prevKey.pose.isEmpty()) return;

            float alpha = 0f;
            if (nextKey != null && nextKey.t > prevKey.t) {
                alpha = (currentTime - prevKey.t) / (nextKey.t - prevKey.t);
                if ("cubic".equals(prevKey.interp)) {
                    alpha = alpha * alpha * (3f - 2f * alpha);
                }
            }

            int sampleCount = 0;
            for (var entry : prevKey.pose.entrySet()) {
                String jointName = entry.getKey();
                Float base = entry.getValue();
                if (jointName == null || base == null) continue;

                float value = base;
                if (nextKey != null && nextKey.pose != null) {
                    Float nv = nextKey.pose.get(jointName);
                    if (nv != null) value = lerp(value, nv, alpha);
                }
                outFrame.put(jointName, value);
                sampleCount++;
            }

            activeJointCount = sampleCount;
            
            if (++debugCounter >= 20) {
                debugCounter = 0;
                logger.debug("VMD sample: t={}/{}, joints={}", currentTime, maxTime, sampleCount);
            }
        }

        /**
         * 기존 렌더 직접 적용 방식 (레거시 호환)
         */
        public void tick(URDFModelOpenGLWithSTL renderer, float deltaTime) {
            tick(renderer, deltaTime, true, false);
        }

        /**
         * @param applyPreview renderer.setJointPreview 호출 여부
         * @param applyTarget  renderer.setJointTarget 호출 여부
         */
        public void tick(URDFModelOpenGLWithSTL renderer,
                         float deltaTime,
                         boolean applyPreview,
                         boolean applyTarget) {
            if (!playing) return;
            if (renderer == null) return;

            URDFMotion motion = currentMotion;
            if (motion == null || motion.keys == null || motion.keys.isEmpty()) return;

            currentTime += deltaTime;

            float maxTime = motion.keys.get(motion.keys.size() - 1).t;
            if (maxTime <= 0f) maxTime = 1f;

            if (motion.loop && currentTime > maxTime) {
                currentTime = currentTime % maxTime;
            } else if (!motion.loop && currentTime > maxTime) {
                playing = false;
                return;
            }

            URDFMotion.Key prevKey = null;
            URDFMotion.Key nextKey = null;

            for (URDFMotion.Key key : motion.keys) {
                if (key.t <= currentTime) prevKey = key;
                else { nextKey = key; break; }
            }
            if (prevKey == null) prevKey = motion.keys.get(0);
            if (prevKey.pose == null || prevKey.pose.isEmpty()) return;

            float alpha = 0f;
            if (nextKey != null && nextKey.t > prevKey.t) {
                alpha = (currentTime - prevKey.t) / (nextKey.t - prevKey.t);
                if ("cubic".equals(prevKey.interp)) {
                    alpha = alpha * alpha * (3f - 2f * alpha);
                }
            }

            activeJointCount = 0;

            for (Map.Entry<String, Float> entry : prevKey.pose.entrySet()) {
                String jointName = entry.getKey();
                Float base = entry.getValue();
                if (jointName == null || base == null) continue;

                float value = base;

                if (nextKey != null && nextKey.pose != null) {
                    Float nv = nextKey.pose.get(jointName);
                    if (nv != null) value = lerp(value, nv, alpha);
                }

                if (applyPreview) {
                    renderer.setJointPreview(jointName, value);
                }
                if (applyTarget) {
                    renderer.setJointTarget(jointName, value);
                }

                activeJointCount++;
            }

            if (++debugCounter >= 20) {
                debugCounter = 0;
                logger.debug("VMD tick: t={}/{}, joints={}", currentTime, maxTime, activeJointCount);
            }
        }

        private float lerp(float a, float b, float t) {
            return a + (b - a) * t;
        }

        public Status getStatus() {
            URDFMotion motion = currentMotion;
            if (motion == null || motion.keys == null) {
                return new Status(null, 0, 0f, 0f, playing, 0);
            }
            float duration = motion.keys.isEmpty() ? 0f : motion.keys.get(motion.keys.size() - 1).t;
            return new Status(motion.name, motion.keys.size(), duration, currentTime, playing, activeJointCount);
        }

        public record Status(String motionName,
                             int keyframeCount,
                             float duration,
                             float currentTime,
                             boolean playing,
                             int activeJoints) {}
    }
}
