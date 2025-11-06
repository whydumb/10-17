package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFJoint;
import com.kAIS.KAIMyEntity.urdf.URDFRobotModel;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.Button;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;
import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.*;

/**
 * URDFMotionEditor (이름 유지) — MotionEditorScreen 스타일 인스펙터
 * - 매핑 기능 제거
 * - 좌: VMC HumanoidTag(본) 리스트 (수동 렌더, 페이지네이션)
 * - 우: URDF 조인트(모터) 리스트 (수동 렌더, 페이지네이션)
 * - ESC로 닫기, F5로 새로고침
 *
 * 시각 수정:
 * - 전체 배경/패널을 불투명 색으로 그림 (투명감 제거)
 */
public class URDFMotionEditor extends Screen {

    // ---------- 색상(불투명) ----------
    private static final int BG_COLOR    = 0xFF0E0E10; // 화면 전체 배경
    private static final int PANEL_COLOR = 0xFF1D1F24; // 좌/우 패널 박스
    private static final int TITLE_LEFT_COLOR  = 0xFFFFD770;
    private static final int TITLE_RIGHT_COLOR = 0xFFFFD770;
    private static final int TEXT_LEFT_COLOR   = 0xFFFFFFFF;
    private static final int TEXT_LEFT_SUB     = 0xFFA0E0FF;
    private static final int TEXT_RIGHT_COLOR  = 0xFFFFFFFF;
    private static final int TEXT_RIGHT_SUB    = 0xFFB0FFA0;

    private final Screen parent;
    private final Object renderer; // getRobotModel()

    // 좌/우 리스트 데이터
    private final List<BoneRow> boneRows = new ArrayList<>();
    private final List<JointRow> jointRows = new ArrayList<>();

    // 페이지 상태
    private int bonePage = 0;
    private int jointPage = 0;
    private int perPageLeft = 18;
    private int perPageRight = 18;

    // 레이아웃 캐시
    private int margin = 8;
    private int titleH = 16;
    private int listTop;
    private int colWidth;
    private int leftX, rightX;
    private int listHeight;

    // UI 버튼 (상단 컨트롤만 사용)
    private Button bonePrevBtn, boneNextBtn, bonePageBtn;
    private Button jointPrevBtn, jointNextBtn, jointPageBtn;

    public URDFMotionEditor(Screen parent, Object renderer) {
        super(Component.literal("VMC & URDF Inspector"));
        this.parent = parent;
        this.renderer = renderer;
    }

    // 하위호환: 예전 호출(new URDFMotionEditor(robotModel, ctrl))
    public URDFMotionEditor(URDFRobotModel model, URDFSimpleController ctrl) {
        this(Minecraft.getInstance() != null ? Minecraft.getInstance().screen : null,
             new LegacyRendererAdapter(model));
    }
    private static final class LegacyRendererAdapter {
        private final URDFRobotModel model;
        LegacyRendererAdapter(URDFRobotModel model) { this.model = model; }
        public URDFRobotModel getRobotModel() { return model; }
    }

    @Override
    protected void init() {
        super.init();
        computeLayout();
        buildData();
        buildHeaderControls();
        updatePageLabels();
    }

    private void computeLayout() {
        listTop = margin + titleH + 4;
        listHeight = Math.max(120, this.height - listTop - 20); // 최소 높이 확보
        colWidth = (this.width - margin * 3) / 2;
        leftX = margin;
        rightX = leftX + colWidth + margin;

        // 행 높이 12px 기준 perPage 결정
        perPageLeft = Math.max(5, listHeight / 12);
        perPageRight = perPageLeft;
    }

    private void buildHeaderControls() {
        // 좌측 헤더 Prev/Next
        bonePrevBtn = addRenderableWidget(Button.builder(Component.literal("< Prev"), b -> {
            if (bonePage > 0) bonePage--;
            updatePageLabels();
        }).bounds(leftX, listTop - 26, 60, 20).build());

        boneNextBtn = addRenderableWidget(Button.builder(Component.literal("Next >"), b -> {
            int pages = Math.max(1, (int)Math.ceil(boneRows.size() / (double)perPageLeft));
            if (bonePage < pages - 1) bonePage++;
            updatePageLabels();
        }).bounds(leftX + 66, listTop - 26, 60, 20).build());

        bonePageBtn = addRenderableWidget(Button.builder(Component.literal("Page"), b -> {})
                .bounds(leftX + 132, listTop - 26, 90, 20).build());
        bonePageBtn.active = false;

        // 우측 헤더 Prev/Next
        jointPrevBtn = addRenderableWidget(Button.builder(Component.literal("< Prev"), b -> {
            if (jointPage > 0) jointPage--;
            updatePageLabels();
        }).bounds(rightX, listTop - 26, 60, 20).build());

        jointNextBtn = addRenderableWidget(Button.builder(Component.literal("Next >"), b -> {
            int pages = Math.max(1, (int)Math.ceil(jointRows.size() / (double)perPageRight));
            if (jointPage < pages - 1) jointPage++;
            updatePageLabels();
        }).bounds(rightX + 66, listTop - 26, 60, 20).build());

        jointPageBtn = addRenderableWidget(Button.builder(Component.literal("Page"), b -> {})
                .bounds(rightX + 132, listTop - 26, 90, 20).build());
        jointPageBtn.active = false;
    }

    private void updatePageLabels() {
        int bonePages  = Math.max(1, (int)Math.ceil(boneRows.size() / (double)perPageLeft));
        int jointPages = Math.max(1, (int)Math.ceil(jointRows.size() / (double)perPageRight));
        bonePage = clamp(bonePage, 0, bonePages - 1);
        jointPage = clamp(jointPage, 0, jointPages - 1);

        bonePageBtn.setMessage(Component.literal("Page " + (bonePage+1) + "/" + bonePages));
        jointPageBtn.setMessage(Component.literal("Page " + (jointPage+1) + "/" + jointPages));
    }

    private void buildData() {
        boneRows.clear();
        jointRows.clear();

        // VMC 본 덤프
        Map<String, Object> bones = reflectCollectBoneMap(reflectGetVmcState());
        List<String> boneNames = new ArrayList<>(bones.keySet());
        boneNames.sort(String.CASE_INSENSITIVE_ORDER);

        if (boneNames.isEmpty()) {
            boneRows.add(new BoneRow("(no VMC state)", "-"));
        } else {
            for (String name : boneNames) {
                Object tr = bones.get(name);
                float[] p = extractPos(tr);
                float[] e = extractEuler(tr);
                String line = String.format(Locale.ROOT,
                        "X:%.2f Y:%.2f Z:%.2f | p:%.2f y:%.2f r:%.2f",
                        p[0], p[1], p[2], e[0], e[1], e[2]);
                boneRows.add(new BoneRow(name, line));
            }
        }

        // URDF 조인트 덤프
        URDFRobotModel model = reflectGetRobotModel();
        if (model == null || model.joints == null || model.joints.isEmpty()) {
            jointRows.add(new JointRow("(no URDF model)", ""));
        } else {
            for (URDFJoint j : model.joints) {
                String name = j.name != null ? j.name : "(unnamed)";
                float curDeg = (float) Math.toDegrees(j.currentPosition);
                String limTxt;
                if (j.limit != null && j.limit.hasLimits() && j.limit.upper > j.limit.lower) {
                    int lo = Math.round((float) Math.toDegrees(j.limit.lower));
                    int hi = Math.round((float) Math.toDegrees(j.limit.upper));
                    limTxt = String.format(Locale.ROOT, "cur:%d° | lim:[%d°, %d°]", Math.round(curDeg), lo, hi);
                } else {
                    limTxt = String.format(Locale.ROOT, "cur:%d° | lim:(none)", Math.round(curDeg));
                }
                jointRows.add(new JointRow(name, limTxt));
            }
        }

        // 페이지 초기화
        bonePage = 0;
        jointPage = 0;
    }

    @Override
    public void resize(Minecraft mc, int w, int h) {
        super.resize(mc, w, h);
        computeLayout();
        updateHeaderBounds();
        updatePageLabels();
    }

    private void updateHeaderBounds() {
        // 좌측
        bonePrevBtn.setX(leftX);
        bonePrevBtn.setY(listTop - 26);
        boneNextBtn.setX(leftX + 66);
        boneNextBtn.setY(listTop - 26);
        bonePageBtn.setX(leftX + 132);
        bonePageBtn.setY(listTop - 26);
        // 우측
        jointPrevBtn.setX(rightX);
        jointPrevBtn.setY(listTop - 26);
        jointNextBtn.setX(rightX + 66);
        jointNextBtn.setY(listTop - 26);
        jointPageBtn.setX(rightX + 132);
        jointPageBtn.setY(listTop - 26);
    }

    @Override
public void render(GuiGraphics g, int mouseX, int mouseY, float partialTicks) {
    // 1) 전체 화면을 불투명 배경으로 칠함 (투명감 제거)
    g.fill(0, 0, this.width, this.height, BG_COLOR);

    // 2) 좌/우 패널 박스 (불투명)
    g.fill(leftX,  listTop, leftX  + colWidth, listTop + listHeight, PANEL_COLOR);
    g.fill(rightX, listTop, rightX + colWidth, listTop + listHeight, PANEL_COLOR);

    // 3) 기본 위젯들(버튼 등) 먼저 렌더
    super.render(g, mouseX, mouseY, partialTicks);

    // 4) 텍스트는 "맨 위 레이어"로 올려 그리기 (Z 크게 밀기)
    g.pose().pushPose();
    g.pose().translate(0, 0, 1000.0f); // 화면 최상층으로

    // 제목
    g.drawString(this.font, "VMC Bones (HumanoidTag) — F5 refresh", leftX, 6, TITLE_LEFT_COLOR, false);
    g.drawString(this.font, "URDF Motors (Joints)",               rightX, 6, TITLE_RIGHT_COLOR, false);

    // 좌측 리스트 텍스트
    int y = listTop + 4;
    int start = bonePage * perPageLeft;
    int end = Math.min(boneRows.size(), start + perPageLeft);
    for (int i = start; i < end; i++) {
        BoneRow r = boneRows.get(i);
        g.drawString(this.font, r.name, leftX + 4, y, TEXT_LEFT_COLOR, false);
        int nx = leftX + 4 + this.font.width(r.name) + 6;
        g.drawString(this.font, r.detail, nx, y, TEXT_LEFT_SUB, false);
        y += 12;
    }

    // 우측 리스트 텍스트
    y = listTop + 4;
    start = jointPage * perPageRight;
    end = Math.min(jointRows.size(), start + perPageRight);
    for (int i = start; i < end; i++) {
        JointRow r = jointRows.get(i);
        g.drawString(this.font, r.name, rightX + 4, y, TEXT_RIGHT_COLOR, false);
        int nx = rightX + 4 + this.font.width(r.name) + 6;
        g.drawString(this.font, r.detail, nx, y, TEXT_RIGHT_SUB, false);
        y += 12;
    }

    g.pose().popPose();
}


    // 마우스 휠: 4인자 시그니처(1.20+ 매핑)
    @Override
    public boolean mouseScrolled(double mx, double my, double deltaX, double deltaY) {
        double delta = (Math.abs(deltaY) > 0.0) ? deltaY : deltaX; // 휠 우선, 없으면 수평 스크롤
        boolean inLeft  = mx >= leftX  && mx < leftX  + colWidth && my >= listTop && my < listTop + listHeight;
        boolean inRight = mx >= rightX && mx < rightX + colWidth && my >= listTop && my < listTop + listHeight;

        if (inLeft) {
            int pages = Math.max(1, (int)Math.ceil(boneRows.size() / (double)perPageLeft));
            bonePage = clamp(bonePage - (int)Math.signum(delta), 0, pages - 1);
            updatePageLabels();
            return true;
        } else if (inRight) {
            int pages = Math.max(1, (int)Math.ceil(jointRows.size() / (double)perPageRight));
            jointPage = clamp(jointPage - (int)Math.signum(delta), 0, pages - 1);
            updatePageLabels();
            return true;
        }
        return super.mouseScrolled(mx, my, deltaX, deltaY);
    }

    // 키: F5 새로고침, ESC 닫기(기본)
    @Override
    public boolean keyPressed(int keyCode, int scanCode, int modifiers) {
        // GLFW.GLFW_KEY_F5 = 294
        if (keyCode == 294) {
            buildData();
            updatePageLabels();
            return true;
        }
        return super.keyPressed(keyCode, scanCode, modifiers);
    }

    @Override
    public void onClose() {
        this.minecraft.setScreen(this.parent);
    }

    /* ---------------- 데이터 행 ---------------- */
    private static class BoneRow {
        final String name;
        final String detail;
        BoneRow(String n, String d) { name = n; detail = d; }
    }
    private static class JointRow {
        final String name;
        final String detail;
        JointRow(String n, String d) { name = n; detail = d; }
    }

    /* ---------------- VMC reflection ---------------- */
    private Object reflectGetVmcState() {
        try {
            Class<?> mgr = Class.forName("top.fifthlight.armorstand.vmc.VmcMarionetteManager");
            Method getState = mgr.getMethod("getState");
            return getState.invoke(null);
        } catch (Throwable ignored) {}
        return null;
    }

    @SuppressWarnings("unchecked")
    private Map<String, Object> reflectCollectBoneMap(Object vmcState) {
        Map<String, Object> map = new HashMap<>();
        if (vmcState == null) return map;
        try {
            Field f = vmcState.getClass().getField("boneTransforms");
            Object raw = f.get(vmcState);
            if (!(raw instanceof Map)) return map;
            Map<Object, Object> m = (Map<Object, Object>) raw;
            for (Map.Entry<Object, Object> e : m.entrySet()) {
                Object tag = e.getKey();
                String name = tag.toString();
                try {
                    Method nameM = tag.getClass().getMethod("name");
                    Object n = nameM.invoke(tag);
                    if (n != null) name = n.toString();
                } catch (Throwable ignored) {}
                map.put(name, e.getValue());
            }
        } catch (Throwable ignored) {}
        return map;
    }

    private static float[] extractPos(Object transform) {
        float[] r = {0, 0, 0};
        if (transform == null) return r;
        try {
            Object pos = transform.getClass().getField("position").get(transform);
            Method mx = pos.getClass().getMethod("x");
            Method my = pos.getClass().getMethod("y");
            Method mz = pos.getClass().getMethod("z");
            r[0] = (Float) mx.invoke(pos);
            r[1] = (Float) my.invoke(pos);
            r[2] = (Float) mz.invoke(pos);
        } catch (Throwable ignored) {}
        return r;
    }

    private static float[] extractEuler(Object transform) {
        float[] r = {0, 0, 0};
        if (transform == null) return r;
        try {
            Object rot = transform.getClass().getField("rotation").get(transform);
            Method mx = rot.getClass().getMethod("x");
            Method my = rot.getClass().getMethod("y");
            Method mz = rot.getClass().getMethod("z");
            Method mw = rot.getClass().getMethod("w");
            float qx = (Float) mx.invoke(rot);
            float qy = (Float) my.invoke(rot);
            float qz = (Float) mz.invoke(rot);
            float qw = (Float) mw.invoke(rot);
            Vector3f e = new Vector3f();
            new Quaternionf(qx, qy, qz, qw).getEulerAnglesXYZ(e);
            r[0] = e.x; r[1] = e.y; r[2] = e.z;
        } catch (Throwable ignored) {}
        return r;
    }

    /* ---------------- URDF access ---------------- */
    private URDFRobotModel reflectGetRobotModel() {
        try {
            Method m = renderer.getClass().getMethod("getRobotModel");
            return (URDFRobotModel) m.invoke(renderer);
        } catch (Throwable ignored) {}
        return null;
    }

    private static int clamp(int v, int lo, int hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
}
