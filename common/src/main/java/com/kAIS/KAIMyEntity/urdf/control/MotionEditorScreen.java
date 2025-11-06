package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFJoint;
import com.kAIS.KAIMyEntity.urdf.URDFRobotModel;

import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.Button;
import net.minecraft.client.gui.components.ObjectSelectionList;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;

import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.*;

/**
 * URDFMotionEditor (이름 유지) - 단순 인스펙터 버전
 * - 매핑 기능 제거
 * - 좌: VMC HumanoidTag(본) 목록 (position/오일러 요약을 한 줄로)
 * - 우: URDF Joint(모터) 목록 (현재값/리밋을 한 줄로)
 * - 하단: Refresh / Open Joint Editor / Close
 *
 * 주의: 행 높이 커스터마이즈(getRowHeight/getHeight) 오버라이드는 전부 제거(버전 호환).
 */
public class URDFMotionEditor extends Screen {

    private final Screen parent;
    private final Object renderer;

    private BoneList boneList;
    private JointList jointList;
    private Button refreshBtn, openJointEditorBtn, closeBtn;

    private String status = "";

    public URDFMotionEditor(Screen parent, Object renderer) {
        super(Component.literal("VMC & URDF Inspector"));
        this.parent = parent;
        this.renderer = renderer;
    }

    // 하위호환: 기존 호출(new URDFMotionEditor(robotModel, ctrl))을 받기 위한 어댑터
    public URDFMotionEditor(URDFRobotModel model, URDFSimpleController ctrl) {
        this(Minecraft.getInstance() != null ? Minecraft.getInstance().screen : null,
             new LegacyRendererAdapter(model));
    }
    private static final class LegacyRendererAdapter {
        private final URDFRobotModel model;
        LegacyRendererAdapter(URDFRobotModel model) { this.model = model; }
        public URDFRobotModel getRobotModel() { return model; }
        public String GetModelDir() { return "."; }
    }

    @Override
    protected void init() {
        super.init();

        final int margin = 8;
        final int titleH = 16;
        final int listTop = margin + titleH + 4;
        final int listHeight = this.height - listTop - 60;
        final int colWidth = (this.width - margin * 3) / 2;
        final int leftX = margin;
        final int rightX = leftX + colWidth + margin;

        this.boneList = new BoneList(this.minecraft, colWidth, listHeight, listTop, leftX);
        fillBones();
        this.addWidget(this.boneList);

        this.jointList = new JointList(this.minecraft, colWidth, listHeight, listTop, rightX);
        fillJoints();
        this.addWidget(this.jointList);

        int btnY = listTop + listHeight + 6;
        this.refreshBtn = this.addRenderableWidget(Button.builder(Component.literal("Refresh"), b -> {
            fillBones();
            fillJoints();
            this.status = "Refreshed.";
        }).bounds(margin, btnY, 80, 20).build());

        this.openJointEditorBtn = this.addRenderableWidget(Button.builder(Component.literal("Open Joint Editor"), b -> {
            tryOpenMotionEditor();
        }).bounds(margin + 86, btnY, 140, 20).build());

        this.closeBtn = this.addRenderableWidget(Button.builder(Component.literal("Close"), b -> onClose())
                .bounds(this.width - margin - 80, btnY, 80, 20).build());
    }

    private void fillBones() {
        this.boneList.children().clear();

        Object vmcState = reflectGetVmcState();
        Map<String, Object> bones = reflectCollectBoneMap(vmcState);

        if (bones.isEmpty()) {
            this.boneList.children().add(new BoneList.Entry(this.boneList, "(no VMC state)", "-", () -> {}));
            return;
        }

        List<String> names = new ArrayList<>(bones.keySet());
        names.sort(String.CASE_INSENSITIVE_ORDER);

        for (String name : names) {
            Object tr = bones.get(name);
            float[] pos = extractPos(tr);     // x,y,z
            float[] eul = extractEuler(tr);   // pitch,yaw,roll

            // 한 줄 요약 (행 높이 18px 기본에 맞춤)
            String line = String.format(Locale.ROOT,
                    "X:%.2f Y:%.2f Z:%.2f | p:%.2f y:%.2f r:%.2f",
                    pos[0], pos[1], pos[2], eul[0], eul[1], eul[2]);

            this.boneList.children().add(new BoneList.Entry(this.boneList, name, line, () -> {}));
        }
    }

    private void fillJoints() {
        this.jointList.children().clear();

        URDFRobotModel model = reflectGetRobotModel();
        if (model == null || model.joints == null || model.joints.isEmpty()) {
            this.jointList.children().add(new JointList.Entry(this.jointList, "(no URDF robot model)", "", () -> {}));
            return;
        }

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

            this.jointList.children().add(new JointList.Entry(this.jointList, name, limTxt, () -> {}));
        }
    }

    private void tryOpenMotionEditor() {
        try {
            if (renderer != null && renderer.getClass().getName().endsWith("URDFModelOpenGLWithSTL")) {
                this.minecraft.setScreen(new MotionEditorScreen(
                        (com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL) renderer));
                return;
            }
        } catch (Throwable ignored) {}
        this.status = "MotionEditorScreen not available.";
    }

    @Override
    public void onClose() {
        this.minecraft.setScreen(this.parent);
    }

    @Override
    public void render(GuiGraphics g, int mouseX, int mouseY, float partialTicks) {
        this.renderBackground(g, mouseX, mouseY, partialTicks);
        g.drawString(this.font, "VMC Humanoid Tags (Bones)", 8, 6, 0xFFD770, false);
        g.drawString(this.font, "URDF Motors (Joints)", this.width / 2 + 8, 6, 0xFFD770, false);

        if (!this.status.isEmpty())
            g.drawString(this.font, this.status, 8, this.height - 28, 0x80FF80, false);
        super.render(g, mouseX, mouseY, partialTicks);
    }

    /* ------------- VMC reflection ------------- */
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
            Field f = vmcState.getClass().getField("boneTransforms"); // Map<?, Transform>
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

    private float[] extractPos(Object transform) {
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

    private float[] extractEuler(Object transform) {
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

    /* ------------- URDF reflection ------------- */
    private URDFRobotModel reflectGetRobotModel() {
        try {
            Method m = renderer.getClass().getMethod("getRobotModel");
            return (URDFRobotModel) m.invoke(renderer);
        } catch (Throwable ignored) {}
        return null;
    }

    /* ------------- Lists ------------- */

    /** 좌: 본 목록 (한 줄 요약) */
    private static class BoneList extends ObjectSelectionList<BoneList.Entry> {
        final int left;
        public BoneList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height);
            this.left = left;
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }

        static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final BoneList owner;
            private final String name, line;
            private final Runnable onSelect;

            Entry(BoneList owner, String name, String line, Runnable onSelect) {
                this.owner = owner; this.name = name; this.line = line; this.onSelect = onSelect;
            }
            @Override
            public void render(GuiGraphics g, int idx, int top, int left, int width, int height,
                               int mouseX, int mouseY, boolean hovered, float partialTicks) {
                var font = Minecraft.getInstance().font;
                g.drawString(font, name, left + 2, top + 4, 0xFFFFFF, false);
                g.drawString(font, line, left + 120, top + 4, 0xA0E0FF, false);
            }
            @Override public boolean mouseClicked(double x, double y, int b) {
                if (b == 0) { owner.setSelected(this); onSelect.run(); return true; }
                return false;
            }
            @Override public Component getNarration() {
                return Component.literal(name + " " + line);
            }
        }
    }

    /** 우: 조인트 목록 (한 줄 요약) */
    private static class JointList extends ObjectSelectionList<JointList.Entry> {
        final int left;
        public JointList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height);
            this.left = left;
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }

        static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final JointList owner;
            private final String name, line;
            private final Runnable onSelect;

            Entry(JointList owner, String name, String line, Runnable onSelect) {
                this.owner = owner; this.name = name; this.line = line; this.onSelect = onSelect;
            }
            @Override
            public void render(GuiGraphics g, int idx, int top, int left, int width, int height,
                               int mouseX, int mouseY, boolean hovered, float partialTicks) {
                var font = Minecraft.getInstance().font;
                g.drawString(font, name, left + 2, top + 4, 0xFFFFFF, false);
                g.drawString(font, line, left + 120, top + 4, 0xB0FFA0, false);
            }
            @Override public boolean mouseClicked(double x, double y, int b) {
                if (b == 0) { owner.setSelected(this); onSelect.run(); return true; }
                return false;
            }
            @Override public Component getNarration() {
                return Component.literal(name + " " + line);
            }
        }
    }
}
