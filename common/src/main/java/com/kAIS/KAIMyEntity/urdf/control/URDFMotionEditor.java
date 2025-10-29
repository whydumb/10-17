package com.kAIS.KAIMyEntity.urdf.control;

import com.kAIS.KAIMyEntity.urdf.URDFJoint;
import com.kAIS.KAIMyEntity.urdf.URDFRobotModel;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;

public final class URDFMotionEditor {
    private final URDFRobotModel model;
    private final URDFSimpleController ctrl;
    private final URDFMotion motion = new URDFMotion();
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    public URDFMotionEditor(URDFRobotModel model, URDFSimpleController ctrl) {
        this.model = model; this.ctrl = ctrl;
    }

    /** 현재 자세를 timeSec 키프레임으로 캡처 */
    public void addKeyframe(float timeSec) {
        URDFMotion.Key k = new URDFMotion.Key();
        k.t = timeSec;
        for (URDFJoint j : model.joints) {
            k.pose.put(j.name, j.currentPosition);
        }
        motion.keys.add(k);
        motion.keys.sort(Comparator.comparing(a -> a.t));
    }

    public void setInterp(int idx, String interp) {
        if (idx < 0 || idx >= motion.keys.size()) return;
        motion.keys.get(idx).interp = interp;
    }

    public void moveKey(int idx, float newTime) {
        if (idx < 0 || idx >= motion.keys.size()) return;
        motion.keys.get(idx).t = newTime;
        motion.keys.sort(Comparator.comparing(a -> a.t));
    }

    public void removeKey(int idx) {
        if (idx < 0 || idx >= motion.keys.size()) return;
        motion.keys.remove(idx);
    }

    public void save(Path path) throws IOException {
        String json = gson.toJson(motion);
        Files.writeString(path, json);
    }

    public void load(Path path) throws IOException {
        String json = Files.readString(path);
        URDFMotion m = new Gson().fromJson(json, URDFMotion.class);
        motion.name = m.name; motion.fps = m.fps; motion.loop = m.loop;
        motion.keys.clear(); motion.keys.addAll(m.keys);
        motion.keys.sort(Comparator.comparing(a -> a.t));
    }

    public URDFMotion getMotion() { return motion; }
}
