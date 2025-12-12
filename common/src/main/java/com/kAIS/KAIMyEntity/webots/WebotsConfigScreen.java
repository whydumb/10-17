// common/src/main/java/com/kAIS/KAIMyEntity/webots/WebotsConfigScreen.java
package com.kAIS.KAIMyEntity.webots;

import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.Button;
import net.minecraft.client.gui.components.EditBox;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.io.*;
import java.util.Properties;
import java.util.concurrent.CompletableFuture;

/**
 * Webots 연결 설정 GUI + 설정 관리
 * - IP 주소 변경
 * - 포트 변경
 * - 연결 테스트
 * - 통계 확인
 * - 설정 자동 저장/로드
 *
 * NOTE:
 *  - render()에서 네트워크 호출(getStatsJson)하면 프레임 드랍/멈춤이 발생할 수 있어
 *    tick()에서 주기적으로 비동기 fetch 후 캐시된 문자열만 렌더링함.
 *  - Test T-Pose는 isConnected()로 막지 않음(연결 판정이 늦거나 false인 상태에서도 send 시도 가능).
 */
public class WebotsConfigScreen extends Screen {
    private static final Logger LOGGER = LogManager.getLogger();

    // UI 색상
    private static final int BG_COLOR = 0xFF0E0E10;
    private static final int PANEL_COLOR = 0xFF1D1F24;
    private static final int TITLE_COLOR = 0xFFFFD770;
    private static final int TEXT_COLOR = 0xFFFFFFFF;
    private static final int CONNECTED_COLOR = 0xFF55FF55;
    private static final int DISCONNECTED_COLOR = 0xFFFF5555;

    private final Screen parent;
    private WebotsController controller;

    // UI 컴포넌트
    private EditBox ipBox;
    private EditBox portBox;
    private Button connectButton;
    private Button testButton;
    private Button closeButton;

    private String statusMessage = "";
    private int statusColor = TEXT_COLOR;

    // 주기적 업데이트
    private int autoRefreshTicker = 0;

    // 서버 통계 캐시(렌더 중 네트워크 호출 방지)
    private String cachedServerStatsJson = "";
    private boolean statsFetchInProgress = false;
    private int statsRefreshTicker = 0;

    public WebotsConfigScreen(Screen parent) {
        super(Component.literal("Webots Connection Settings"));
        this.parent = parent;

        try {
            this.controller = WebotsController.getInstance();
        } catch (Exception e) {
            LOGGER.warn("WebotsController not initialized yet", e);
            this.controller = null;
        }
    }

    @Override
    protected void init() {
        super.init();

        int centerX = this.width / 2;
        int startY = 80;

        // === IP 주소 입력 ===
        this.ipBox = new EditBox(this.font, centerX - 100, startY, 200, 20,
                Component.literal("IP Address"));

        // ✅ Config에서 마지막 저장된 IP 로드
        Config config = Config.getInstance();
        this.ipBox.setValue(config.getLastIp());
        this.ipBox.setMaxLength(50);
        addRenderableWidget(this.ipBox);

        startY += 30;

        // === 포트 입력 ===
        this.portBox = new EditBox(this.font, centerX - 100, startY, 200, 20,
                Component.literal("Port"));

        // ✅ Config에서 마지막 저장된 Port 로드
        this.portBox.setValue(String.valueOf(config.getLastPort()));
        this.portBox.setMaxLength(5);
        addRenderableWidget(this.portBox);

        startY += 35;

        // === 연결/재연결 버튼 ===
        this.connectButton = Button.builder(Component.literal("Connect / Reconnect"), b -> handleConnect())
                .bounds(centerX - 100, startY, 200, 20)
                .build();
        addRenderableWidget(this.connectButton);

        startY += 25;

        // === T-Pose 테스트 버튼 ===
        // NOTE: connected 체크로 막지 않음(전송 시도 자체는 가능)
        this.testButton = Button.builder(Component.literal("Test T-Pose (Force Send)"), b -> handleTest())
                .bounds(centerX - 100, startY, 200, 20)
                .build();
        addRenderableWidget(this.testButton);

        // === 닫기 버튼 ===
        this.closeButton = Button.builder(Component.literal("Close"), b -> Minecraft.getInstance().setScreen(parent))
                .bounds(centerX - 50, this.height - 30, 100, 20)
                .build();
        addRenderableWidget(this.closeButton);

        updateButtonStates();
    }

    private void handleConnect() {
        String ip = ipBox.getValue().trim();
        int port;

        try {
            port = Integer.parseInt(portBox.getValue().trim());
            if (port < 1 || port > 65535) {
                setStatus("Invalid port number (1-65535)", DISCONNECTED_COLOR);
                return;
            }
        } catch (NumberFormatException e) {
            setStatus("Invalid port format", DISCONNECTED_COLOR);
            return;
        }

        if (ip.isEmpty()) {
            setStatus("IP address cannot be empty", DISCONNECTED_COLOR);
            return;
        }

        try {
            // 인스턴스 생성/재생성(주소 변경 반영)
            controller = WebotsController.getInstance(ip, port);

            // ✅ Config에 저장
            Config.getInstance().update(ip, port);

            // testConnection()은 비동기라 즉시 connected=true를 단정하면 안 됨
            setStatus("Connecting to " + ip + ":" + port + " ...", TEXT_COLOR);
            LOGGER.info("Webots connect requested: {}:{}", ip, port);

            // 서버 통계 캐시 초기화
            cachedServerStatsJson = "";
            statsRefreshTicker = 0;

        } catch (Exception e) {
            setStatus("Connection failed: " + e.getMessage(), DISCONNECTED_COLOR);
            LOGGER.error("Failed to connect to Webots", e);
        }
    }

    private void handleTest() {
        if (controller == null) {
            setStatus("Not initialized. Click 'Connect' first.", DISCONNECTED_COLOR);
            return;
        }

        try {
            // T-Pose 자세 전송(큐/배치 전송 로직은 WebotsController가 처리)
            controller.setJoint("r_sho_pitch", 0.3f);
            controller.setJoint("r_sho_roll", 1.57f);
            controller.setJoint("r_el", -0.1f);

            controller.setJoint("l_sho_pitch", 0.3f);
            controller.setJoint("l_sho_roll", -1.57f);
            controller.setJoint("l_el", -0.1f);

            // 연결 여부와 관계없이 "전송 시도"는 했음
            boolean connectedNow = controller.isConnected();
            setStatus(connectedNow
                            ? "T-Pose sent! Check Webots simulation."
                            : "T-Pose queued/sent attempt (currently DISCONNECTED). Check server/logs.",
                    connectedNow ? CONNECTED_COLOR : TEXT_COLOR);

            LOGGER.info("T-Pose test queued/sent attempt (connected={})", connectedNow);

        } catch (Exception e) {
            setStatus("Test failed: " + e.getMessage(), DISCONNECTED_COLOR);
            LOGGER.error("T-Pose test failed", e);
        }
    }

    private void setStatus(String message, int color) {
        this.statusMessage = message;
        this.statusColor = color;
    }

    private void updateButtonStates() {
        boolean hasController = (controller != null);
        // Test는 연결여부와 무관하게 활성화(전송 시도 가능)
        testButton.active = hasController;
    }

    @Override
    public void render(GuiGraphics graphics, int mouseX, int mouseY, float partialTicks) {
        // 배경
        graphics.fill(0, 0, this.width, this.height, BG_COLOR);

        // 메인 패널
        int panelX = this.width / 2 - 250;
        int panelY = 50;
        int panelW = 500;
        int panelH = this.height - 100;
        graphics.fill(panelX, panelY, panelX + panelW, panelY + panelH, PANEL_COLOR);

        super.render(graphics, mouseX, mouseY, partialTicks);

        // 제목
        graphics.pose().pushPose();
        graphics.pose().translate(0, 0, 1000.0f);
        graphics.drawCenteredString(this.font, "Webots Connection Settings",
                this.width / 2, 20, TITLE_COLOR);

        // 라벨
        graphics.drawString(this.font, "IP Address:", this.width / 2 - 100, 68, TEXT_COLOR, false);
        graphics.drawString(this.font, "Port:", this.width / 2 - 100, 98, TEXT_COLOR, false);

        // 연결 상태
        int statusY = 145;
        if (controller != null) {
            boolean connected = controller.isConnected();
            String connStatus = connected ? "§a● CONNECTED" : "§c● DISCONNECTED";
            graphics.drawCenteredString(this.font, connStatus, this.width / 2, statusY,
                    connected ? CONNECTED_COLOR : DISCONNECTED_COLOR);

            String address = "Address: " + controller.getRobotAddress();
            graphics.drawCenteredString(this.font, address, this.width / 2,
                    statusY + 12, TEXT_COLOR);

        } else {
            graphics.drawCenteredString(this.font, "§c● NOT INITIALIZED",
                    this.width / 2, statusY, DISCONNECTED_COLOR);
        }

        // 상태 메시지
        if (!statusMessage.isEmpty()) {
            graphics.drawCenteredString(this.font, statusMessage,
                    this.width / 2, statusY + 30, statusColor);
        }

        // 통계 패널(캐시된 값만 표시)
        int statsY = statusY + 55;
        graphics.drawString(this.font, "=== Statistics ===",
                panelX + 20, statsY, TITLE_COLOR, false);

        if (controller == null) {
            graphics.drawString(this.font, "Server: (not initialized)",
                    panelX + 20, statsY + 15, DISCONNECTED_COLOR, false);
        } else {
            boolean connected = controller.isConnected();
            if (!connected) {
                graphics.drawString(this.font, "Server: (disconnected) - you can still Force Send T-Pose",
                        panelX + 20, statsY + 15, DISCONNECTED_COLOR, false);
            } else {
                // connected면 최근 stats 캐시 표시
                String s = cachedServerStatsJson;
                if (s == null || s.isEmpty()) {
                    graphics.drawString(this.font, "Server: (fetching stats...)",
                            panelX + 20, statsY + 15, TEXT_COLOR, false);
                } else if (s.contains("error")) {
                    graphics.drawString(this.font, "Server: " + s,
                            panelX + 20, statsY + 15, DISCONNECTED_COLOR, false);
                } else {
                    graphics.drawString(this.font, "Server: OK",
                            panelX + 20, statsY + 15, CONNECTED_COLOR, false);
                }
            }
        }

        graphics.pose().popPose();

        // 주기적 버튼 업데이트
        if (++autoRefreshTicker >= 20) {
            autoRefreshTicker = 0;
            updateButtonStates();
        }
    }

    @Override
    public void tick() {
        super.tick();

        // stats는 render에서 호출하면 렉 걸리니 tick에서 주기적으로 비동기 갱신
        if (controller != null && controller.isConnected()) {
            if (++statsRefreshTicker >= 20) { // 1초마다
                statsRefreshTicker = 0;

                if (!statsFetchInProgress) {
                    statsFetchInProgress = true;
                    CompletableFuture
                            .supplyAsync(() -> {
                                try {
                                    return controller.getStatsJson();
                                } catch (Throwable t) {
                                    return "{\"error\":\"" + t.getMessage() + "\"}";
                                }
                            })
                            .whenComplete((res, ex) -> {
                                if (ex != null) {
                                    cachedServerStatsJson = "{\"error\":\"" + ex.getMessage() + "\"}";
                                } else {
                                    cachedServerStatsJson = (res != null) ? res : "";
                                }
                                statsFetchInProgress = false;
                            });
                }
            }
        } else {
            // 연결이 아니면 캐시를 너무 자주 유지할 필요는 없지만, UI 혼동 줄이려면 비워둠
            cachedServerStatsJson = "";
            statsFetchInProgress = false;
            statsRefreshTicker = 0;
        }
    }

    @Override
    public void onClose() {
        Minecraft.getInstance().setScreen(parent);
    }

    @Override
    public boolean isPauseScreen() {
        return false;
    }

    // ========================================================================
    // ✅ 내부 Config 클래스
    // ========================================================================
    public static class Config {
        private static final Logger CONFIG_LOGGER = LogManager.getLogger();

        // 기본값
        private static final String DEFAULT_IP = "localhost";
        private static final int DEFAULT_PORT = 8080;

        // 현재 설정값
        private String lastIp;
        private int lastPort;

        // 설정 파일 경로
        private final File configFile;

        // 싱글톤
        private static Config instance;

        private Config() {
            File gameDirectory = Minecraft.getInstance().gameDirectory;
            File configDir = new File(gameDirectory, "config");
            if (!configDir.exists()) {
                //noinspection ResultOfMethodCallIgnored
                configDir.mkdirs();
            }
            this.configFile = new File(configDir, "webots_connection.properties");

            // 설정 로드
            load();
        }

        public static Config getInstance() {
            if (instance == null) {
                instance = new Config();
            }
            return instance;
        }

        private void load() {
            if (!configFile.exists()) {
                lastIp = DEFAULT_IP;
                lastPort = DEFAULT_PORT;
                save();
                CONFIG_LOGGER.info("Created default Webots config: {}:{}", lastIp, lastPort);
                return;
            }

            Properties props = new Properties();
            try (FileInputStream fis = new FileInputStream(configFile)) {
                props.load(fis);
                lastIp = props.getProperty("ip", DEFAULT_IP);
                lastPort = Integer.parseInt(props.getProperty("port", String.valueOf(DEFAULT_PORT)));
                CONFIG_LOGGER.info("Loaded Webots config: {}:{}", lastIp, lastPort);
            } catch (Exception e) {
                CONFIG_LOGGER.warn("Failed to load Webots config, using defaults", e);
                lastIp = DEFAULT_IP;
                lastPort = DEFAULT_PORT;
            }
        }

        public void save() {
            Properties props = new Properties();
            props.setProperty("ip", lastIp);
            props.setProperty("port", String.valueOf(lastPort));

            try (FileOutputStream fos = new FileOutputStream(configFile)) {
                props.store(fos, "Webots Connection Settings");
                CONFIG_LOGGER.info("Saved Webots config: {}:{}", lastIp, lastPort);
            } catch (Exception e) {
                CONFIG_LOGGER.error("Failed to save Webots config", e);
            }
        }

        public void update(String ip, int port) {
            this.lastIp = ip;
            this.lastPort = port;
            save();
        }

        public String getLastIp() { return lastIp; }
        public int getLastPort() { return lastPort; }
        public String getDefaultIp() { return DEFAULT_IP; }
        public int getDefaultPort() { return DEFAULT_PORT; }
    }
}
