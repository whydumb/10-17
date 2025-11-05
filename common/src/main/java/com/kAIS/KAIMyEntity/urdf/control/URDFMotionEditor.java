package com.kAIS.KAIMyEntity.urdf.control;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.kAIS.KAIMyEntity.urdf.URDFJoint;
import com.kAIS.KAIMyEntity.urdf.URDFRobotModel;
import com.kAIS.KAIMyEntity.urdf.control.URDFSimpleController;

import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.AbstractSliderButton;
import net.minecraft.client.gui.components.Button;
import net.minecraft.client.gui.components.CycleButton;
import net.minecraft.client.gui.components.ObjectSelectionList;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;

import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.function.Consumer;

/**
 * URDFMotionEditor (이름 유지) = VMC-URDF 매핑 에디터 GUI
 * - 외부 의존성(URDFRenderer, VMC)은 전부 리플렉션으로 접근
 * - ObjectSelectionList 1.19 계열 (ctor 5인자), renderBackground(GuiGraphics, x,y,pt) 시그니처 사용
 * - 하위호환 생성자(URDFRobotModel, URDFSimpleController) 제공 → 기존 호출 그대로 동작
 */
public class URDFMotionEditor extends Screen {

    // --- VMC 표준 본 목록 ---
    private static final String[] VMC_BONES = {
            "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
    };

    // --- 데이터 모델 ---
    public static class VMCMapping {
        public String vmcBone;
        public String urdfJoint;
        public float multiplier = 1.0f;
        public float offset = 0.0f;
        public String component = "Y"; // X,Y,Z or pitch,roll,yaw
        public enum ExtractionMode { EULER_X, EULER_Y, EULER_Z, QUATERNION_ANGLE }
        public ExtractionMode mode = ExtractionMode.EULER_Y;
    }
    private static class VMCMappingSet { public List<VMCMapping> mappings = new ArrayList<>(); }

    // --- 레퍼런스 ---
    private final Screen parent;
    /** getRobotModel(), setJointPreview(String,float), GetModelDir() 를 가진 객체 (리플렉션으로 호출) */
    private final Object renderer;

    // --- UI ---
    private BoneList vmcList;
    private JointList jointList;
    private MappingList mappingList;

    private Button addOrUpdateBtn, removeBtn, testBtn, saveBtn, loadBtn, closeBtn;
    private FloatSlider multiplierSlider, offsetSlider;
    private CycleButton<String> componentCycle;
    private CycleButton<VMCMapping.ExtractionMode> modeCycle;

    // --- 상태 ---
    private String selectedVmcBone = null;
    private String selectedUrdfJoint = null;
    private float currentMultiplier = 1.0f;
    private float currentOffset = 0.0f;
    private String currentComponent = "Y";
    private VMCMapping.ExtractionMode currentMode = VMCMapping.ExtractionMode.EULER_Y;

    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private String status = "";

    private final Map<String, VMCMapping> mappingIndex = new HashMap<>();

    // ===== 신규 표준 생성자: (Screen parent, Object renderer) =====
    public URDFMotionEditor(Screen parent, Object renderer) {
        super(Component.literal("VMC-URDF Mapping Editor"));
        this.parent = parent;
        this.renderer = renderer;
    }

    // ===== 하위호환 생성자: (URDFRobotModel, URDFSimpleController) =====
    // 기존 코드: new URDFMotionEditor(robotModel, ctrl) 그대로 동작
    public URDFMotionEditor(URDFRobotModel model, URDFSimpleController ctrl) {
        this(
            Minecraft.getInstance() != null ? Minecraft.getInstance().screen : null,
            new LegacyRendererAdapter(model, ctrl)
        );
    }

    // 구형 호출을 지원하기 위한 얇은 어댑터 (리플렉션 대상 메서드 제공)
    private static final class LegacyRendererAdapter {
        private final URDFRobotModel model;
        private final URDFSimpleController ctrl;

        LegacyRendererAdapter(URDFRobotModel model, URDFSimpleController ctrl) {
            this.model = model;
            this.ctrl = ctrl;
        }
        public URDFRobotModel getRobotModel() { return model; }
        public void setJointPreview(String name, float value) {
            if (model == null || model.joints == null) return;
            for (URDFJoint j : model.joints) {
                if (j != null && name.equals(j.name)) {
                    j.currentPosition = value;
                    if (ctrl != null) {
                        try {
                            ctrl.getClass().getMethod("setJointPosition", String.class, float.class)
                                .invoke(ctrl, name, value);
                        } catch (Throwable ignored) {}
                    }
                    break;
                }
            }
        }
        public String GetModelDir() { return "."; }
    }

    @Override
    protected void init() {
        super.init();

        final int margin = 8;
        final int titleH = 16;
        final int listTop = margin + titleH + 4;
        final int listHeight = this.height - listTop - 160;
        final int colWidth = (this.width - margin * 3) / 2;
        final int leftX = margin;
        final int rightX = leftX + colWidth + margin;

        // 좌측 VMC 목록
        this.vmcList = new BoneList(this.minecraft, colWidth, listHeight, listTop, leftX);
        for (String s : VMC_BONES) {
            this.vmcList.children().add(new BoneList.Entry(s, () -> {
                this.selectedVmcBone = s;
                updateAddButtonLabel();
            }));
        }
        this.addWidget(this.vmcList);

        // 우측 URDF 조인트 목록
        this.jointList = new JointList(this.minecraft, colWidth, listHeight, listTop, rightX);
        URDFRobotModel model = reflectGetRobotModel();
        if (model != null && model.joints != null) {
            for (URDFJoint j : model.joints) {
                final String name = j.name;
                this.jointList.children().add(new JointList.Entry(name, () -> {
                    this.selectedUrdfJoint = name;
                    updateAddButtonLabel();
                }));
            }
        }
        this.addWidget(this.jointList);

        int y = listTop + listHeight + 6;

        // Component 선택
        this.componentCycle = this.addRenderableWidget(
                CycleButton.<String>builder(s -> Component.literal(s))
                        .withValues("X","Y","Z","pitch","roll","yaw")
                        .withInitialValue(this.currentComponent)
                        .create(margin, y, 120, 20, Component.literal("Component"),
                                (btn, v) -> this.currentComponent = v)
        );

        // Mode 선택
        this.modeCycle = this.addRenderableWidget(
                CycleButton.<VMCMapping.ExtractionMode>builder(m -> Component.literal(m.name()))
                        .withValues(VMCMapping.ExtractionMode.values())
                        .withInitialValue(this.currentMode)
                        .create(margin + 125, y, 160, 20, Component.literal("Mode"),
                                (btn, v) -> this.currentMode = v)
        );

        // Multiplier 슬라이더
        y += 24;
        this.multiplierSlider = new FloatSlider(margin, y, 200, 20, Component.literal("Multiplier"),
                -3.0f, 3.0f, this.currentMultiplier, v -> this.currentMultiplier = v);
        this.addRenderableWidget(this.multiplierSlider);

        // Offset 슬라이더
        this.offsetSlider = new FloatSlider(margin + 210, y, 220, 20, Component.literal("Offset [rad]"),
                (float)-Math.PI, (float)Math.PI, this.currentOffset, v -> this.currentOffset = v);
        this.addRenderableWidget(this.offsetSlider);

        // 매핑 리스트
        int mapListTop = y + 28;
        int mapListHeight = 90;
        this.mappingList = new MappingList(this.minecraft, this.width - margin*2, mapListHeight, mapListTop, margin);
        this.addWidget(this.mappingList);

        // 버튼
        int btnY = mapListTop + mapListHeight + 6;
        this.addOrUpdateBtn = this.addRenderableWidget(Button.builder(Component.literal("Add Mapping"), b -> onAddOrUpdate())
                .bounds(margin, btnY, 110, 20).build());
        this.removeBtn = this.addRenderableWidget(Button.builder(Component.literal("삭제"), b -> onRemoveSelected())
                .bounds(margin + 116, btnY, 60, 20).build());
        this.testBtn = this.addRenderableWidget(Button.builder(Component.literal("VMC 테스트"), b -> onTest())
                .bounds(margin + 182, btnY, 90, 20).build());
        this.saveBtn = this.addRenderableWidget(Button.builder(Component.literal("저장"), b -> onSave())
                .bounds(margin + 276, btnY, 60, 20).build());
        this.loadBtn = this.addRenderableWidget(Button.builder(Component.literal("불러오기"), b -> onLoad())
                .bounds(margin + 340, btnY, 80, 20).build());
        this.closeBtn = this.addRenderableWidget(Button.builder(Component.literal("닫기"), b -> onClose())
                .bounds(this.width - margin - 80, btnY, 80, 20).build());

        rebuildIndex();
        updateAddButtonLabel();
    }

    private void updateAddButtonLabel() {
        boolean exists = findMapping(selectedVmcBone, selectedUrdfJoint) != null;
        if (this.addOrUpdateBtn != null) {
            this.addOrUpdateBtn.setMessage(Component.literal(exists ? "Update Mapping" : "Add Mapping"));
        }
    }

    private void onAddOrUpdate() {
        if (this.selectedVmcBone == null || this.selectedUrdfJoint == null) {
            this.status = "VMC 본과 URDF 조인트를 선택하세요.";
            return;
        }
        VMCMapping exist = findMapping(selectedVmcBone, selectedUrdfJoint);
        if (exist == null) {
            VMCMapping m = new VMCMapping();
            m.vmcBone = selectedVmcBone;
            m.urdfJoint = selectedUrdfJoint;
            m.multiplier = currentMultiplier;
            m.offset = currentOffset;
            m.component = currentComponent;
            m.mode = currentMode;
            this.mappingList.children().add(new MappingList.Entry(m, this.mappingList));
            indexPut(m);
            this.status = "매핑 추가: " + m.vmcBone + " → " + m.urdfJoint;
        } else {
            exist.multiplier = currentMultiplier;
            exist.offset = currentOffset;
            exist.component = currentComponent;
            exist.mode = currentMode;
            this.status = "매핑 업데이트: " + exist.vmcBone + " → " + exist.urdfJoint;
        }
        updateAddButtonLabel();
    }

    private void onRemoveSelected() {
        MappingList.Entry sel = this.mappingList.getSelectedEntry();
        if (sel == null) {
            this.status = "삭제할 매핑을 목록에서 선택하세요.";
            return;
        }
        this.mappingList.children().remove(sel);
        indexRemove(sel.mapping);
		this.status = "삭제됨: " + sel.mapping.vmcBone + " → " + sel.mapping.urdfJoint;
        updateAddButtonLabel();
    }

    private void onTest() {
        Object state = reflectGetVmcState();
        if (state == null) {
            this.status = "VMC 상태가 없습니다 (연결/실행 확인).";
            return;
        }
        applyCurrentMappings(state);
        this.status = "테스트 적용 완료.";
    }

    private void onSave() {
        Path file = getMappingFile();
        VMCMappingSet set = new VMCMappingSet();
        for (MappingList.Entry e : this.mappingList.children()) set.mappings.add(e.mapping);
        try {
            Files.createDirectories(file.getParent());
            Files.writeString(file, gson.toJson(set));
            this.status = "저장 완료: " + file;
        } catch (IOException e) {
            this.status = "저장 실패: " + e.getMessage();
        }
    }

    private void onLoad() {
        Path file = getMappingFile();
        if (!Files.exists(file)) { this.status = "파일 없음: " + file; return; }
        try {
            String json = Files.readString(file);
            VMCMappingSet set = gson.fromJson(json, VMCMappingSet.class);
            this.mappingList.children().clear();
            this.mappingIndex.clear();
            if (set != null && set.mappings != null) {
                for (VMCMapping m : set.mappings) {
                    this.mappingList.children().add(new MappingList.Entry(m, this.mappingList));
                    indexPut(m);
                }
            }
            this.status = "불러오기 완료: " + file;
        } catch (IOException e) {
            this.status = "불러오기 실패: " + e.getMessage();
        }
    }

    @Override
    public void onClose() { // Screen#onClose 는 public 이어야 함
        this.minecraft.setScreen(this.parent);
    }

    private Path getMappingFile() {
        String dir = reflectGetModelDir();
        return Paths.get(dir == null ? "." : dir, "vmc_mapping.json");
    }

    private void rebuildIndex() {
        this.mappingIndex.clear();
        for (MappingList.Entry e : this.mappingList.children()) indexPut(e.mapping);
    }
    private void indexPut(VMCMapping m) { this.mappingIndex.put(key(m.vmcBone, m.urdfJoint), m); }
    private void indexRemove(VMCMapping m) { this.mappingIndex.remove(key(m.vmcBone, m.urdfJoint)); }
    private static String key(String vmc, String urdf){ return (vmc==null?"":vmc) + "|" + (urdf==null?"":urdf); }
    private VMCMapping findMapping(String vmc, String urdf){ return this.mappingIndex.get(key(vmc, urdf)); }

    // --- 렌더 ---
    @Override
    public void render(GuiGraphics g, int mouseX, int mouseY, float partialTicks) {
        this.renderBackground(g, mouseX, mouseY, partialTicks); // 1.19 계열 시그니처
        g.drawString(this.font, "VMC-URDF Mapping Editor", 8, 6, 0xFFFFFF, false);
        g.drawString(this.font, "VMC 본 목록", 8, 20, 0xFFD770, false);
        g.drawString(this.font, "URDF 조인트 목록", this.width / 2 + 4, 20, 0xFFD770, false);

        int infoY = this.vmcList.getBottom() + 6;
        String sel = "선택된 매핑: " + (selectedVmcBone==null?"(VMC 미선택)":selectedVmcBone)
                + " → " + (selectedUrdfJoint==null?"(URDF 미선택)":selectedUrdfJoint);
        g.drawString(this.font, sel, 8, infoY, 0xA0A0A0, false);

        if (!this.status.isEmpty()) g.drawString(this.font, this.status, 8, this.height - 28, 0x80FF80, false);
        super.render(g, mouseX, mouseY, partialTicks);
    }

    // --- VMC 적용 (리플렉션) ---
    private void applyCurrentMappings(Object vmcState) {
        Map<String, Object> byName = reflectCollectBoneMap(vmcState);
        for (MappingList.Entry e : this.mappingList.children()) {
            VMCMapping m = e.mapping;
            Object tr = byName.get(m.vmcBone);
            if (tr == null) continue;
            float value = extractValueReflect(tr, m);
            value = applyMulOffsetClamp(value, m);
            reflectSetJointPreview(m.urdfJoint, value);
        }
    }

    private float extractValueReflect(Object transform, VMCMapping m) {
        try {
            Field fPos = transform.getClass().getField("position");
            Field fRot = transform.getClass().getField("rotation");
            Object pos = fPos.get(transform);
            Object rot = fRot.get(transform);

            if ("X".equalsIgnoreCase(m.component) ||
                "Y".equalsIgnoreCase(m.component) ||
                "Z".equalsIgnoreCase(m.component)) {
                Method mx = pos.getClass().getMethod("x");
                Method my = pos.getClass().getMethod("y");
                Method mz = pos.getClass().getMethod("z");
                float x = ((Float)mx.invoke(pos));
                float y = ((Float)my.invoke(pos));
                float z = ((Float)mz.invoke(pos));
                switch (m.component.toUpperCase(Locale.ROOT)) {
                    case "X": return x;
                    case "Y": return y;
                    case "Z": return z;
                }
            }

            Method mx = rot.getClass().getMethod("x");
            Method my = rot.getClass().getMethod("y");
            Method mz = rot.getClass().getMethod("z");
            Method mw = rot.getClass().getMethod("w");
            float qx = ((Float)mx.invoke(rot));
            float qy = ((Float)my.invoke(rot));
            float qz = ((Float)mz.invoke(rot));
            float qw = ((Float)mw.invoke(rot));

            if (m.mode == VMCMapping.ExtractionMode.QUATERNION_ANGLE) {
                double angle = 2.0 * Math.acos(Math.max(-1.0, Math.min(1.0, qw)));
                return (float)angle;
            }

            Vector3f euler = new Vector3f();
            new Quaternionf(qx, qy, qz, qw).getEulerAnglesXYZ(euler);
            float pitch = euler.x; float yaw = euler.y; float roll = euler.z;

            switch (m.mode) {
                case EULER_X: return pitch;
                case EULER_Y: return yaw;
                case EULER_Z: return roll;
            }
            switch (m.component.toLowerCase(Locale.ROOT)) {
                case "pitch": return pitch;
                case "yaw":   return yaw;
                case "roll":  return roll;
            }
        } catch (Throwable ignored) {}
        return 0f;
    }

    private float applyMulOffsetClamp(float v, VMCMapping m) {
        float out = v * m.multiplier + m.offset;
        URDFRobotModel model = reflectGetRobotModel();
        if (model != null && model.joints != null) {
            for (URDFJoint j : model.joints) {
                if (Objects.equals(j.name, m.urdfJoint) && j.limit != null) {
                    float lo = (float) j.limit.lower;
                    float hi = (float) j.limit.upper;
                    if (hi > lo) {
                        if (out < lo) out = lo;
                        if (out > hi) out = hi;
                    }
                    break;
                }
            }
        }
        return out;
    }

    // --- 리플렉션 유틸 ---
    private URDFRobotModel reflectGetRobotModel() {
        try {
            Method m = renderer.getClass().getMethod("getRobotModel");
            return (URDFRobotModel)m.invoke(renderer);
        } catch (Throwable ignored) {}
        return null;
    }
    private void reflectSetJointPreview(String name, float value) {
        try {
            Method m = renderer.getClass().getMethod("setJointPreview", String.class, float.class);
            m.invoke(renderer, name, value);
        } catch (Throwable ignored) {}
    }
    private String reflectGetModelDir() {
        try {
            Method m = renderer.getClass().getMethod("GetModelDir");
            Object r = m.invoke(renderer);
            return r == null ? null : r.toString();
        } catch (Throwable ignored) {}
        return null;
    }
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

    // --- 리스트 위젯들 (1.19 시그니처) ---
    private static class BoneList extends ObjectSelectionList<BoneList.Entry> {
        final int left;
        public BoneList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height);
            this.left = left;
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }
        @Override public int getX() { return this.left; }

        public static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final String name;
            private final Runnable onSelect;
            public Entry(String name, Runnable onSelect) { this.name = name; this.onSelect = onSelect; }
            @Override
            public void render(GuiGraphics g, int idx, int top, int left, int width, int height,
                               int mouseX, int mouseY, boolean hovered, float partialTicks) {
                g.drawString(Minecraft.getInstance().font, name, left + 2, top + 4, 0xFFFFFF, false);
            }
            @Override public boolean mouseClicked(double mx, double my, int button) {
                if (button == 0) { onSelect.run(); return true; }
                return false;
            }
            @Override public Component getNarration() { return Component.literal(name); }
        }
    }

    private static class JointList extends ObjectSelectionList<JointList.Entry> {
        final int left;
        public JointList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height);
            this.left = left;
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }
        @Override public int getX() { return this.left; }

        public static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final String name;
            private final Runnable onSelect;
            public Entry(String name, Runnable onSelect) { this.name = name; this.onSelect = onSelect; }
            @Override
            public void render(GuiGraphics g, int idx, int top, int left, int width, int height,
                               int mouseX, int mouseY, boolean hovered, float partialTicks) {
                g.drawString(Minecraft.getInstance().font, name, left + 2, top + 4, 0xFFFFFF, false);
            }
            @Override public boolean mouseClicked(double mx, double my, int button) {
                if (button == 0) { onSelect.run(); return true; }
                return false;
            }
            @Override public Component getNarration() { return Component.literal(name); }
        }
    }

    private static class MappingList extends ObjectSelectionList<MappingList.Entry> {
        final int left;
        public MappingList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height);
            this.left = left;
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }
        @Override public int getX() { return this.left; }

        public Entry getSelectedEntry() { return super.getSelected(); }

        public static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final MappingList owner;
            public final VMCMapping mapping;
            public Entry(VMCMapping mapping, MappingList owner) { this.mapping = mapping; this.owner = owner; }

            @Override
            public void render(GuiGraphics g, int idx, int top, int left, int width, int height,
                               int mouseX, int mouseY, boolean hovered, float partialTicks) {
                String text = String.format(Locale.ROOT,
                        "• %s → %s (×%.3f, +%.3f, %s, %s)",
                        mapping.vmcBone, mapping.urdfJoint,
                        mapping.multiplier, mapping.offset,
                        mapping.component, mapping.mode.name());
                g.drawString(Minecraft.getInstance().font, text, left + 2, top + 4, 0xE0E0E0, false);
            }
            @Override
            public boolean mouseClicked(double mx, double my, int button) {
                if (button == 0) {
                    this.owner.setSelected(this);
                    URDFMotionEditor scr = (URDFMotionEditor) Minecraft.getInstance().screen;
                    if (scr != null) scr.loadToEditors(mapping);
                    return true;
                }
                return false;
            }
            @Override public Component getNarration() {
                return Component.literal(mapping.vmcBone + "→" + mapping.urdfJoint);
            }
        }
    }

    private void loadToEditors(VMCMapping m) {
        this.selectedVmcBone = m.vmcBone;
        this.selectedUrdfJoint = m.urdfJoint;
        this.currentMultiplier = m.multiplier;
               this.currentOffset = m.offset;
        this.currentComponent = m.component;
        this.currentMode = m.mode;

        this.multiplierSlider.setValueImmediately(currentMultiplier);
        this.offsetSlider.setValueImmediately(currentOffset);
        this.componentCycle.setValue(currentComponent);
        this.modeCycle.setValue(currentMode);

        updateAddButtonLabel();
        this.status = "편집 로드: " + m.vmcBone + " → " + m.urdfJoint;
    }

    private static class FloatSlider extends AbstractSliderButton {
        private final float min, max;
        private final Consumer<Float> onChange;
        private final String base;
        public FloatSlider(int x, int y, int w, int h, Component title,
                           float min, float max, float initial, Consumer<Float> onChange) {
            super(x, y, w, h, title, 0.0D);
            this.min = min; this.max = max; this.onChange = onChange; this.base = title.getString();
            setValueImmediately(initial);
        }
        public void setValueImmediately(float value) {
            this.value = toSlider(value);
            applyValue();
            updateMessage();
        }
        private double toSlider(float v) { return (v - min) / (double)(max - min); }
        private float fromSlider(double s) { return (float)(min + s * (max - min)); }
        @Override protected void updateMessage() {
            float v = fromSlider(this.value);
            this.setMessage(Component.literal(base + ": " + String.format(Locale.ROOT, "%.3f", v)));
        }
        @Override protected void applyValue() { this.onChange.accept(fromSlider(this.value)); }
    }
}
