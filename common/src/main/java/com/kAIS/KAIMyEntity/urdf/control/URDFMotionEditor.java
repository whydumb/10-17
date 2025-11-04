// SPDX-License-Identifier: MIT
// (c) KAIMyEntity sample implementation
//
// Minecraft 1.20.x 기준으로 작성되었습니다.
// - 1.19.x를 사용하시면 Screen#render(PoseStack, int, int, float)로 바꾸고
//   GuiGraphics 호출을 PoseStack 기반 그리기로 변환하세요.
// - ObjectSelectionList는 1.19/1.20 모두 존재합니다.
// - Checkbox, CycleButton, AbstractSliderButton 역시 버전에 맞춰 import만 조정하면 됩니다.

package com.kAIS.KAIMyEntity.client.gui;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.kAIS.KAIMyEntity.urdf.URDFJoint;
import com.kAIS.KAIMyEntity.urdf.URDFRobotModel;
// renderer 타입은 프로젝트에 맞게 import/클래스명을 맞춰주세요.
import com.kAIS.KAIMyEntity.urdf.render.URDFRenderer;

import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiGraphics;
import net.minecraft.client.gui.components.AbstractSliderButton;
import net.minecraft.client.gui.components.Button;
import net.minecraft.client.gui.components.CycleButton;
import net.minecraft.client.gui.components.ObjectSelectionList;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;

import org.joml.Quaternionf;
import org.joml.Quaternionfc;
import org.joml.Vector3f;

import top.fifthlight.armorstand.vmc.VmcMarionetteManager;
import top.fifthlight.armorstand.vmc.VmcMarionetteStateView;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.function.Consumer;

/**
 * VMC ↔ URDF 매핑 에디터
 * - 좌: VMC 본 목록
 * - 우: URDF 조인트 목록
 * - 중앙: 선택된 매핑, Multiplier/Offset, Component/Mode, 매핑 리스트
 * - 하단: 테스트/저장/불러오기/닫기
 *
 * 프로젝트 내 MotionEditorScreen.java와 동일한 renderer, 폰트 렌더링 유틸을 재사용합니다.
 */
public class VMCMappingEditorScreen extends Screen {

    // === 고정 VMC 본 목록 ===
    private static final String[] VMC_BONES = {
            "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftShoulder", "LeftUpperArm", "LeftLowerArm", "LeftHand",
            "RightShoulder", "RightUpperArm", "RightLowerArm", "RightHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
    };

    // === 데이터 모델 ===
    public static class VMCMapping {
        public String vmcBone;
        public String urdfJoint;
        public float multiplier = 1.0f;
        public float offset = 0.0f;
        /**
         * position: "X","Y","Z"
         * orientation: "pitch","roll","yaw"
         */
        public String component = "Y";

        public enum ExtractionMode {
            EULER_X, EULER_Y, EULER_Z,
            QUATERNION_ANGLE
        }
        public ExtractionMode mode = ExtractionMode.EULER_Y;
    }

    private static class VMCMappingSet {
        public List<VMCMapping> mappings = new ArrayList<>();
    }

    // === 참조 ===
    private final Screen parent;
    private final URDFRenderer renderer;

    // === UI 위젯 ===
    private BoneList vmcList;
    private JointList jointList;
    private MappingList mappingList;

    private Button addOrUpdateBtn;
    private Button removeBtn;
    private Button testBtn;
    private Button saveBtn;
    private Button loadBtn;
    private Button closeBtn;

    private FloatSlider multiplierSlider;
    private FloatSlider offsetSlider;

    private CycleButton<String> componentCycle;
    private CycleButton<VMCMapping.ExtractionMode> modeCycle;

    // === 상태 ===
    private String selectedVmcBone = null;
    private String selectedUrdfJoint = null;
    private float currentMultiplier = 1.0f;
    private float currentOffset = 0.0f;
    private String currentComponent = "Y";
    private VMCMapping.ExtractionMode currentMode = VMCMapping.ExtractionMode.EULER_Y;

    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private String status = "";

    // 매핑을 이름 쌍으로 빠르게 찾기 위한 인덱스
    private final Map<String /*vmcBone|urdfJoint*/, VMCMapping> mappingIndex = new HashMap<>();

    public VMCMappingEditorScreen(Screen parent, URDFRenderer renderer) {
        super(Component.translatable("VMC-URDF Mapping Editor"));
        this.parent = parent;
        this.renderer = renderer;
    }

    @Override
    protected void init() {
        super.init();

        // 레이아웃 계산
        final int margin = 8;
        final int listTop = margin + 16;
        final int listHeight = this.height - listTop - 160;
        final int colWidth = (this.width - margin * 3) / 2; // 좌/우 2열
        final int leftX = margin;
        final int rightX = leftX + colWidth + margin;

        // === 좌측 VMC 목록 ===
        this.vmcList = new BoneList(this.minecraft, colWidth, listHeight, listTop, leftX);
        for (String s : VMC_BONES) {
            this.vmcList.addEntry(new BoneList.Entry(s, () -> {
                this.selectedVmcBone = s;
                updateAddButtonLabel();
            }));
        }
        this.addWidget(this.vmcList);

        // === 우측 URDF 조인트 목록 ===
        this.jointList = new JointList(this.minecraft, colWidth, listHeight, listTop, rightX);
        URDFRobotModel model = renderer.getRobotModel();
        if (model != null && model.joints != null) {
            for (URDFJoint j : model.joints) {
                final String name = j.name;
                this.jointList.addEntry(new JointList.Entry(name, () -> {
                    this.selectedUrdfJoint = name;
                    updateAddButtonLabel();
                }));
            }
        }
        this.addWidget(this.jointList);

        // === 중앙: 매핑 편집 영역 ===
        int y = listTop + listHeight + 6;

        // 선택된 매핑 텍스트는 render에서 그림

        // Component Cycle
        this.componentCycle = this.addRenderableWidget(
                CycleButton.<String>builder(s -> Component.literal(s))
                        .withValues("X", "Y", "Z", "pitch", "roll", "yaw")
                        .withInitialValue(this.currentComponent)
                        .create(margin, y, 120, 20, Component.literal("Component"),
                                (btn, value) -> {
                                    this.currentComponent = value;
                                })
        );

        // Mode Cycle
        this.modeCycle = this.addRenderableWidget(
                CycleButton.<VMCMapping.ExtractionMode>builder(mode -> Component.literal(mode.name()))
                        .withValues(VMCMapping.ExtractionMode.values())
                        .withInitialValue(this.currentMode)
                        .create(margin + 125, y, 160, 20, Component.literal("Mode"),
                                (btn, value) -> {
                                    this.currentMode = value;
                                })
        );

        // Multiplier Slider (-3.0 ~ 3.0)
        y += 24;
        this.multiplierSlider = new FloatSlider(margin, y, 200, 20, Component.literal("Multiplier"),
                -3.0f, 3.0f, this.currentMultiplier, v -> this.currentMultiplier = v);
        this.addRenderableWidget(this.multiplierSlider);

        // Offset Slider (-PI ~ +PI) [rad]
        this.offsetSlider = new FloatSlider(margin + 210, y, 220, 20, Component.literal("Offset [rad]"),
                (float) -Math.PI, (float) Math.PI, this.currentOffset, v -> this.currentOffset = v);
        this.addRenderableWidget(this.offsetSlider);

        // 매핑 리스트
        int mapListTop = y + 28;
        int mapListHeight = 90;
        this.mappingList = new MappingList(this.minecraft, this.width - margin * 2, mapListHeight, mapListTop, margin);
        this.addWidget(this.mappingList);

        // 하단 버튼들
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

        rebuildIndex(); // 로드 전 초기화
        updateAddButtonLabel();
    }

    private void updateAddButtonLabel() {
        boolean hasKey = findMapping(selectedVmcBone, selectedUrdfJoint) != null;
        if (this.addOrUpdateBtn != null) {
            this.addOrUpdateBtn.setMessage(Component.literal(hasKey ? "Update Mapping" : "Add Mapping"));
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
            this.mappingList.addEntry(new MappingList.Entry(m, this.mappingList));
            indexPut(m);
            this.status = "매핑 추가: " + m.vmcBone + " → " + m.urdfJoint;
        } else {
            exist.multiplier = currentMultiplier;
            exist.offset = currentOffset;
            exist.component = currentComponent;
            exist.mode = currentMode;
            this.mappingList.refreshText();
            this.status = "매핑 업데이트: " + exist.vmcBone + " → " + exist.urdfJoint;
        }
        updateAddButtonLabel();
    }

    private void onRemoveSelected() {
        MappingList.Entry sel = this.mappingList.getSelected();
        if (sel == null) {
            this.status = "삭제할 매핑을 목록에서 선택하세요.";
            return;
        }
        VMCMapping m = sel.mapping;
        this.mappingList.removeEntry(sel);
        indexRemove(m);
        this.status = "삭제됨: " + m.vmcBone + " → " + m.urdfJoint;
        updateAddButtonLabel();
    }

    private void onTest() {
        VmcMarionetteStateView state = VmcMarionetteManager.getState();
        if (state == null) {
            this.status = "VMC 상태가 없습니다 (연결 또는 실행 상태 확인).";
            return;
        }
        applyCurrentMappings(state);
        this.status = "테스트 적용 완료.";
    }

    private void onSave() {
        Path file = getMappingFile();
        VMCMappingSet set = new VMCMappingSet();
        for (MappingList.Entry e : this.mappingList.children()) {
            set.mappings.add(e.mapping);
        }
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
        if (!Files.exists(file)) {
            this.status = "파일 없음: " + file;
            return;
        }
        try {
            String json = Files.readString(file);
            VMCMappingSet set = gson.fromJson(json, VMCMappingSet.class);
            this.mappingList.clear();
            this.mappingIndex.clear();
            if (set != null && set.mappings != null) {
                for (VMCMapping m : set.mappings) {
                    this.mappingList.addEntry(new MappingList.Entry(m, this.mappingList));
                    indexPut(m);
                }
            }
            this.status = "불러오기 완료: " + file;
        } catch (IOException e) {
            this.status = "불러오기 실패: " + e.getMessage();
        }
    }

    private void onClose() {
        this.minecraft.setScreen(this.parent);
    }

    private Path getMappingFile() {
        String dir = renderer.GetModelDir(); // 프로젝트의 renderer API
        return Paths.get(dir, "vmc_mapping.json");
    }

    private void rebuildIndex() {
        this.mappingIndex.clear();
        for (MappingList.Entry e : this.mappingList.children()) {
            indexPut(e.mapping);
        }
    }

    private void indexPut(VMCMapping m) {
        this.mappingIndex.put(key(m.vmcBone, m.urdfJoint), m);
    }

    private void indexRemove(VMCMapping m) {
        this.mappingIndex.remove(key(m.vmcBone, m.urdfJoint));
    }

    private static String key(String vmc, String urdf) {
        return (vmc == null ? "" : vmc) + "|" + (urdf == null ? "" : urdf);
    }

    private VMCMapping findMapping(String vmc, String urdf) {
        return this.mappingIndex.get(key(vmc, urdf));
    }

    // === 렌더링 ===
    @Override
    public void render(GuiGraphics g, int mouseX, int mouseY, float partialTicks) {
        this.renderBackground(g);

        // 제목
        g.drawString(this.font, "VMC-URDF Mapping Editor", 8, 6, 0xFFFFFF, false);

        // 좌/우 구역 제목
        g.drawString(this.font, "VMC 본 목록", 8, 20, 0xFFD770, false);
        g.drawString(this.font, "URDF 조인트 목록", this.width / 2 + 4, 20, 0xFFD770, false);

        // 선택된 매핑 표시
        int infoY = this.vmcList.getBottom() + 6;
        String sel = "선택된 매핑: "
                + (selectedVmcBone == null ? "(VMC 미선택)" : selectedVmcBone)
                + " → "
                + (selectedUrdfJoint == null ? "(URDF 미선택)" : selectedUrdfJoint);
        g.drawString(this.font, sel, 8, infoY, 0xA0A0A0, false);

        // 현재 상태 메시지
        if (!this.status.isEmpty()) {
            g.drawString(this.font, this.status, 8, this.height - 28, 0x80FF80, false);
        }

        super.render(g, mouseX, mouseY, partialTicks);
        // 리스트들은 super.render에서 같이 렌더됨(ObjectSelectionList는 addWidget으로 등록)
    }

    // === 적용 로직 ===
    private void applyCurrentMappings(VmcMarionetteStateView vmcState) {
        if (vmcState == null || vmcState.boneTransforms == null) return;

        // boneTransforms: (tag, transform)
        // tag는 enum(HumanoidTag)일 가능성이 높음 → name()으로 String 비교
        // transform.rotation: Quaternionfc, transform.position: Vector3fc
        Map<String, VmcMarionetteStateView.Transform> byName = new HashMap<>();
        vmcState.boneTransforms.forEach((tag, transform) -> {
            String name = tag.toString();
            try {
                // 대부분 enum이면 name()이 더 정확
                name = (String) tag.getClass().getMethod("name").invoke(tag);
            } catch (Throwable ignored) {}
            byName.put(name, transform);
        });

        // 각 매핑에 대해 값을 추출해 URDF에 적용
        for (MappingList.Entry e : this.mappingList.children()) {
            VMCMapping m = e.mapping;
            VmcMarionetteStateView.Transform tr = byName.get(m.vmcBone);
            if (tr == null) continue;

            float value = extractValue(tr, m);
            value = applyMulOffsetClamp(value, m);
            renderer.setJointPreview(m.urdfJoint, value);
        }
    }

    private float extractValue(VmcMarionetteStateView.Transform tr, VMCMapping m) {
        // 위치형 component
        if ("X".equalsIgnoreCase(m.component) ||
            "Y".equalsIgnoreCase(m.component) ||
            "Z".equalsIgnoreCase(m.component)) {

            float v = 0f;
            switch (m.component.toUpperCase(Locale.ROOT)) {
                case "X": v = tr.position.x(); break;
                case "Y": v = tr.position.y(); break;
                case "Z": v = tr.position.z(); break;
            }
            return v;
        }

        // 회전형 component: pitch/roll/yaw
        Quaternionfc q = tr.rotation;
        // JOML 기준 Euler XYZ (X->Y->Z)로 꺼내 pitch=X, yaw=Y, roll=Z로 매핑
        Vector3f euler = new Vector3f();
        new Quaternionf(q).getEulerAnglesXYZ(euler);
        float pitch = euler.x; // rad
        float yaw   = euler.y;
        float roll  = euler.z;

        switch (m.mode) {
            case QUATERNION_ANGLE: {
                // angle = 2*acos(w)
                float w = q.w();
                double angle = 2.0 * Math.acos(Math.max(-1.0, Math.min(1.0, w)));
                return (float) angle;
            }
            case EULER_X: return pitch;
            case EULER_Y: return yaw;
            case EULER_Z: return roll;
        }

        // component로 직접 지정된 경우 우선
        switch (m.component.toLowerCase(Locale.ROOT)) {
            case "pitch": return pitch;
            case "roll":  return roll;
            case "yaw":   return yaw;
        }
        return 0f;
    }

    private float applyMulOffsetClamp(float v, VMCMapping m) {
        float out = v * m.multiplier + m.offset;
        // 조인트 제한에 맞춰 clamp (가능시)
        URDFRobotModel model = renderer.getRobotModel();
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

    // === 리스트들 ===

    /** VMC 본 선택 리스트 */
    private static class BoneList extends ObjectSelectionList<BoneList.Entry> {
        final int left;
        public BoneList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height, 18);
            this.left = left;
            this.setRenderBackground(false);
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }
        @Override public int getX() { return this.left; }

        public static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final String name;
            private final Runnable onSelect;
            public Entry(String name, Runnable onSelect) {
                this.name = name; this.onSelect = onSelect;
            }
            @Override
            public void render(GuiGraphics g, int idx, int top, int left, int width, int height,
                               int mouseX, int mouseY, boolean hovered, float partialTicks) {
                g.drawString(Minecraft.getInstance().font, name, left + 2, top + 4, 0xFFFFFF, false);
            }
            @Override
            public boolean mouseClicked(double mouseX, double mouseY, int button) {
                if (button == 0) {
                    this.onSelect.run();
                    return true;
                }
                return false;
            }
            @Override public Component getNarration() { return Component.literal(name); }
        }
    }

    /** URDF 조인트 선택 리스트 */
    private static class JointList extends ObjectSelectionList<JointList.Entry> {
        final int left;
        public JointList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height, 18);
            this.left = left;
            this.setRenderBackground(false);
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }
        @Override public int getX() { return this.left; }

        public static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final String name;
            private final Runnable onSelect;
            public Entry(String name, Runnable onSelect) {
                this.name = name; this.onSelect = onSelect;
            }
            @Override
            public void render(GuiGraphics g, int idx, int top, int left, int width, int height,
                               int mouseX, int mouseY, boolean hovered, float partialTicks) {
                g.drawString(Minecraft.getInstance().font, name, left + 2, top + 4, 0xFFFFFF, false);
            }
            @Override
            public boolean mouseClicked(double mouseX, double mouseY, int button) {
                if (button == 0) {
                    this.onSelect.run();
                    return true;
                }
                return false;
            }
            @Override public Component getNarration() { return Component.literal(name); }
        }
    }

    /** 매핑 목록(선택 시 편집값에 로드) */
    private static class MappingList extends ObjectSelectionList<MappingList.Entry> {
        final int left;
        public MappingList(Minecraft mc, int width, int height, int top, int left) {
            super(mc, width, height, top, top + height, 18);
            this.left = left;
            this.setRenderBackground(false);
        }
        @Override public int getRowWidth() { return this.width - 8; }
        @Override public int getRowLeft() { return this.left + 4; }
        @Override public int getX() { return this.left; }

        public Entry getSelected() {
            return this.getSelected() != null ? this.getSelected() : null;
        }

        public void refreshText() {
            // no-op: 각 엔트리는 render 시점에 mapping을 읽어 출력하므로 별도 동기화 불필요
        }

        public static class Entry extends ObjectSelectionList.Entry<Entry> {
            private final MappingList owner;
            public final VMCMapping mapping;
            public Entry(VMCMapping mapping, MappingList owner) {
                this.mapping = mapping; this.owner = owner;
            }
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
            public boolean mouseClicked(double mouseX, double mouseY, int button) {
                if (button == 0) {
                    this.owner.setSelected(this);
                    // 선택된 엔트리를 편집 영역에 로드
                    VMCMappingEditorScreen scr = (VMCMappingEditorScreen) Minecraft.getInstance().screen;
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

    // === 범용 float 슬라이더 ===
    private static class FloatSlider extends AbstractSliderButton {
        private final float min, max;
        private final Consumer<Float> onChange;
        public FloatSlider(int x, int y, int w, int h, Component title, float min, float max, float initial, Consumer<Float> onChange) {
            super(x, y, w, h, title, 0.0D);
            this.min = min; this.max = max; this.onChange = onChange;
            setValueImmediately(initial);
        }
        public void setValueImmediately(float value) {
            this.value = toSlider(value);
            applyValue();
            updateMessage();
        }
        private double toSlider(float v) { // -> 0..1
            return (v - min) / (double)(max - min);
        }
        private float fromSlider(double s) { // 0..1 -> value
            return (float)(min + s * (max - min));
        }
        @Override
        protected void updateMessage() {
            float v = fromSlider(this.value);
            this.setMessage(Component.literal(this.getMessage().getString().split(":")[0] + ": " + String.format(Locale.ROOT, "%.3f", v)));
        }
        @Override
        protected void applyValue() {
            float v = fromSlider(this.value);
            this.onChange.accept(v);
        }
    }
}
