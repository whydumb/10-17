package com.kAIS.KAIMyEntity.urdf;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.*;
import net.minecraft.client.renderer.GameRenderer;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joml.Matrix4f;
import org.joml.Vector3f;
import org.lwjgl.opengl.GL46C;

import java.nio.FloatBuffer;
import java.util.HashMap;
import java.util.Map;

/**
 * STL 메시를 OpenGL로 렌더링
 * VBO 캐싱으로 성능 최적화
 */
public class STLMeshRenderer {
    private static final Logger logger = LogManager.getLogger();
    
    // 메시 캐시 (파일 경로 → GPU 버퍼)
    private static final Map<String, CachedMesh> meshCache = new HashMap<>();
    
    /**
     * GPU에 업로드된 메시 데이터
     */
    private static class CachedMesh {
        int vao;  // Vertex Array Object
        int vbo;  // Vertex Buffer (position + normal)
        int triangleCount;
        Vector3f minBounds;
        Vector3f maxBounds;
        
        void delete() {
            GL46C.glDeleteVertexArrays(vao);
            GL46C.glDeleteBuffers(vbo);
        }
    }
    
    /**
     * STL 메시 렌더링 (캐싱 지원)
     */
    public static void renderSTL(String filepath, Vector3f scale, 
                                  float r, float g, float b, float a,
                                  PoseStack mat) {
        
        // 캐시 확인
        CachedMesh cached = meshCache.get(filepath);
        
        if (cached == null) {
            // 처음 로드
            STLLoader.STLMesh mesh = STLLoader.load(filepath);
            if (mesh == null) {
                logger.warn("Failed to load STL: " + filepath);
                return;
            }
            
            // 스케일 적용
            if (scale != null && 
                (Math.abs(scale.x - 1.0f) > 0.001f || 
                 Math.abs(scale.y - 1.0f) > 0.001f || 
                 Math.abs(scale.z - 1.0f) > 0.001f)) {
                STLLoader.scaleMesh(mesh, scale);
            }
            
            // GPU에 업로드
            cached = uploadMeshToGPU(mesh);
            meshCache.put(filepath, cached);
            
            logger.info("Cached STL mesh: " + filepath + 
                       " (" + cached.triangleCount + " triangles)");
        }
        
        // 렌더링
        renderCachedMesh(cached, r, g, b, a, mat);
    }
    
    /**
     * 메시를 GPU에 업로드
     */
    private static CachedMesh uploadMeshToGPU(STLLoader.STLMesh mesh) {
        CachedMesh cached = new CachedMesh();
        cached.triangleCount = mesh.getTriangleCount();
        cached.minBounds = new Vector3f(mesh.minBounds);
        cached.maxBounds = new Vector3f(mesh.maxBounds);
        
        // VAO 생성
        cached.vao = GL46C.glGenVertexArrays();
        GL46C.glBindVertexArray(cached.vao);
        
        // VBO 생성 (Interleaved: position + normal)
        cached.vbo = GL46C.glGenBuffers();
        GL46C.glBindBuffer(GL46C.GL_ARRAY_BUFFER, cached.vbo);
        
        // 데이터 준비 (각 정점: 6 floats = position(3) + normal(3))
        int vertexCount = mesh.getVertexCount();
        FloatBuffer buffer = org.lwjgl.system.MemoryUtil.memAllocFloat(vertexCount * 6);
        
        for (STLLoader.Triangle tri : mesh.triangles) {
            for (int i = 0; i < 3; i++) {
                // Position
                buffer.put(tri.vertices[i].x);
                buffer.put(tri.vertices[i].y);
                buffer.put(tri.vertices[i].z);
                
                // Normal
                buffer.put(tri.normal.x);
                buffer.put(tri.normal.y);
                buffer.put(tri.normal.z);
            }
        }
        buffer.flip();
        
        // GPU에 전송
        GL46C.glBufferData(GL46C.GL_ARRAY_BUFFER, buffer, GL46C.GL_STATIC_DRAW);
        org.lwjgl.system.MemoryUtil.memFree(buffer);
        
        // Vertex Attributes 설정
        // Position (location 0)
        GL46C.glEnableVertexAttribArray(0);
        GL46C.glVertexAttribPointer(0, 3, GL46C.GL_FLOAT, false, 6 * 4, 0);
        
        // Normal (location 1)
        GL46C.glEnableVertexAttribArray(1);
        GL46C.glVertexAttribPointer(1, 3, GL46C.GL_FLOAT, false, 6 * 4, 3 * 4);
        
        GL46C.glBindVertexArray(0);
        
        return cached;
    }
    
    /**
     * 캐시된 메시 렌더링
     */
    private static void renderCachedMesh(CachedMesh mesh, 
                                         float r, float g, float b, float a,
                                         PoseStack mat) {
        
        RenderSystem.setShader(GameRenderer::getPositionColorShader);
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.enableDepthTest();
        RenderSystem.enableCull();
        
        // VAO 바인딩
        GL46C.glBindVertexArray(mesh.vao);
        
        // 셰이더 설정
        Matrix4f matrix = mat.last().pose();
        
        // Minecraft의 기본 셰이더 사용
        // Position + Color로 렌더링
        BufferBuilder bufferBuilder = Tesselator.getInstance().begin(
            VertexFormat.Mode.TRIANGLES,
            DefaultVertexFormat.POSITION_COLOR
        );
        
        // 직접 그리기 (VBO 사용)
        // TODO: 커스텀 셰이더로 Normal 활용하면 더 예쁨
        
        // 일단 간단하게 그리기
        GL46C.glDrawArrays(GL46C.GL_TRIANGLES, 0, mesh.triangleCount * 3);
        
        GL46C.glBindVertexArray(0);
    }
    
    /**
     * Minecraft 스타일 렌더링 (느리지만 호환성 좋음)
     */
    public static void renderSTLImmediate(String filepath, Vector3f scale,
                                          float r, float g, float b, float a,
                                          PoseStack mat) {
        
        STLLoader.STLMesh mesh = STLLoader.load(filepath);
        if (mesh == null) {
            return;
        }
        
        if (scale != null) {
            STLLoader.scaleMesh(mesh, scale);
        }
        
        RenderSystem.setShader(GameRenderer::getPositionColorShader);
        RenderSystem.enableBlend();
        RenderSystem.defaultBlendFunc();
        RenderSystem.enableDepthTest();
        RenderSystem.enableCull();
        
        BufferBuilder bufferBuilder = Tesselator.getInstance().begin(
            VertexFormat.Mode.TRIANGLES,
            DefaultVertexFormat.POSITION_COLOR
        );
        
        Matrix4f matrix = mat.last().pose();
        
        // 각 삼각형 렌더링
        for (STLLoader.Triangle tri : mesh.triangles) {
            // 간단한 조명 효과 (법선 기반)
            Vector3f lightDir = new Vector3f(0.5f, 0.7f, 0.3f).normalize();
            float brightness = Math.max(0.3f, tri.normal.dot(lightDir));
            
            float finalR = r * brightness;
            float finalG = g * brightness;
            float finalB = b * brightness;
            
            // 3개 정점
            for (int i = 0; i < 3; i++) {
                Vector3f v = tri.vertices[i];
                bufferBuilder.addVertex(matrix, v.x, v.y, v.z)
                    .setColor(finalR, finalG, finalB, a);
            }
        }
        
        BufferUploader.drawWithShader(bufferBuilder.buildOrThrow());
    }
    
    /**
     * 캐시 정리
     */
    public static void clearCache() {
        for (CachedMesh mesh : meshCache.values()) {
            mesh.delete();
        }
        meshCache.clear();
        logger.info("Cleared STL mesh cache");
    }
    
    /**
     * 특정 메시 캐시 삭제
     */
    public static void removeCached(String filepath) {
        CachedMesh mesh = meshCache.remove(filepath);
        if (mesh != null) {
            mesh.delete();
        }
    }
    
    /**
     * 캐시 통계
     */
    public static void printCacheStats() {
        logger.info("=== STL Mesh Cache ===");
        logger.info("Cached meshes: " + meshCache.size());
        
        int totalTriangles = 0;
        for (CachedMesh mesh : meshCache.values()) {
            totalTriangles += mesh.triangleCount;
        }
        logger.info("Total triangles: " + totalTriangles);
    }
}
