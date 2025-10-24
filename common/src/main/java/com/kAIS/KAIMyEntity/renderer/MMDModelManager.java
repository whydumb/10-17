package com.kAIS.KAIMyEntity.renderer;

import com.kAIS.KAIMyEntity.KAIMyEntityClient;
import com.kAIS.KAIMyEntity.NativeFunc;
import com.kAIS.KAIMyEntity.urdf.URDFModelOpenGL;
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

    /**
     * 모델 로딩 - URDF, PMX, PMD 순서로 시도
     */
    public static IMMDModel LoadModel(String modelName, long layerCount) {
        File modelDir = new File(gameDirectory + "/KAIMyEntity/" + modelName);
        String modelDirStr = modelDir.getAbsolutePath();

        if (!modelDir.exists()) {
            logger.error("✗ Model directory not found: " + modelDirStr);
            return null;
        }

        // === 1. URDF 우선 시도 ===
        File urdfFile = new File(modelDir, "robot.urdf");
        if (urdfFile.isFile()) {
            logger.info("Found URDF file for " + modelName + ", loading...");
            try {
                IMMDModel urdfModel = URDFModelOpenGL.Create(
                    urdfFile.getAbsolutePath(), 
                    modelDirStr
                );
                
                if (urdfModel != null) {
                    logger.info("✓ URDF model loaded successfully: " + modelName);
                    return urdfModel;
                } else {
                    logger.warn("✗ URDF model loading returned null");
                }
            } catch (Exception e) {
                logger.error("✗ Exception loading URDF model: " + e.getMessage(), e);
            }
            logger.warn("Falling back to PMX/PMD");
        }

        // === 2. PMX 시도 ===
        File pmxFile = new File(modelDir, "model.pmx");
        if (pmxFile.isFile()) {
            logger.info("Found PMX file for " + modelName);
            IMMDModel mmdModel = MMDModelOpenGL.Create(
                pmxFile.getAbsolutePath(), 
                modelDirStr, 
                false, // isPMD = false
                layerCount
            );
            if (mmdModel != null) {
                logger.info("✓ PMX model loaded successfully: " + modelName);
                return mmdModel;
            }
        }

        // === 3. PMD 시도 ===
        File pmdFile = new File(modelDir, "model.pmd");
        if (pmdFile.isFile()) {
            logger.info("Found PMD file for " + modelName);
            IMMDModel mmdModel = MMDModelOpenGL.Create(
                pmdFile.getAbsolutePath(), 
                modelDirStr, 
                true, // isPMD = true
                layerCount
            );
            if (mmdModel != null) {
                logger.info("✓ PMD model loaded successfully: " + modelName);
                return mmdModel;
            }
        }

        logger.error("✗ No valid model found in " + modelDirStr);
        logger.error("  Looked for: robot.urdf, model.pmx, model.pmd");
        return null;
    }

    /**
     * 모델 가져오기 (캐시 포함)
     */
    public static Model GetModel(String modelName, String uuid) {
        String fullName = modelName + uuid;
        Model model = models.get(fullName);
        
        if (model == null) {
            IMMDModel m = LoadModel(modelName, 3);
            if (m == null) {
                return null;
            }

            // ✓ 모델 타입별 등록
            if (m instanceof URDFModelOpenGL) {
                RegisterURDFModel(fullName, m, modelName);
            } else if (m instanceof MMDModelOpenGL) {
                RegisterMMDModel(fullName, m, modelName);
            } else {
                logger.error("Unknown model type: " + m.getClass().getName());
                return null;
            }

            model = models.get(fullName);
        }
        return model;
    }

    public static Model GetModel(String modelName){
        return GetModel(modelName, "");
    }

    /**
     * MMD 모델 등록 (기존 로직)
     */
    private static void RegisterMMDModel(String fullName, IMMDModel model, String modelName) {
        NativeFunc nf = NativeFunc.GetInst();
        
        EntityData ed = new EntityData();
        ed.stateLayers = new EntityData.EntityState[3];
        ed.playCustomAnim = false;
        ed.rightHandMat = nf.CreateMat();
        ed.leftHandMat = nf.CreateMat();
        ed.matBuffer = ByteBuffer.allocateDirect(64);

        MMDModelData m = new MMDModelData();
        m.entityName = fullName;
        m.model = model;
        m.modelName = modelName;
        m.entityData = ed;
        
        // MMD 애니메이션 등록
        MMDAnimManager.AddModel(model);
        model.ResetPhysics();
        model.ChangeAnim(MMDAnimManager.GetAnimModel(model, "idle"), 0);
        
        models.put(fullName, m);
        logger.info("✓ MMD model registered: " + fullName);
    }

    /**
     * URDF 모델 등록 (NativeFunc 의존성 제거)
     */
    private static void RegisterURDFModel(String fullName, IMMDModel model, String modelName) {
        URDFModelData m = new URDFModelData();
        m.entityName = fullName;
        m.model = model;
        m.modelName = modelName;
        
        // URDF는 애니메이션 없음
        model.ResetPhysics(); // 조인트를 0으로 리셋
        
        models.put(fullName, m);
        logger.info("✓ URDF model registered: " + fullName);
    }

    public static void ReloadModel() {
        for (Model i : models.values())
            DeleteModel(i);
        models = new HashMap<>();
    }

    static void DeleteModel(Model model) {
        if (model instanceof MMDModelData) {
            MMDModelOpenGL mmdModel = (MMDModelOpenGL) model.model;
            MMDModelOpenGL.Delete(mmdModel);
            MMDAnimManager.DeleteModel(model.model);
        }
        // URDF는 네이티브 리소스 없으므로 삭제 불필요
    }

    // ========== URDF 헬퍼 메서드 ==========
    
    public static URDFModelOpenGL GetURDFModel(String modelName) {
        Model m = GetModel(modelName);
        if (m instanceof URDFModelData && m.model instanceof URDFModelOpenGL) {
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

    // ========== 모델 클래스 계층 ==========

    /**
     * 기본 모델 클래스
     */
    public static abstract class Model {
        public IMMDModel model;
        public String entityName;
        public String modelName;
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
                logger.debug("model.properties not found for " + modelName);
            }
            isPropertiesLoaded = true;
            KAIMyEntityClient.reloadProperties = false;
        }
        
        public abstract boolean isMMDModel();
        public abstract boolean isURDFModel();
    }

    /**
     * MMD 모델 (PMX/PMD)
     */
    public static class MMDModelData extends Model {
        public EntityData entityData;
        
        @Override
        public boolean isMMDModel() { return true; }
        
        @Override
        public boolean isURDFModel() { return false; }
    }

    /**
     * URDF 모델
     */
    public static class URDFModelData extends Model {
        @Override
        public boolean isMMDModel() { return false; }
        
        @Override
        public boolean isURDFModel() { return true; }
    }

    /**
     * 엔티티 상태 데이터 (MMD 전용)
     */
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
        
        public boolean playCustomAnim;
        public long rightHandMat, leftHandMat;
        public EntityState[] stateLayers;
        ByteBuffer matBuffer;

        public enum EntityState {
            Idle, Walk, Sprint, Air, OnClimbable, OnClimbableUp, OnClimbableDown, 
            Swim, Ride, Ridden, Driven, Sleep, ElytraFly, Die, SwingRight, SwingLeft, 
            ItemRight, ItemLeft, Sneak, OnHorse, Crawl, LieDown
        }
    }
}
