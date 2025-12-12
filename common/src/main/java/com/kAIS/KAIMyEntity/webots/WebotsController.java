package com.kAIS.KAIMyEntity.webots;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;

public class WebotsController {
    private static final Logger LOGGER = LogManager.getLogger();
    private static WebotsController instance;

    private final HttpClient httpClient;
    private String webotsUrl;
    private String robotIp;
    private int robotPort;
    private final ExecutorService executor;
    private final ScheduledExecutorService scheduler;
    private final BlockingQueue<Command> commandQueue;
    private final Map<String, Float> lastSentByName;
    private final Float[] lastFrame = new Float[NMOTORS];
    private static final int NMOTORS = 20;

    // 기본 델타 임계값(라디안)
    private static final float DEFAULT_DELTA_THRESHOLD = 0.0040f;

    private volatile boolean connected = false;
    private final AtomicInteger failureCount = new AtomicInteger(0);
    private static final int MAX_FAILURES = 10;

    private final Stats stats = new Stats();

    // ==================== 매핑(정정 완료): Java ↔ Webots Motor Index ====================
    private static final Map<String, JointMapping> JOINT_MAP = new HashMap<>();

    static {
        // 머리
        JOINT_MAP.put("head_pan",  new JointMapping("Neck",  18, -1.57f,  1.57f));
        JOINT_MAP.put("head_tilt", new JointMapping("Head",  19, -0.52f,  0.52f));

        // 팔
        JOINT_MAP.put("r_sho_pitch", new JointMapping("ShoulderR", 0, -1.57f,  0.52f));
        JOINT_MAP.put("l_sho_pitch", new JointMapping("ShoulderL", 1, -1.57f,  0.52f));
        JOINT_MAP.put("r_sho_roll",  new JointMapping("ArmUpperR", 2, -0.15f,  2.30f));
        JOINT_MAP.put("l_sho_roll",  new JointMapping("ArmUpperL", 3, -2.25f,  0.15f));
        JOINT_MAP.put("r_el",        new JointMapping("ArmLowerR", 4, -1.57f, -0.10f));
        JOINT_MAP.put("l_el",        new JointMapping("ArmLowerL", 5, -1.57f, -0.10f));

        // 골반/힙
        JOINT_MAP.put("r_hip_yaw",   new JointMapping("PelvYR", 6, -1.047f, 1.047f));
        JOINT_MAP.put("l_hip_yaw",   new JointMapping("PelvYL", 7, -0.69f,  2.50f));
        JOINT_MAP.put("r_hip_roll",  new JointMapping("PelvR",  8, -1.01f,  1.01f));
        JOINT_MAP.put("l_hip_roll",  new JointMapping("PelvL",  9, -0.35f,  0.35f));
        JOINT_MAP.put("r_hip_pitch", new JointMapping("LegUpperR", 10, -2.50f, 0.87f));
        JOINT_MAP.put("l_hip_pitch", new JointMapping("LegUpperL", 11, -2.50f, 0.87f));

        // 무릎
        JOINT_MAP.put("r_knee", new JointMapping("LegLowerR", 12, -0.10f, 2.09f));
        JOINT_MAP.put("l_knee", new JointMapping("LegLowerL", 13, -0.10f, 2.09f));

        // 발목/발
        JOINT_MAP.put("r_ank_pitch", new JointMapping("AnkleR", 14, -0.87f, 0.87f));
        JOINT_MAP.put("l_ank_pitch", new JointMapping("AnkleL", 15, -1.39f, 1.22f));
        JOINT_MAP.put("r_ank_roll",  new JointMapping("FootR",  16, -0.87f, 0.87f));
        JOINT_MAP.put("l_ank_roll",  new JointMapping("FootL",  17, -0.87f, 0.87f));

        // 역호환(이름 그대로 들어오는 경우)
        JOINT_MAP.put("ShoulderR", new JointMapping("ShoulderR", 0, -1.57f, 0.52f));
        JOINT_MAP.put("ShoulderL", new JointMapping("ShoulderL", 1, -1.57f, 0.52f));
        JOINT_MAP.put("ArmUpperR", new JointMapping("ArmUpperR", 2, -0.15f, 2.30f));
        JOINT_MAP.put("ArmUpperL", new JointMapping("ArmUpperL", 3, -2.25f, 0.15f));
        JOINT_MAP.put("ArmLowerR", new JointMapping("ArmLowerR", 4, -1.57f, -0.10f));
        JOINT_MAP.put("ArmLowerL", new JointMapping("ArmLowerL", 5, -1.57f, -0.10f));
        JOINT_MAP.put("PelvYR",    new JointMapping("PelvYR", 6, -1.047f, 1.047f));
        JOINT_MAP.put("PelvYL",    new JointMapping("PelvYL", 7, -0.69f,  2.50f));
        JOINT_MAP.put("PelvR",     new JointMapping("PelvR",  8, -1.01f,  1.01f));
        JOINT_MAP.put("PelvL",     new JointMapping("PelvL",  9, -0.35f,  0.35f));
        JOINT_MAP.put("LegUpperR", new JointMapping("LegUpperR", 10, -2.50f, 0.87f));
        JOINT_MAP.put("LegUpperL", new JointMapping("LegUpperL", 11, -2.50f, 0.87f));
        JOINT_MAP.put("LegLowerR", new JointMapping("LegLowerR", 12, -0.10f, 2.09f));
        JOINT_MAP.put("LegLowerL", new JointMapping("LegLowerL", 13, -0.10f, 2.09f));
        JOINT_MAP.put("AnkleR",    new JointMapping("AnkleR", 14, -0.87f, 0.87f));
        JOINT_MAP.put("AnkleL",    new JointMapping("AnkleL", 15, -1.39f, 1.22f));
        JOINT_MAP.put("FootR",     new JointMapping("FootR",  16, -0.87f, 0.87f));
        JOINT_MAP.put("FootL",     new JointMapping("FootL",  17, -0.87f, 0.87f));
        JOINT_MAP.put("Neck",      new JointMapping("Neck",   18, -1.57f, 1.57f));
        JOINT_MAP.put("Head",      new JointMapping("Head",   19, -0.52f, 0.52f));
    }

    private WebotsController(String ip, int port) {
        this.robotIp = ip;
        this.robotPort = port;
        this.webotsUrl = String.format("http://%s:%d", ip, port);

        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofMillis(400))
                .build();

        this.executor = Executors.newSingleThreadExecutor(r -> {
            Thread t = new Thread(r, "Webots-Sender");
            t.setDaemon(true);
            return t;
        });

        this.scheduler = Executors.newScheduledThreadPool(1, r -> {
            Thread t = new Thread(r, "Webots-Scheduler");
            t.setDaemon(true);
            return t;
        });

        this.commandQueue = new LinkedBlockingQueue<>();
        this.lastSentByName = new ConcurrentHashMap<>();

        // 50 Hz, 배치 드레인 + 코얼레싱
        scheduler.scheduleAtFixedRate(this::processQueue, 0, 20, TimeUnit.MILLISECONDS);
        testConnection();

        LOGGER.info("✅ WebotsController initialized: {}", webotsUrl);
    }

    public static WebotsController getInstance() {
        if (instance == null) {
            try {
                WebotsConfigScreen.Config config = WebotsConfigScreen.Config.getInstance();
                instance = new WebotsController(config.getLastIp(), config.getLastPort());
            } catch (Exception e) {
                LOGGER.warn("Failed to load config, using defaults", e);
                instance = new WebotsController("localhost", 8080);
            }
        }
        return instance;
    }

    public static WebotsController getInstance(String ip, int port) {
        if (instance != null) {
            if (!instance.robotIp.equals(ip) || instance.robotPort != port) {
                LOGGER.info("🔄 Recreating WebotsController with new address: {}:{}", ip, port);
                instance.shutdown();
                instance = new WebotsController(ip, port);
                try {
                    WebotsConfigScreen.Config config = WebotsConfigScreen.Config.getInstance();
                    config.update(ip, port);
                } catch (Exception e) {
                    LOGGER.warn("Failed to save config", e);
                }
            }
        } else {
            instance = new WebotsController(ip, port);
            try {
                WebotsConfigScreen.Config config = WebotsConfigScreen.Config.getInstance();
                config.update(ip, port);
            } catch (Exception e) {
                LOGGER.warn("Failed to save config", e);
            }
        }
        return instance;
    }

    public void reconnect(String ip, int port) {
        LOGGER.info("🔄 Reconnecting to {}:{}", ip, port);
        this.robotIp = ip;
        this.robotPort = port;
        this.webotsUrl = String.format("http://%s:%d", ip, port);
        this.failureCount.set(0);
        this.connected = false;

        commandQueue.clear();
        lastSentByName.clear();
        Arrays.fill(lastFrame, null);

        testConnection();
        try {
            WebotsConfigScreen.Config config = WebotsConfigScreen.Config.getInstance();
            config.update(ip, port);
        } catch (Exception e) {
            LOGGER.warn("Failed to save config", e);
        }
    }

    private void testConnection() {
        executor.submit(() -> {
            try {
                String url = webotsUrl + "/?command=get_stats";
                HttpRequest request = HttpRequest.newBuilder()
                        .uri(URI.create(url))
                        .timeout(Duration.ofMillis(500))
                        .GET()
                        .build();

                HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());

                if (response.statusCode() == 200) {
                    connected = true;
                    failureCount.set(0);
                    LOGGER.info("✅ Connected to Webots: {}", webotsUrl);
                } else {
                    LOGGER.warn("⚠️  Webots returned status {}", response.statusCode());
                }
            } catch (Exception e) {
                connected = false;
                LOGGER.error("❌ Failed to connect to Webots: {}", e.getMessage());
            }
        });
    }

    // 단일 관절(하위 호환)
    public void setJoint(String jointName, float urdfValue) {
        JointMapping mapping = JOINT_MAP.get(jointName);
        if (mapping == null) {
            if (stats.unknownJointWarnings.computeIfAbsent(jointName, k -> 0) < 3) {
                LOGGER.warn("Unknown joint: {} (warning {} of 3)", jointName,
                        stats.unknownJointWarnings.merge(jointName, 1, Integer::sum));
            }
            return;
        }

        float webotsValue = convertUrdfToWebots(jointName, urdfValue);

        Float last = lastSentByName.get(jointName);
        float dth = getDeltaThreshold(mapping.index);
        if (last != null && Math.abs(webotsValue - last) < dth) {
            stats.deltaSkipped++;
            return;
        }

        float clamped = clamp(webotsValue, mapping.min, mapping.max);
        if (Math.abs(clamped - webotsValue) > 0.001f) {
            stats.rangeClamped++;
        }

        if (commandQueue.offer(new Command(mapping.index, clamped))) {
            lastSentByName.put(jointName, clamped);
            stats.queued++;
        } else {
            stats.queueFull++;
        }
    }

    // 배치 프레임 전송: joints에 들어온 항목만 전송(NaN로 채워 건너뜀)
    public void sendFrame(Map<String, Float> jointsUrdf) {
        if (!connected && failureCount.get() > MAX_FAILURES) return;

        // 20개 슬롯을 NaN으로 시작(서버는 NaN은 무시)
        Float[] frame = new Float[NMOTORS];
        Arrays.fill(frame, Float.NaN);

        for (Map.Entry<String, Float> e : jointsUrdf.entrySet()) {
            String jointName = e.getKey();
            float urdfValue = e.getValue() != null ? e.getValue() : Float.NaN;
            JointMapping m = JOINT_MAP.get(jointName);
            if (m == null) continue;

            if (Float.isNaN(urdfValue)) continue;
            float v = convertUrdfToWebots(jointName, urdfValue);
            v = clamp(v, m.min, m.max);

            // 델타 임계값 적용
            Float last = lastFrame[m.index];
            float dth = getDeltaThreshold(m.index);
            if (last != null && Math.abs(v - last) < dth) continue;

            frame[m.index] = v;
        }

        // 모두 NaN이면 보낼 필요 없음
        boolean any = false;
        for (Float f : frame) { if (f != null && !Float.isNaN(f)) { any = true; break; } }
        if (!any) return;

        executor.submit(() -> sendBatch(frame));
    }

    // 큐 프로세스: 드레인 + 코얼레싱
    private void processQueue() {
        List<Command> drained = new ArrayList<>();
        commandQueue.drainTo(drained);
        if (drained.isEmpty()) return;

        Float[] lastByIndex = new Float[NMOTORS];
        for (Command c : drained) lastByIndex[c.index] = c.value;

        executor.submit(() -> {
            for (int i = 0; i < NMOTORS; i++) {
                if (lastByIndex[i] != null) {
                    sendToWebots(i, lastByIndex[i]);
                }
            }
        });
    }

    private void sendBatch(Float[] values) {
        if (!connected && failureCount.get() > MAX_FAILURES) return;

        try {
            // NaN은 문자열 "nan"으로 보내고, 서버는 이를 건너뜀
            StringBuilder sb = new StringBuilder();
            sb.append(webotsUrl).append("/?command=set_joints&v=");
            for (int i = 0; i < NMOTORS; i++) {
                if (i > 0) sb.append(',');
                Float v = values[i];
                if (v == null || Float.isNaN(v)) sb.append("nan");
                else sb.append(String.format(java.util.Locale.US, "%.5f", v));
            }

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(sb.toString()))
                    .timeout(Duration.ofMillis(120))
                    .GET()
                    .build();

            HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() == 200) {
                stats.sent += NMOTORS; // 대략 집계
                failureCount.set(0);
                for (int i = 0; i < NMOTORS; i++) {
                    Float v = values[i];
                    if (v != null && !Float.isNaN(v)) lastFrame[i] = v;
                }
                if (!connected) {
                    connected = true;
                    LOGGER.info("✅ Reconnected to Webots");
                }
            } else {
                stats.failed++;
                LOGGER.warn("⚠️  Webots returned status {}", response.statusCode());
            }
        } catch (Exception e) {
            stats.failed++;
            int failures = failureCount.incrementAndGet();

            if (failures == MAX_FAILURES) {
                connected = false;
                LOGGER.error("❌ Connection lost to Webots after {} failures", MAX_FAILURES);
            } else if (failures % 50 == 0) {
                LOGGER.warn("⚠️  Failed to send batch to Webots ({} failures): {}", failures, e.getMessage());
            }
        }
    }

    private void sendToWebots(int index, float value) {
        if (!connected && failureCount.get() > MAX_FAILURES) {
            return;
        }

        try {
            String url = String.format(java.util.Locale.US, "%s/?command=set_joint&index=%d&value=%.4f",
                    webotsUrl, index, value);

            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(Duration.ofMillis(100))
                    .GET()
                    .build();

            HttpResponse<String> response = httpClient.send(request,
                    HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() == 200) {
                stats.sent++;
                failureCount.set(0);
                lastFrame[index] = value;
                if (!connected) {
                    connected = true;
                    LOGGER.info("✅ Reconnected to Webots");
                }
            } else {
                stats.failed++;
                LOGGER.warn("⚠️  Webots returned status {}", response.statusCode());
            }

        } catch (Exception e) {
            stats.failed++;
            int failures = failureCount.incrementAndGet();

            if (failures == MAX_FAILURES) {
                connected = false;
                LOGGER.error("❌ Connection lost to Webots after {} failures", MAX_FAILURES);
            } else if (failures % 50 == 0) {
                LOGGER.warn("⚠️  Failed to send to Webots ({} failures): {}",
                        failures, e.getMessage());
            }
        }
    }

    public String getStatsJson() {
        try {
            String url = webotsUrl + "/?command=get_stats";
            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(Duration.ofMillis(200))
                    .GET()
                    .build();

            HttpResponse<String> response = httpClient.send(request,
                    HttpResponse.BodyHandlers.ofString());

            return response.body();

        } catch (Exception e) {
            return String.format("{\"error\": \"%s\"}", e.getMessage());
        }
    }

    public void printStats() {
        LOGGER.info("=== Webots Controller Stats ===");
        LOGGER.info("  Target: {}:{} {}", robotIp, robotPort, connected ? "✅" : "❌");
        LOGGER.info("  Queued: {} | Sent: {} | Failed: {}", stats.queued, stats.sent, stats.failed);
        LOGGER.info("  Delta Skipped: {} | Range Clamped: {} | Queue Full: {}",
                stats.deltaSkipped, stats.rangeClamped, stats.queueFull);
        LOGGER.info("  Queue Size: {} | Failure Count: {}", commandQueue.size(), failureCount.get());

        String serverStats = getStatsJson();
        LOGGER.info("  Server Stats: {}", serverStats);
    }

    public boolean isConnected() {
        return connected;
    }

    public String getRobotAddress() {
        return String.format("%s:%d", robotIp, robotPort);
    }

    public void shutdown() {
        LOGGER.info("🛑 Shutting down WebotsController...");
        scheduler.shutdown();
        executor.shutdown();
        try {
            if (!executor.awaitTermination(1, TimeUnit.SECONDS)) {
                executor.shutdownNow();
            }
        } catch (InterruptedException e) {
            executor.shutdownNow();
        }
        LOGGER.info("✅ WebotsController shutdown complete");
    }

    // ========== 내부 클래스 ==========
    private static class Command {
        final int index;
        final float value;
        final long timestamp;

        Command(int index, float value) {
            this.index = index;
            this.value = value;
            this.timestamp = System.currentTimeMillis();
        }
    }

    public static class JointMapping {
        public final String webotsName;
        public final int index;
        public final float min;
        public final float max;

        JointMapping(String webotsName, int index, float min, float max) {
            this.webotsName = webotsName;
            this.index = index;
            this.min = min;
            this.max = max;
        }
    }

    private static class Stats {
        long queued = 0;
        long sent = 0;
        long failed = 0;
        long deltaSkipped = 0;
        long rangeClamped = 0;
        long queueFull = 0;
        final Map<String, Integer> unknownJointWarnings = new ConcurrentHashMap<>();
    }

    private static float clamp(float value, float min, float max) {
        return Math.max(min, Math.min(max, value));
    }

    // 유틸리티
    public static String[] getSupportedJoints() {
        return JOINT_MAP.keySet().toArray(new String[0]);
    }

    public static JointMapping getJointMapping(String jointName) {
        return JOINT_MAP.get(jointName);
    }

    public static Integer getMotorIndex(String jointName) {
        JointMapping mapping = JOINT_MAP.get(jointName);
        return mapping != null ? mapping.index : null;
    }

    // 델타 임계값(관절군별)
    private float getDeltaThreshold(int index) {
        if (index == 18 || index == 19) return 0.0025f; // 머리
        if (index == 4 || index == 5) return 0.0030f;   // 팔꿈치
        if (index >= 0 && index <= 3) return 0.0035f;   // 어깨
        if (index >= 6 && index <= 17) return 0.0050f;  // 하체/발
        return DEFAULT_DELTA_THRESHOLD;
    }

    // ====================== URDF → Webots 변환기 ======================
    private float convertUrdfToWebots(String jointName, float urdfValue) {
        return switch (jointName) {
            // 팔꿈치
            case "r_el" -> map(urdfValue, 0.0f, 2.7925f, -0.10f, -1.57f);
            case "l_el" -> map(urdfValue, -2.7925f, 0.0f, -1.57f, -0.10f);
            // 무릎 (역방향)
            case "r_knee", "l_knee" -> map(urdfValue, -2.27f, 0.0f, 2.09f, -0.1f);
            // 머리
            case "head_pan"  -> clamp(urdfValue, -1.57f, 1.57f);
            case "head_tilt" -> clamp(urdfValue, -0.52f, 0.52f);
            // 기타
            case "l_ank_pitch" -> clamp(urdfValue, -1.39f, 1.22f);
            case "r_hip_yaw"   -> clamp(urdfValue, -1.047f, 1.047f);
            case "l_hip_yaw"   -> clamp(urdfValue, -0.69f, 2.50f);
            default -> urdfValue;
        };
    }

    private float map(float v, float fromLow, float fromHigh, float toLow, float toHigh) {
        if (v <= fromLow) return toLow;
        if (v >= fromHigh) return toHigh;
        return toLow + (v - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
    }
}