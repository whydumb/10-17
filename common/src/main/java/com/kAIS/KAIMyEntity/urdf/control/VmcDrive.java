package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL;
import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.util.HashMap;
import java.util.Map;

import static java.lang.Math.abs;

final class VmcDrive {

    private static final VMCListenerController.VmcListener.Transform TMP_CHEST = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_NECK  = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_HEAD  = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_LUA   = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_LLA   = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_RUA   = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_RLA   = new VMCListenerController.VmcListener.Transform();

    private static final Map<String, Float> LOCAL_FRAME = new HashMap<>();

    // ============================================
    // 🎯 URDF 축 정의 (Chest 프레임 기준)
    // ============================================

    // Left Shoulder (URDF 기반 계산된 축)
    private static final Vector3f L_PITCH_AXIS = new Vector3f(0,  1, 0); // +Y
    private static final Vector3f L_ROLL_AXIS0 = new Vector3f(-1, 0, 0); // -X

    // Right Shoulder
    private static final Vector3f R_PITCH_AXIS = new Vector3f(0, -1, 0); // -Y
    private static final Vector3f R_ROLL_AXIS0 = new Vector3f(1,  0, 0); // +X

    // ============================================
    // 캘리브레이션용 레스트 포즈 (T-pose 기준)
    // ============================================
    private static Quaternionf leftShoulderRest = null;
    private static Quaternionf rightShoulderRest = null;

    /**
     * ✅ 수정: T-pose 캘리브레이션 (URDF 좌표계로 변환 후 저장)
     * @param renderer URDF 렌더러 (좌표계 변환용)
     * @param chestSrc tracking 원본 chest
     * @param leftUpperArmSrc tracking 원본 left upper arm
     * @param rightUpperArmSrc tracking 원본 right upper arm
     */
    public static void calibrateTPose(
            URDFModelOpenGLWithSTL renderer,
            VMCListenerController.VmcListener.Transform chestSrc,
            VMCListenerController.VmcListener.Transform leftUpperArmSrc,
            VMCListenerController.VmcListener.Transform rightUpperArmSrc
    ) {
        // ✅ URDF 좌표계로 변환 (tick과 동일한 basis 사용)
        var chest = toUrdf(renderer, chestSrc, TMP_CHEST);

        if (leftUpperArmSrc != null) {
            var leftUpperArm = toUrdf(renderer, leftUpperArmSrc, TMP_LUA);
            leftShoulderRest = new Quaternionf(chest.rotation).conjugate()
                    .mul(leftUpperArm.rotation).normalize();
            System.out.println("✅ Left shoulder T-pose calibrated");
        }
        if (rightUpperArmSrc != null) {
            var rightUpperArm = toUrdf(renderer, rightUpperArmSrc, TMP_RUA);
            rightShoulderRest = new Quaternionf(chest.rotation).conjugate()
                    .mul(rightUpperArm.rotation).normalize();
            System.out.println("✅ Right shoulder T-pose calibrated");
        }
    }

    // ============================================
    // Main tick
    // ============================================

    private static VMCListenerController.VmcListener.Transform toUrdf(
            URDFModelOpenGLWithSTL renderer,
            VMCListenerController.VmcListener.Transform src,
            VMCListenerController.VmcListener.Transform dst
    ) {
        if (src == null) return null;
        renderer.trackingRotToUrdf(src.rotation, dst.rotation);
        return dst;
    }

    static void tick(URDFModelOpenGLWithSTL renderer, Map<String, Float> outFrame) {
        var listener = VMCListenerController.VmcListener.getInstance();
        Map<String, VMCListenerController.VmcListener.Transform> bones = listener.getSnapshot();
        if (bones.isEmpty()) return;

        var chestSrc = bones.get("Chest");
        if (chestSrc == null) chestSrc = bones.get("Spine");
        if (chestSrc == null) chestSrc = bones.get("Hips");
        if (chestSrc == null) return;

        var chest = toUrdf(renderer, chestSrc, TMP_CHEST);

        Map<String, Float> frame = (outFrame != null) ? outFrame : LOCAL_FRAME;
        frame.clear();

        processArm(renderer, frame, bones, chest, true);
        processArm(renderer, frame, bones, chest, false);
        processHead(renderer, frame, bones, chest);
    }

    // ============================================
    // 🦾 팔 처리 (✅ Clamp 타이밍 수정)
    // ============================================

    private static void processArm(
            URDFModelOpenGLWithSTL renderer,
            Map<String, Float> frame,
            Map<String, VMCListenerController.VmcListener.Transform> bones,
            VMCListenerController.VmcListener.Transform parentBone,
            boolean isLeft
    ) {
        String upperName = isLeft ? "LeftUpperArm" : "RightUpperArm";
        String lowerName = isLeft ? "LeftLowerArm" : "RightLowerArm";

        var upperSrc = bones.get(upperName);
        if (upperSrc == null) return;

        var upper = toUrdf(renderer, upperSrc, isLeft ? TMP_LUA : TMP_RUA);

        // === 1) Shoulder: 정규화 + 캘리브레이션 적용 ===
        Quaternionf parentRot = new Quaternionf(parentBone.rotation).normalize();
        Quaternionf childRot  = new Quaternionf(upper.rotation).normalize();
        Quaternionf localShoulder = new Quaternionf(parentRot).conjugate().mul(childRot).normalize();

        // 레스트 포즈 보정 적용
        Quaternionf rest = isLeft ? leftShoulderRest : rightShoulderRest;
        if (rest != null) {
            localShoulder = new Quaternionf(rest).conjugate().mul(localShoulder).normalize();
        }

        String pitchJoint = isLeft ? "l_sho_pitch" : "r_sho_pitch";
        String rollJoint  = isLeft ? "l_sho_roll"  : "r_sho_roll";

        // === URDF 축 선택 ===
        Vector3f axisPitch = isLeft ? new Vector3f(L_PITCH_AXIS) : new Vector3f(R_PITCH_AXIS);
        Vector3f axisRoll0 = isLeft ? new Vector3f(L_ROLL_AXIS0) : new Vector3f(R_ROLL_AXIS0);

        // === ✅ 2) Pitch 추출 → 즉시 Clamp ===
        float pitchRaw = signedTwistAngle(localShoulder, axisPitch);
        float pitch = clampJoint(pitchJoint, pitchRaw);

        // === ✅ 3) Clamped pitch로 qPitch 생성 → 잔여 회전 계산 ===
        Quaternionf qPitch = new Quaternionf().rotationAxis(pitch, axisPitch.x, axisPitch.y, axisPitch.z);
        Quaternionf qRem = new Quaternionf(qPitch).conjugate().mul(localShoulder).normalize();

        // === 4) Roll 축 회전 ===
        Vector3f axisRoll = new Vector3f(axisRoll0);
        qPitch.transform(axisRoll);

        // === ✅ 5) Roll 추출 → 즉시 Clamp ===
        float rollRaw = signedTwistAngle(qRem, axisRoll);
        float roll = clampJoint(rollJoint, rollRaw);

        // === URDF에 적용 ===
        renderer.setJointPreview(pitchJoint, pitch);
        renderer.setJointPreview(rollJoint, roll);
        frame.put(pitchJoint, pitch);
        frame.put(rollJoint, roll);

        // === 디버그 로그 (2초마다) ===
        if (System.currentTimeMillis() % 2000 < 50) {
            System.out.printf("[%s SHOULDER] pitch=%.3f (%.1f°), roll=%.3f (%.1f°)%n",
                    isLeft ? "LEFT" : "RIGHT",
                    pitch, Math.toDegrees(pitch),
                    roll, Math.toDegrees(roll)
            );
        }

        // === Elbow: UpperArm → LowerArm (Z축 twist) ===
        var lowerSrc = bones.get(lowerName);
        if (lowerSrc != null) {
            var lower = toUrdf(renderer, lowerSrc, isLeft ? TMP_LLA : TMP_RLA);

            Quaternionf upperRot = new Quaternionf(upper.rotation).normalize();
            Quaternionf lowerRot = new Quaternionf(lower.rotation).normalize();
            Quaternionf localElbow = new Quaternionf(upperRot).conjugate().mul(lowerRot).normalize();

            float angleZ = twistAngleAroundAxis(localElbow, 0f, 0f, 1f);
            float elbowAngle = isLeft ? -abs(angleZ) : abs(angleZ);

            String elbowJoint = isLeft ? "l_el" : "r_el";
            elbowAngle = clampJoint(elbowJoint, elbowAngle);

            renderer.setJointPreview(elbowJoint, elbowAngle);
            frame.put(elbowJoint, elbowAngle);
        }
    }

    // ============================================
    // 🗣️ 머리 처리 (✅ Clamp 타이밍 수정)
    // ============================================

    private static void processHead(
            URDFModelOpenGLWithSTL renderer,
            Map<String, Float> frame,
            Map<String, VMCListenerController.VmcListener.Transform> bones,
            VMCListenerController.VmcListener.Transform chest
    ) {
        var headSrc = bones.get("Head");
        if (headSrc == null) return;
        var head = toUrdf(renderer, headSrc, TMP_HEAD);

        var neckSrc = bones.get("Neck");
        var neck = (neckSrc != null) ? toUrdf(renderer, neckSrc, TMP_NECK) : null;
        var parent = (neck != null) ? neck : chest;

        Quaternionf parentRot = new Quaternionf(parent.rotation).normalize();
        Quaternionf headRot = new Quaternionf(head.rotation).normalize();
        Quaternionf localHead = new Quaternionf(parentRot).conjugate().mul(headRot).normalize();

        // === ✅ 1) Pan 추출 → 즉시 Clamp ===
        float panRaw = twistAngleAroundAxis(localHead, 0f, 1f, 0f);
        float pan = clampJoint("head_pan", panRaw);

        // === ✅ 2) Clamped pan으로 제거 → Tilt 계산 ===
        Quaternionf qPan = new Quaternionf().rotationAxis(pan, 0f, 1f, 0f);
        Quaternionf noPan = new Quaternionf(qPan).conjugate().mul(localHead).normalize();

        // === ✅ 3) Tilt 추출 → 즉시 Clamp ===
        float tiltRaw = twistAngleAroundAxis(noPan, 1f, 0f, 0f);
        float tilt = clampJoint("head_tilt", tiltRaw);

        renderer.setJointPreview("head_pan", pan);
        renderer.setJointPreview("head_tilt", tilt);
        frame.put("head_pan", pan);
        frame.put("head_tilt", tilt);
    }

    // ============================================
    // 🧮 안정화된 Twist 추출 유틸리티
    // ============================================

    /**
     * ✅ 개선: extractTwist 기반 안정 각도 계산
     */
    private static float signedTwistAngle(Quaternionf q, Vector3f axis) {
        Vector3f a = new Vector3f(axis).normalize();
        Quaternionf qNorm = new Quaternionf(q).normalize();
        Quaternionf t = extractTwist(qNorm, a);

        // angle magnitude
        float vecLen = (float)Math.sqrt(t.x*t.x + t.y*t.y + t.z*t.z);
        float angle = 2f * (float)Math.atan2(vecLen, t.w);

        // sign (twist 벡터가 axis와 같은 방향이면 +)
        float dot = t.x*a.x + t.y*a.y + t.z*a.z;
        if (dot < 0) angle = -angle;

        // wrap to [-π, π]
        while (angle > Math.PI) angle -= 2f * (float)Math.PI;
        while (angle < -Math.PI) angle += 2f * (float)Math.PI;

        return angle;
    }

    /**
     * q에서 axis 방향의 twist만 추출
     */
    private static Quaternionf extractTwist(Quaternionf q, Vector3f axisUnit) {
        Vector3f a = new Vector3f(axisUnit).normalize();
        float d = q.x * a.x + q.y * a.y + q.z * a.z;
        return new Quaternionf(a.x * d, a.y * d, a.z * d, q.w).normalize();
    }

    /**
     * 단순 twist (하위 호환용)
     */
    private static float twistAngleAroundAxis(Quaternionf q, float ux, float uy, float uz) {
        return signedTwistAngle(q, new Vector3f(ux, uy, uz));
    }

    // ============================================
    // ⚙️ Joint Limit Clamp
    // ============================================

    private static final Map<String, float[]> JOINT_LIMITS = new HashMap<>();
    static {
        // 어깨 (라디안)
        JOINT_LIMITS.put("l_sho_pitch", new float[]{-1.57f, 0.52f});
        JOINT_LIMITS.put("r_sho_pitch", new float[]{-1.57f, 0.52f});
        JOINT_LIMITS.put("l_sho_roll",  new float[]{-2.25f, 0.15f});
        JOINT_LIMITS.put("r_sho_roll",  new float[]{-0.15f, 2.30f});

        // 팔꿈치
        JOINT_LIMITS.put("l_el", new float[]{-2.7925f, 0.0f});
        JOINT_LIMITS.put("r_el", new float[]{0.0f, 2.7925f});

        // 머리
        JOINT_LIMITS.put("head_pan",  new float[]{-1.57f, 1.57f});
        JOINT_LIMITS.put("head_tilt", new float[]{-0.52f, 0.52f});
    }

    private static float clampJoint(String name, float value) {
        float[] limits = JOINT_LIMITS.get(name);
        if (limits == null) return value;
        return Math.max(limits[0], Math.min(limits[1], value));
    }
}