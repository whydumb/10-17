package com.kAIS.KAIMyEntity.urdf;

import com.kAIS.KAIMyEntity.renderer.IMMDModel;
import com.mojang.blaze3d.vertex.PoseStack;
import com.mojang.blaze3d.vertex.VertexConsumer;
import net.minecraft.client.renderer.texture.OverlayTexture;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.world.entity.Entity;
import org.joml.Matrix4f;
import org.joml.Quaternionf;
import org.joml.Vector3f;
import org.joml.Vector4f;

import java.util.List;

/**
 * URDF STL 렌더러 (Y-up 기준)
 * - RenderType 경로 사용 (VertexConsumer 기반)
 * - NoCull은 믹스인에서 RenderType.entityTranslucent(...)로 처리
 * - 로컬 조인트 축 회전 유지
 */
public class URDFModelOpenGLWithSTL implements IMMDModel {

    private final ResourceLocation texture;
    private final List<Link> links;

    public URDFModelOpenGLWithSTL(ResourceLocation texture, List<Link> links) {
        this.texture = texture;
        this.links = links;
    }

    /** 레거시 호환(가능하면 renderToBuffer만 사용) */
    @Override
    public void Render(Entity e, float yaw, float pitch, Vector3f trans, float dt, PoseStack pose, int light) {
        // 레거시 경로를 호출해도 안전하게 동작하도록 기본 구현 유지
        // (VertexConsumer가 없으니 실제 그리진 않음)
    }

    /** 표준 렌더: VertexConsumer(addVertex) 사용 */
    @Override
    public void renderToBuffer(Entity entityIn,
                               float entityYaw, float entityPitch, Vector3f entityTrans, float tickDelta,
                               PoseStack pose,
                               VertexConsumer out,
                               int light,
                               int overlay) {
        if (out == null || links == null || links.isEmpty()) return;

        pose.pushPose();
        pose.translate(entityTrans.x, entityTrans.y, entityTrans.z);

        for (Link link : links) {
            pose.pushPose();

            // 로컬 조인트 회전 (축: link.axis, 각도: deg)
            Quaternionf q = new Quaternionf().rotateAxis((float) Math.toRadians(link.angle), link.axis);
            pose.mulPose(q);

            link.renderMesh(pose, out, light, overlay);

            pose.popPose();
        }

        pose.popPose();
    }

    @Override public void ChangeAnim(long anim, long layer) {}
    @Override public void ResetPhysics() {}
    @Override public long GetModelLong() { return 0; }
    @Override public String GetModelDir() { return "urdf"; }
    @Override public ResourceLocation getTexture() { return texture; }

    // ----------------------------------------------------------
    // 내부 구조체
    // ----------------------------------------------------------

    /** URDF 링크: 한 조인트 + 메시 */
    public static class Link {
        public Vector3f axis = new Vector3f(0, 0, 1); // 회전축(로컬)
        public float angle = 0;                        // 각도(deg)
        public Mesh mesh;                              // STL 메시(삼각형 리스트)

        public void renderMesh(PoseStack pose, VertexConsumer out, int light, int overlay) {
            if (mesh == null || mesh.triangles == null || mesh.triangles.isEmpty()) return;

            Matrix4f mat = pose.last().pose();
            Vector4f p = new Vector4f();

            for (Triangle t : mesh.triangles) {
                // 간단한 면 노멀(삼각형 기준)
                Vector3f n = calcFaceNormal(t);

                // v1
                p.set(t.v1, 1f).mul(mat);
                out.addVertex(
                        p.x, p.y, p.z,
                        1f, 1f, 1f, 1f,   // RGBA
                        0f, 0f,           // UV
                        overlay,
                        light,
                        n.x, n.y, n.z     // Normal
                );

                // v2
                p.set(t.v2, 1f).mul(mat);
                out.addVertex(
                        p.x, p.y, p.z,
                        1f, 1f, 1f, 1f,
                        1f, 0f,
                        overlay,
                        light,
                        n.x, n.y, n.z
                );

                // v3
                p.set(t.v3, 1f).mul(mat);
                out.addVertex(
                        p.x, p.y, p.z,
                        1f, 1f, 1f, 1f,
                        0f, 1f,
                        overlay,
                        light,
                        n.x, n.y, n.z
                );
            }
        }

        private static Vector3f calcFaceNormal(Triangle t) {
            Vector3f u = new Vector3f(t.v2).sub(t.v1);
            Vector3f v = new Vector3f(t.v3).sub(t.v1);
            Vector3f n = u.cross(v, new Vector3f());
            if (n.lengthSquared() > 1e-12f) n.normalize();
            else n.set(0, 1, 0);
            return n;
        }
    }

    /** 삼각형 */
    public static class Triangle {
        public final Vector3f v1, v2, v3;
        public Triangle(Vector3f v1, Vector3f v2, Vector3f v3) {
            this.v1 = v1; this.v2 = v2; this.v3 = v3;
        }
    }

    /** 메시 */
    public static class Mesh {
        public List<Triangle> triangles;
        public Mesh(List<Triangle> triangles) { this.triangles = triangles; }
    }

    // ----------------------------------------------------------
    // 팩토리 (MMDModelManager의 Create 호출 호환)
    // ----------------------------------------------------------
    public static URDFModelOpenGLWithSTL Create(String modelName, String modelPath) {
        // TODO: modelPath로 STL/URDF 파싱해서 링크/메시 구성
        ResourceLocation tex = ResourceLocation.parse("minecraft:textures/misc/white.png");
        return new URDFModelOpenGLWithSTL(tex, List.of());
    }
}
