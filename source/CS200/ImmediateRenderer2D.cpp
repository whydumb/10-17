/**
 * \file ImmediateRenderer2D.cpp
 * \author Rudy Castan
 * \author
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute
 */
#include "ImmediateRenderer2D.hpp"

#include "Engine/Matrix.hpp"
#include "Engine/Path.hpp"
#include "Engine/Texture.hpp"
#include "OpenGL/Buffer.hpp"
#include "OpenGL/GL.hpp"
#include "Renderer2DUtils.hpp"
#include <utility>
#include "Engine/Logger.hpp"

namespace CS200
{

    ImmediateRenderer2D::ImmediateRenderer2D(ImmediateRenderer2D&& other) noexcept
        : quad_vertex_array(other.quad_vertex_array),
          quad_vertex_buffer(other.quad_vertex_buffer),
          quad_index_buffer(other.quad_index_buffer),
          camera_uniform_buffer(other.camera_uniform_buffer),
          shader(std::move(other.shader)),
          view_projection(other.view_projection)
    {
  
        other.quad_vertex_array = 0;
        other.quad_vertex_buffer = 0;
        other.quad_index_buffer = 0;
        other.camera_uniform_buffer = 0;
        other.shader = OpenGL::CompiledShader{};
        other.view_projection.Reset();
    }


    ImmediateRenderer2D& ImmediateRenderer2D::operator=(ImmediateRenderer2D&& other) noexcept
    {
        if (this != &other)
        {
            Shutdown(); 

            std::swap(quad_vertex_array, other.quad_vertex_array);
            std::swap(quad_vertex_buffer, other.quad_vertex_buffer);
            std::swap(quad_index_buffer, other.quad_index_buffer);
            std::swap(camera_uniform_buffer, other.camera_uniform_buffer);
            std::swap(shader, other.shader);
            std::swap(view_projection, other.view_projection);
        }
        return *this;
    }


    ImmediateRenderer2D::~ImmediateRenderer2D()
    {
        Shutdown();
    }


    void ImmediateRenderer2D::Init()
    {

         shader = OpenGL::CreateShader(assets::locate_asset("Assets/shaders/ImmediateRenderer2D/quad.vert"), assets::locate_asset("Assets/shaders/ImmediateRenderer2D/quad.frag"));


 
        const GLuint indices[] = { 0, 1, 2, 2, 3, 0 };

        const float vertices[] = {
    
            -0.5f, -0.5f, 0.0f, 0.0f, // bottom-left
             0.5f, -0.5f, 1.0f, 0.0f, // bottom-right
             0.5f,  0.5f, 1.0f, 1.0f, // top-right
            -0.5f,  0.5f, 0.0f, 1.0f  // top-left
        };


        GL::GenBuffers(1, &quad_vertex_buffer);
        GL::BindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);
        GL::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // give the data of vertices to quadvertexbuffer


        GL::GenBuffers(1, &quad_index_buffer);
        GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_index_buffer);
        GL::BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 
 
        GL::GenVertexArrays(1, &quad_vertex_array);
        GL::BindVertexArray(quad_vertex_array);

 
        GL::BindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer); 

        GL::EnableVertexAttribArray(0);
        GL::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));


        GL::EnableVertexAttribArray(1);
        GL::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));


        GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_index_buffer);

        GL::BindVertexArray(0);


        GL::GenBuffers(1, &camera_uniform_buffer);
        GL::BindBuffer(GL_UNIFORM_BUFFER, camera_uniform_buffer);
        GL::BufferData(GL_UNIFORM_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        GL::BindBufferBase(GL_UNIFORM_BUFFER, 0, camera_uniform_buffer);
        OpenGL::BindUniformBufferToShader(shader.Shader, 0, camera_uniform_buffer, "Camera");
        GL::BindBuffer(GL_UNIFORM_BUFFER, 0);
    }


    void ImmediateRenderer2D::Shutdown()
    {
        if (quad_vertex_array)
        {
            GL::DeleteVertexArrays(1, &quad_vertex_array);
            quad_vertex_array = 0;
        }
        if (quad_vertex_buffer)
        {
            GL::DeleteBuffers(1, &quad_vertex_buffer);
            quad_vertex_buffer = 0;
        }
        if (quad_index_buffer)
        {
            GL::DeleteBuffers(1, &quad_index_buffer);
            quad_index_buffer = 0;
        }
        if (camera_uniform_buffer)
        {
            GL::DeleteBuffers(1, &camera_uniform_buffer);
            camera_uniform_buffer = 0;
        }

        OpenGL::DestroyShader(shader);
        shader = OpenGL::CompiledShader{};
    }


    void ImmediateRenderer2D::BeginScene(const Math::TransformationMatrix& vp)
    {
        view_projection = vp;

        float data[16] = {
            static_cast<float>(vp[0][0]), static_cast<float>(vp[1][0]), static_cast<float>(vp[2][0]), 0.0f,
            static_cast<float>(vp[0][1]), static_cast<float>(vp[1][1]), static_cast<float>(vp[2][1]), 0.0f,
            static_cast<float>(vp[0][2]), static_cast<float>(vp[1][2]), static_cast<float>(vp[2][2]), 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f
        };

        GL::BindBuffer(GL_UNIFORM_BUFFER, camera_uniform_buffer);
        GL::BufferData(GL_UNIFORM_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        GL::BindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void ImmediateRenderer2D::EndScene()
    {
    }

    void ImmediateRenderer2D::DrawQuad(const Math::TransformationMatrix& transform,
                                       OpenGL::TextureHandle texture,
                                       Math::vec2 texture_coord_bl,
                                       Math::vec2 texture_coord_tr,
                                       CS200::RGBA tintColor)
    {
        GL::UseProgram(shader.Shader); 

        float model[9] = { 
            static_cast<float>(transform[0][0]), static_cast<float>(transform[1][0]), static_cast<float>(transform[2][0]),
            static_cast<float>(transform[0][1]), static_cast<float>(transform[1][1]), static_cast<float>(transform[2][1]),
            static_cast<float>(transform[0][2]), static_cast<float>(transform[1][2]), static_cast<float>(transform[2][2]),
        };

        if (shader.UniformLocations.contains("uModel")) 
            GL::UniformMatrix3fv(shader.UniformLocations.at("uModel"), 1, GL_FALSE, model); 
        Math::vec2 uv_scale = texture_coord_tr - texture_coord_bl;
        Math::vec2 uv_offset = texture_coord_bl;

        if (shader.UniformLocations.contains("uTextureTransform"))
        {
            float texmat[9] = 
            {
            static_cast<float>(uv_scale.x), 0.0f, 0.0f,
            0.0f, static_cast<float>(uv_scale.y), 0.0f,
            static_cast<float>(uv_offset.x), static_cast<float>(uv_offset.y), 1.0f
            };
            GL::UniformMatrix3fv(shader.UniformLocations.at("uTextureTransform"), 1, GL_FALSE, texmat);

        }
        
        if (shader.UniformLocations.contains("u_depth"))
            GL::Uniform1f(shader.UniformLocations.at("u_depth"), 0.0f);

        if (shader.UniformLocations.contains("uTintColor"))
        {
            auto c = unpack_color(tintColor);
            GL::Uniform4f(shader.UniformLocations.at("uTintColor"), c[0], c[1], c[2], c[3]);
        }

        if (shader.UniformLocations.contains("uTex2d"))
            GL::Uniform1i(shader.UniformLocations.at("uTex2d"), 0); 

        GL::ActiveTexture(GL_TEXTURE0);
        GL::BindTexture(GL_TEXTURE_2D, texture); 

        GL::BindVertexArray(quad_vertex_array);
        GL::DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        GL::BindVertexArray(0);
    }
}
