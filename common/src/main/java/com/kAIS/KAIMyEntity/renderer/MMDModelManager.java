package com.kAIS.KAIMyEntity.renderer;

import com.kAIS.KAIMyEntity.KAIMyEntityClient;
import com.kAIS.KAIMyEntity.NativeFunc;
import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGL;  // 추가
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import net.minecraft.client.Minecraft;

public class MMDModelManager {
    static final Logger logger = LogManager.getLogger();
    static final Minecraft MCinstance = Minecraft.getInstance();
    static Map<String, Model> models;
    static String gameDirectory = MCinstance.gameDirectory.getAbsolutePath();

    public static void Init() {
        models = new HashMap<>();
        logger.info("MMDModelManager.Init() finished");
    }

    public static IMMDModel LoadModel(String modelName, long layerCount) {
        //Model path
        File modelDir = new File(gameDirectory + "/KAIMyEntity/" + modelName);
        String modelDirStr = modelDir.getAbsolutePath();

        // === URDF 지원 추가 ===
        // URDF 파일이 있으면 URDF로 로드
        File urdfModelFilename = new File(modelDir, "robot.urdf");
        if (urdfModelFilename.isFile()) {
            logger.info("Found URDF file for " + modelName + ", loading as URDF model...");
            try {
                IMMDModel urdfModel = URDFModelOpenGL.Create(
                    urdfModelFilename.getAbsolutePath(), 
                    modelDirStr
                );
                if (urdfModel != null) {
                    logger.info("✓ Successfully loaded URDF model: " + modelName);
                    return urdfModel;
                } else {
                    logger.warn("Failed to load URDF model, will try PMX/PMD");
                }
            } catch (Exception e) {
                logger.error("Error loading URDF model: " + e.getMessage(), e);
                logger.warn("Falling back to PMX/PMD");
            }
        }
        // === URDF 지원 끝 ===

        // 기존 PMX/PMD 로딩 로직 (그대로 유지)
        String modelFilenameStr;
        boolean isPMD;
        File pmxModelFilename = new File(modelDir, "model.pmx");
        if (pmxModelFilename.isFile()) {
            modelFilenameStr = pmxModelFilename.getAbsolutePath();
            isPMD = false;
        } else {
            File pmdModelFilename = new File(modelDir, "model.pmd");
            if (pmdModelFilename.isFile()) {
                modelFilenameStr = pmdModelFilename.getAbsolutePath();
                isPMD = true;
            } else {
                return null;
            }
        }
        return MMDModelOpenGL.Create(modelFilenameStr, modelDirStr, isPMD, layerCount);
    }

    public static Model GetModel(String modelName, String uuid) {
        Model model = models.get(modelName + uuid);
        if (model == null) {
            IMMDModel m = LoadModel(modelName, 3);
            if (m == null)
                return null;
            MMDAnimManager.AddModel(m);
            AddModel(modelName + uuid, m, modelName);
            model = models.get(modelName + uuid);
        }
        return model;
    }

    public static Model GetModel(String modelName){
        return GetModel(modelName, "");
    }

    public static void AddModel(String Name, IMMDModel model, String modelName) {
        NativeFunc nf = NativeFunc.GetInst();
        EntityData ed = new EntityData();
        ed.stateLayers = new EntityData.EntityState[3];
        ed.playCustomAnim = false;
        ed.rightHandMat = nf.CreateMat();
        ed.leftHandMat = nf.CreateMat();
        ed.matBuffer = ByteBuffer.allocateDirect(64); //float * 16

        ModelWithEntityData m = new ModelWithEntityData();
        m.entityName = Name;
        m.model = model;
        m.modelName = modelName;
        m.entityData = ed;
        model.ResetPhysics();
        model.ChangeAnim(MMDAnimManager.GetAnimModel(model, "idle"), 0);
        models.put(Name, m);
    }

    public static void ReloadModel() {
        for (Model i : models.values())
            DeleteModel(i);
        models = new HashMap<>();
    }

    static void DeleteModel(Model model) {
        MMDModelOpenGL.Delete((MMDModelOpenGL) model.model);

        //Unregister animation user
        MMDAnimManager.DeleteModel(model.model);
    }

    // === URDF 헬퍼 메서드 추가 ===
    public static URDFModelOpenGL GetURDFModel(String modelName) {
        Model m = GetModel(modelName);
        if (m != null && m.model instanceof URDFModelOpenGL) {
            return (URDFModelOpenGL) m.model;
        }
        return null;
    }

    public static void UpdateURDFJoints(String modelName, Map<String, Float> jointPositions) {
        URDFModelOpenGL urdfModel = GetURDFModel(modelName);
        if (urdfModel != null) {
            urdfModel.updateJointPositions(jointPositions);
        }
    }
    // === URDF 헬퍼 메서드 끝 ===

    public static class Model {
        public IMMDModel model;
        String entityName;
        String modelName;
        public Properties properties = new Properties();
        boolean isPropertiesLoaded = false;

        public void loadModelProperties(boolean forceReload){
            if (isPropertiesLoaded && !forceReload)
                return;
            String path2Properties = gameDirectory + "/KAIMyEntity/" + modelName + "/model.properties";
            try {
                InputStream istream = new FileInputStream(path2Properties);
                properties.load(istream);
            } catch (IOException e) {
                logger.warn( "KAIMyEntity/" + modelName + "/model.properties not found" );
            }
            isPropertiesLoaded = true;
            KAIMyEntityClient.reloadProperties = false;
        } 
    }

    public static class ModelWithEntityData extends Model {
        public EntityData entityData;
    }

    public static class EntityData {
        public static HashMap<EntityState, String> stateProperty = new HashMap<>() {{
            put(EntityState.Idle, "idle");
            put(EntityState.Walk, "walk");
            put(EntityState.Sprint, "sprint");
            put(EntityState.Air, "air");
            put(EntityState.OnClimbable, "onClimbable");
            put(EntityState.OnClimbableUp, "onClimbableUp");
            put(EntityState.OnClimbableDown, "onClimbableDown");
            put(EntityState.Swim, "swim");
            put(EntityState.Ride, "ride");
            put(EntityState.Ridden, "ridden");
            put(EntityState.Driven, "driven");
            put(EntityState.Sleep, "sleep");
            put(EntityState.ElytraFly, "elytraFly");
            put(EntityState.Die, "die");
            put(EntityState.SwingRight, "swingRight");
            put(EntityState.SwingLeft, "swingLeft");
            put(EntityState.Sneak, "sneak");
            put(EntityState.OnHorse, "onHorse");
            put(EntityState.Crawl, "crawl");
            put(EntityState.LieDown, "lieDown");
        }};
        public boolean playCustomAnim; //Custom animation played in layer 0.
        public long rightHandMat, leftHandMat;
        public EntityState[] stateLayers;
        ByteBuffer matBuffer;

        public enum EntityState {Idle, Walk, Sprint, Air, OnClimbable, OnClimbableUp, OnClimbableDown, Swim, Ride, Ridden, Driven, Sleep, ElytraFly, Die, SwingRight, SwingLeft, ItemRight, ItemLeft, Sneak, OnHorse, Crawl, LieDown}
    }
}
