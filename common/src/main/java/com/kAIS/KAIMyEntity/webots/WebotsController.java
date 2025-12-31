package com.kAIS.KAIMyEntity.webots;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.net.URI;
import java.net.http.*;
import java.time.Duration;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;
import java.util.Locale;

public class WebotsController {
    private static final Logger LOGGER = LogManager.getLogger();
    private static WebotsController instance;

    private static final int NMOTORS = 20;

    private final HttpClient httpClient;
    private final ExecutorService io;
    private final ScheduledExecutorService tick50hz;

    private final BlockingQueue<Command> commandQueue = new LinkedBlockingQueue<>();
    private final AtomicReference<Float[]> realtimePendingFrame = new AtomicReference<>(null);

    private final Map<String, Float> lastSentByName = new ConcurrentHashMap<>();
    private final Float[] lastFrame = new Float[NMOTORS];

    private volatile String webotsUrl;
    private volatile String robotIp;
    private volatile int robotPort;

    private volatile boolean connected = false;
    private final AtomicInteger failureCount = new AtomicInteger(0);
    private static final int MAX_FAILURES = 10;

    private final AtomicBoolean inFlight = new AtomicBoolean(false);

    private final Stats stats = new Stats();

    private static final float DEFAULT_DELTA_THRESHOLD = 0.0040f;

    // ==================== 매핑: Java JointName ↔ Webots index ====================
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
    }

    /**
     * ✅ "방향이 반대"인 관절 목록
     * - 여기 있는 관절은 URDF->Webots 변환 후 v = -v 를 적용합니다.
     *
     * 네 증상 기준 기본값:
     *  - 머리 좌우 반대 => head_pan
     *  - (오른)어깨 위아래 반대 => r_sho_pitch (필요시 r_sho_roll도)
     *
     * 만약 r_sho_roll을 넣었는데 옆으로 벌리는 움직임이 이상해지면
     * r_sho_roll을 Set에서 빼고 r_sho_pitch만 유지해.
     */
    private static final Set<String> INVERT_SIGN_JOINTS = Set.of(
            "head_pan",
            "r_sho_pitch",
            "r_sho_roll"
    );

    private WebotsController(String ip, int port) {
        reconnectInternal(ip, port);

        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofMillis(400))
                .build();

        this.io = Executors.newSingleThreadExecutor(r -> {
            Thread t = new Thread(r, "Webots-IO");
            t.setDaemon(true);
            return t;
        });

        this.tick50hz = Executors.newScheduledThreadPool(1, r -> {
            Thread t = new Thread(r, "Webots-50Hz");
            t.setDaemon(true);
            return t;
        });

        tick50hz.scheduleAtFixedRate(this::flushOnce, 0, 20, TimeUnit.MILLISECONDS);
        testConnection();

        LOGGER.info("✅ WebotsController initialized: {}", webotsUrl);
    }

    public static WebotsController getInstance() {
        if (instance == null) {
            WebotsConfigScreen.Config cfg = WebotsConfigScreen.Config.getInstance();
            instance = new WebotsController(cfg.getLastIp(), cfg.getLastPort());
        }
        return instance;
    }

    public static WebotsController getInstance(String ip, int port) {
        if (instance == null) instance = new WebotsController(ip, port);
        else instance.reconnect(ip, port);
        return instance;
    }

    public void printStats() {
        LOGGER.info("Target: {}:{} connected={}", robotIp, robotPort, connected);
        LOGGER.info("Server Stats: {}", getStatsJson());
    }

    public void reconnect(String ip, int port) {
        LOGGER.info("🔄 Reconnecting to {}:{}", ip, port);
        reconnectInternal(ip, port);

        failureCount.set(0);
        connected = false;

        commandQueue.clear();
        lastSentByName.clear();
        Arrays.fill(lastFrame, null);
        realtimePendingFrame.set(null);

        testConnection();
    }

    private void reconnectInternal(String ip, int port) {
        this.robotIp = ip;
        this.robotPort = port;
        this.webotsUrl = String.format("http://%s:%d", ip, port);
    }

    private void testConnection() {
        io.submit(() -> {
            try {
                String url = webotsUrl + "/?command=get_stats";
                HttpRequest req = HttpRequest.newBuilder()
                        .uri(URI.create(url))
                        .timeout(Duration.ofMillis(400))
                        .GET()
                        .build();

                HttpResponse<String> res = httpClient.send(req, HttpResponse.BodyHandlers.ofString());
                if (res.statusCode() == 200) {
                    connected = true;
                    failureCount.set(0);
                    LOGGER.info("✅ Connected to Webots: {}", webotsUrl);
                } else {
                    connected = false;
                    LOGGER.warn("⚠️ Webots status {} on get_stats", res.statusCode());
                }
            } catch (Exception e) {
                connected = false;
                LOGGER.warn("❌ get_stats failed: {}", e.getMessage());
            }
        });
    }

    // -------------------- public API --------------------

    public boolean isConnected() { return connected; }
    public String getRobotAddress() { return String.format("%s:%d", robotIp, robotPort); }

    public void setJoint(String jointName, float urdfValue) {
        // ✅ 1. 정규화
        String canon = normalizeJointName(jointName);
        if (canon == null) return;

        // ✅ 2. 매핑 조회
        JointMapping m = JOINT_MAP.get(canon);
        if (m == null) {
            warnUnknownJoint(jointName, "setJoint");
            return;
        }

        // ✅ 3. URDF → Webots 변환
        float v = convertUrdfToWebots(canon, urdfValue);

        // ✅ 4. 방향(sign) 반전 (0은 그대로 유지됨)
        if (INVERT_SIGN_JOINTS.contains(canon)) {
            v = -v;
        }

        // ✅ 5. clamp
        v = clamp(v, m.min, m.max);

        // ✅ 6. 델타 스킵 체크
        Float last = lastSentByName.get(canon);
        float dth = getDeltaThreshold(m.index);
        if (last != null && Math.abs(v - last) < dth) {
            stats.deltaSkipped++;
            return;
        }

        // ✅ 7. 큐에 추가
        if (commandQueue.offer(new Command(m.index, v))) {
            lastSentByName.put(canon, v);
            stats.queued++;
        } else {
            stats.queueFull++;
        }
    }

    public void sendFrame(Map<String, Float> jointsUrdf) {
        if (jointsUrdf == null || jointsUrdf.isEmpty()) return;
        if (!connected && failureCount.get() > MAX_FAILURES) return;

        Float[] frame = new Float[NMOTORS];
        Arrays.fill(frame, Float.NaN);

        int mapped = 0, unknown = 0;

        for (var e : jointsUrdf.entrySet()) {
            String name = e.getKey();

            // ✅ 1. 정규화
            String canon = normalizeJointName(name);
            if (canon == null) continue;

            // ✅ 2. 매핑 조회
            JointMapping m = JOINT_MAP.get(canon);
            if (m == null) {
                if (unknown++ < 5) warnUnknownJoint(name, "sendFrame");
                continue;
            }

            Float uv = e.getValue();
            if (uv == null || Float.isNaN(uv)) continue;

            // ✅ 3. URDF → Webots 변환
            float v = convertUrdfToWebots(canon, uv);

            // ✅ 4. 방향(sign) 반전
            if (INVERT_SIGN_JOINTS.contains(canon)) {
                v = -v;
            }

            // ✅ 5. clamp
            v = clamp(v, m.min, m.max);

            // ✅ 6. 델타 스킵 체크
            Float last = lastFrame[m.index];
            float dth = getDeltaThreshold(m.index);
            if (last != null && Math.abs(v - last) < dth) continue;

            frame[m.index] = v;
            mapped++;
        }

        if (mapped == 0) return;
        realtimePendingFrame.set(frame);
    }

    public String getStatsJson() {
        try {
            String url = webotsUrl + "/?command=get_stats";
            HttpRequest req = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(Duration.ofMillis(200))
                    .GET().build();
            HttpResponse<String> res = httpClient.send(req, HttpResponse.BodyHandlers.ofString());
            return res.body();
        } catch (Exception e) {
            return String.format("{\"error\":\"%s\"}", e.getMessage());
        }
    }

    public void shutdown() {
        LOGGER.info("🛑 Shutting down WebotsController...");
        tick50hz.shutdownNow();
        io.shutdownNow();
        LOGGER.info("✅ WebotsController shutdown complete");
    }

    // -------------------- 50Hz flush loop --------------------

    private void flushOnce() {
        if (inFlight.get()) return;

        Float[] fromQueue = new Float[NMOTORS];
        Arrays.fill(fromQueue, Float.NaN);
        List<Command> drained = new ArrayList<>();
        commandQueue.drainTo(drained);
        for (Command c : drained) fromQueue[c.index] = c.value;

        Float[] realtime = realtimePendingFrame.getAndSet(null);

        Float[] merged = new Float[NMOTORS];
        Arrays.fill(merged, Float.NaN);

        if (realtime != null) {
            for (int i = 0; i < NMOTORS; i++) merged[i] = realtime[i];
        }
        for (int i = 0; i < NMOTORS; i++) {
            if (fromQueue[i] != null && !Float.isNaN(fromQueue[i])) merged[i] = fromQueue[i];
        }

        boolean any = false;
        for (Float v : merged) {
            if (v != null && !Float.isNaN(v)) { any = true; break; }
        }
        if (!any) return;

        inFlight.set(true);
        io.submit(() -> {
            try {
                boolean ok = sendBatchOrFallback(merged);
                if (ok) {
                    for (int i = 0; i < NMOTORS; i++) {
                        Float v = merged[i];
                        if (v != null && !Float.isNaN(v)) lastFrame[i] = v;
                    }
                }
            } finally {
                inFlight.set(false);
            }
        });
    }

    private boolean sendBatchOrFallback(Float[] values) {
        if (!connected && failureCount.get() > MAX_FAILURES) return false;

        StringBuilder sb = new StringBuilder(256);
        sb.append(webotsUrl).append("/?command=set_joints&v=");
        for (int i = 0; i < NMOTORS; i++) {
            if (i > 0) sb.append(',');
            Float v = values[i];
            if (v == null || Float.isNaN(v)) sb.append("nan");
            else sb.append(String.format(Locale.US, "%.5f", v));
        }

        try {
            HttpRequest req = HttpRequest.newBuilder()
                    .uri(URI.create(sb.toString()))
                    .timeout(Duration.ofMillis(200))
                    .GET().build();

            HttpResponse<String> res = httpClient.send(req, HttpResponse.BodyHandlers.ofString());
            if (res.statusCode() == 200) {
                stats.sent += NMOTORS;
                failureCount.set(0);
                if (!connected) connected = true;
                return true;
            }

            stats.failed++;
            failureCount.incrementAndGet();
            return fallbackPerJoint(values);

        } catch (Exception e) {
            stats.failed++;
            int f = failureCount.incrementAndGet();
            if (f >= MAX_FAILURES) connected = false;
            return fallbackPerJoint(values);
        }
    }

    private boolean fallbackPerJoint(Float[] values) {
        boolean anyOk = false;
        for (int i = 0; i < NMOTORS; i++) {
            Float v = values[i];
            if (v == null || Float.isNaN(v)) continue;
            if (sendToWebots(i, v)) anyOk = true;
        }
        return anyOk;
    }

    private boolean sendToWebots(int index, float value) {
        if (!connected && failureCount.get() > MAX_FAILURES) return false;
        try {
            String url = String.format(Locale.US, "%s/?command=set_joint&index=%d&value=%.4f",
                    webotsUrl, index, value);

            HttpRequest req = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(Duration.ofMillis(120))
                    .GET().build();

            HttpResponse<String> res = httpClient.send(req, HttpResponse.BodyHandlers.ofString());
            if (res.statusCode() == 200) {
                stats.sent++;
                failureCount.set(0);
                if (!connected) connected = true;
                return true;
            }
            stats.failed++;
            int f = failureCount.incrementAndGet();
            if (f >= MAX_FAILURES) connected = false;
            return false;

        } catch (Exception e) {
            stats.failed++;
            int f = failureCount.incrementAndGet();
            if (f >= MAX_FAILURES) connected = false;
            return false;
        }
    }

    private void warnUnknownJoint(String jointName, String where) {
        int n = stats.unknownJointWarnings.merge(jointName, 1, Integer::sum);
        if (n <= 3) LOGGER.warn("[{}] Unknown joint: {} ({} of 3)", where, jointName, n);
    }

    // ==================== 정규화 & 변환 ====================

    /**
     * Webots alias를 URDF 표준 키로 정규화
     */
    private String normalizeJointName(String jointName) {
        if (jointName == null) return null;
        String j = jointName.trim();
        if (j.isEmpty()) return null;

        return switch (j) {
            // 팔꿈치
            case "ArmLowerR" -> "r_el";
            case "ArmLowerL" -> "l_el";

            // 어깨
            case "ShoulderR" -> "r_sho_pitch";
            case "ShoulderL" -> "l_sho_pitch";
            case "ArmUpperR" -> "r_sho_roll";
            case "ArmUpperL" -> "l_sho_roll";

            // 머리
            case "Neck" -> "head_pan";
            case "Head" -> "head_tilt";

            // 하체
            case "PelvYR" -> "r_hip_yaw";
            case "PelvYL" -> "l_hip_yaw";
            case "PelvR" -> "r_hip_roll";
            case "PelvL" -> "l_hip_roll";
            case "LegUpperR" -> "r_hip_pitch";
            case "LegUpperL" -> "l_hip_pitch";
            case "LegLowerR" -> "r_knee";
            case "LegLowerL" -> "l_knee";
            case "AnkleR" -> "r_ank_pitch";
            case "AnkleL" -> "l_ank_pitch";
            case "FootR" -> "r_ank_roll";
            case "FootL" -> "l_ank_roll";

            default -> j;
        };
    }

    /**
     * URDF 관절값을 Webots 모터 포지션으로 변환
     */
    private float convertUrdfToWebots(String jointName, float urdfValue) {
        return switch (jointName) {

            // ✅ 팔꿈치: "부호가 뭐로 오든" 굽힘량(abs) 기반으로 매핑
            // - URDF가 +로 오든 -로 오든 움직이게 함
            // - 0 -> 거의 펼침(-0.10), |urdf| 커질수록 더 굽힘(-1.57)
            case "r_el", "l_el" -> {
                float u = Math.abs(urdfValue);
                yield map(u, 0.0f, 2.7925f, -0.10f, -1.57f);
            }

            // 무릎 (기존 유지)
            case "r_knee", "l_knee" -> map(urdfValue, -2.27f, 0.0f, 2.09f, -0.10f);

            // 머리/특정 관절은 clamp
            case "head_pan"  -> clamp(urdfValue, -1.57f, 1.57f);
            case "head_tilt" -> clamp(urdfValue, -0.52f, 0.52f);

            case "l_ank_pitch" -> clamp(urdfValue, -1.39f, 1.22f);
            case "r_hip_yaw"   -> clamp(urdfValue, -1.047f, 1.047f);
            case "l_hip_yaw"   -> clamp(urdfValue, -0.69f, 2.50f);

            default -> urdfValue;
        };
    }

    // -------------------- 내부 클래스/유틸 --------------------

    private static class Command {
        final int index;
        final float value;
        Command(int index, float value) { this.index = index; this.value = value; }
    }

    public static class JointMapping {
        public final String webotsName;
        public final int index;
        public final float min, max;
        public JointMapping(String webotsName, int index, float min, float max) {
            this.webotsName = webotsName; this.index = index; this.min = min; this.max = max;
        }
    }

    private static class Stats {
        long queued = 0;
        long sent = 0;
        long failed = 0;
        long deltaSkipped = 0;
        long queueFull = 0;
        final Map<String, Integer> unknownJointWarnings = new ConcurrentHashMap<>();
    }

    private static float clamp(float v, float lo, float hi) {
        return Math.max(lo, Math.min(hi, v));
    }

    private float getDeltaThreshold(int index) {
        if (index == 18 || index == 19) return 0.0025f;
        if (index == 4 || index == 5) return 0.0030f;
        if (index >= 0 && index <= 3) return 0.0035f;
        if (index >= 6 && index <= 17) return 0.0050f;
        return DEFAULT_DELTA_THRESHOLD;
    }

    private float map(float v, float fromLow, float fromHigh, float toLow, float toHigh) {
        if (v <= fromLow) return toLow;
        if (v >= fromHigh) return toHigh;
        return toLow + (v - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
    }
}
