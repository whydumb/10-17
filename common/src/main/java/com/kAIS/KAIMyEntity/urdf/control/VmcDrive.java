package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGLWithSTL;
import org.joml.Quaternionf;
import org.joml.Vector3f;

import java.util.HashMap;
import java.util.Map;

import static java.lang.Math.abs;

final class VmcDrive {

    // 변환용 scratch Transform들
    private static final VMCListenerController.VmcListener.Transform TMP_CHEST = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_NECK  = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_HEAD  = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_LUA   = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_LLA   = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_RUA   = new VMCListenerController.VmcListener.Transform();
    private static final VMCListenerController.VmcListener.Transform TMP_RLA   = new VMCListenerController.VmcListener.Transform();

    // outFrame이 null일 때 사용할 scratch 맵 (GC 방지)
    private static final Map<String, Float> LOCAL_FRAME = new HashMap<>();

    /**
     * VMC tracking 좌표계 → URDF 좌표계 변환 (rotation만 - position은 현재 미사용)
     */
    private static VMCListenerController.VmcListener.Transform toUrdf(
            URDFModelOpenGLWithSTL renderer,
            VMCListenerController.VmcListener.Transform src,
            VMCListenerController.VmcListener.Transform dst
    ) {
        if (src == null) return null;
        // position 변환 제거 - 현재 rotation만 사용하므로 불필요한 연산 제거
        renderer.trackingRotToUrdf(src.rotation, dst.rotation);
        return dst;
    }

    static void tick(URDFModelOpenGLWithSTL renderer, Map<String, Float> outFrame) {
        var listener = VMCListenerController.VmcListener.getInstance();

        Map<String, VMCListenerController.VmcListener.Transform> bones = listener.getSnapshot();
        if (bones.isEmpty()) return;

        // Chest/Spine/Hips 중 하나 찾기
        var chestSrc = bones.get("Chest");
        if (chestSrc == null) chestSrc = bones.get("Spine");
        if (chestSrc == null) chestSrc = bones.get("Hips");
        if (chestSrc == null) return;

        // URDF 좌표계로 변환
        var chest = toUrdf(renderer, chestSrc, TMP_CHEST);

        // outFrame이 null이면 내부 scratch 사용 (GC 방지)
        Map<String, Float> frame = (outFrame != null) ? outFrame : LOCAL_FRAME;
        frame.clear();

        // 팔
        processArm(renderer, frame, bones, chest, true);
        processArm(renderer, frame, bones, chest, false);

        // 머리
        processHead(renderer, frame, bones, chest);
    }

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

        Quaternionf parentRot = new Quaternionf(parent.rotation);
        Quaternionf headRot = new Quaternionf(head.rotation);

        // local = parent^-1 * child
        Quaternionf localHead = new Quaternionf(parentRot).conjugate().mul(headRot);

        // Euler 대신 twist 기반 pan/tilt 추출 (안정적)
        float pan = twistAngleAroundAxis(localHead, 0f, 1f, 0f);

        Quaternionf qPan = new Quaternionf().rotateY(pan);
        Quaternionf noPan = new Quaternionf(qPan).conjugate().mul(localHead);

        float tilt = twistAngleAroundAxis(noPan, 1f, 0f, 0f);

        renderer.setJointPreview("head_pan", pan);
        renderer.setJointPreview("head_tilt", tilt);

        frame.put("head_pan", pan);
        frame.put("head_tilt", tilt);
    }

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

        // URDF 좌표계로 변환
        var upper = toUrdf(renderer, upperSrc, isLeft ? TMP_LUA : TMP_RUA);

        // 어깨: Chest 기준 UpperArm 로컬
        Quaternionf parentRot = new Quaternionf(parentBone.rotation);
        Quaternionf childRot  = new Quaternionf(upper.rotation);
        Quaternionf localShoulder = new Quaternionf(parentRot).conjugate().mul(childRot);

        Vector3f shoulderEuler = new Vector3f();
        localShoulder.getEulerAnglesXYZ(shoulderEuler);

        String pitchJoint = isLeft ? "l_sho_pitch" : "r_sho_pitch";
        String rollJoint  = isLeft ? "l_sho_roll"  : "r_sho_roll";

        renderer.setJointPreview(pitchJoint, shoulderEuler.x);
        renderer.setJointPreview(rollJoint, shoulderEuler.z);

        frame.put(pitchJoint, shoulderEuler.x);
        frame.put(rollJoint, shoulderEuler.z);

        // 팔꿈치: UpperArm 기준 LowerArm 로컬 -> Z축 twist
        var lowerSrc = bones.get(lowerName);
        if (lowerSrc != null) {
            var lower = toUrdf(renderer, lowerSrc, isLeft ? TMP_LLA : TMP_RLA);

            Quaternionf upperRot = new Quaternionf(upper.rotation);
            Quaternionf lowerRot = new Quaternionf(lower.rotation);
            Quaternionf localElbow = new Quaternionf(upperRot).conjugate().mul(lowerRot);

            float angleZ = twistAngleAroundAxis(localElbow, 0f, 0f, 1f);
            float elbowAngle = isLeft ? -abs(angleZ) : abs(angleZ);

            String elbowJoint = isLeft ? "l_el" : "r_el";
            renderer.setJointPreview(elbowJoint, elbowAngle);
            frame.put(elbowJoint, elbowAngle);
        }
    }

    // q=(x,y,z,w), axis u=(ux,uy,uz). return signed angle (-pi..pi)
    private static float twistAngleAroundAxis(Quaternionf q, float ux, float uy, float uz) {
        float norm = (float) Math.sqrt(ux*ux + uy*uy + uz*uz);
        if (norm == 0f) return 0f;
        ux /= norm; uy /= norm; uz /= norm;

        float qw = q.w, qx = q.x, qy = q.y, qz = q.z;
        float dot = qx*ux + qy*uy + qz*uz;
        float angle = 2f * (float) Math.atan2(dot, qw);

        if (angle > Math.PI) angle -= (float)(2*Math.PI);
        if (angle < -Math.PI) angle += (float)(2*Math.PI);
        return angle;
    }
}