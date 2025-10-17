/**
 * \file immediateRenderer2D.hpp
 * \author Rudy Castan
 * \author 
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "IRenderer2D.hpp"
#include "OpenGL/Shader.hpp"
#include "OpenGL/VertexArray.hpp"
#include <array>
#include "Engine/Matrix.hpp"
#include "Engine/Engine.hpp"

namespace CS200
{
    /**
     * \brief Immediate-mode 2D renderer for drawing sprites, shapes, and lines
     *
     * ImmediateRenderer2D provides a simple, immediate-mode interface for 2D graphics rendering.
     * "Immediate mode" means each draw call renders directly to the screen without batching or
     * optimization - perfect for learning graphics programming and prototyping.
     *
     * Key Features:
     * - Textured quad rendering with tint colors and texture coordinate mapping
     * - SDF (Signed Distance Field) based shape rendering for perfect circles and rectangles
     * - Line rendering with arbitrary thickness
     * - Camera/view-projection matrix support for world-to-screen transformations
     * - Hardware-accelerated OpenGL rendering with custom shaders
     *
     * Rendering Pipeline:
     * 1. Init() - Set up OpenGL resources (shaders, buffers, vertex arrays)
     * 2. BeginScene() - Set camera/view matrix for the frame
     * 3. Draw*() calls - Render individual primitives immediately
     * 4. EndScene() - Finalize the frame (currently no-op)
     * 5. Shutdown() - Clean up OpenGL resources
     *
     * Technical Implementation:
     * - Uses two rendering paths: textured quads and SDF shapes
     * - Quad rendering: Standard texture mapping with transform and tint
     * - SDF rendering: Fragment shader-based shapes with perfect edges and outlines
     * - Uniform buffer for camera data shared between shaders
     * - Immediate submission to GPU (no batching)
     *
     * Common Use Cases:
     * - Sprite rendering for games (characters, backgrounds, UI)
     * - Debug visualization (bounding boxes, collision shapes)
     * - Simple 2D graphics applications
     * - Educational graphics programming
     *
     * Example Usage:
     * \code
     * // Create and initialize renderer (typically done in Engine initialization)
     * ImmediateRenderer2D renderer;
     * renderer.Init();
     *
     * // In your game loop update/draw function:
     *
     * // Set up NDC camera matrix for screen-space rendering
     * Math::ivec2 screen_size = {800, 600};  // Your window/viewport size
     * auto ndc_matrix = CS200::build_ndc_matrix(screen_size);
     * renderer.BeginScene(ndc_matrix);
     *
     * // Draw a textured sprite (position/rotation/scale transform)
     * auto sprite_transform = Math::TranslationMatrix({100, 200}) *
     *                        Math::RotationMatrix(45.0) *
     *                        Math::ScaleMatrix({64, 64});
     * renderer.DrawQuad(sprite_transform, texture_handle, {0,0}, {1,1}, CS200::WHITE);
     *
     * // Draw shapes with fills and outlines
     * auto circle_transform = Math::TranslationMatrix({300, 300}) * Math::ScaleMatrix({50, 50});
     * renderer.DrawCircle(circle_transform, CS200::WHITE, CS200::BLACK, 2.0);
     *
     * // Draw debug lines
     * renderer.DrawLine({0, 0}, {100, 100}, CS200::WHITE, 1.0);
     *
     * renderer.EndScene();
     *
     * // Cleanup when shutting down
     * renderer.Shutdown();
     * \endcode
     */
    class ImmediateRenderer2D : public IRenderer2D
    {
    public:
    
        /**
         * \brief Default constructor - creates uninitialized renderer
         *
         * Implementation notes:
         * - Initialize all handles/resources to invalid/zero values
         * - Must call Init() before use
         */
        ImmediateRenderer2D() = default;

        /**
         * \brief Copy constructor deleted - renderer manages unique OpenGL resources
         */
        ImmediateRenderer2D(const ImmediateRenderer2D& other) = delete;

        /**
         * \brief Move constructor - transfer ownership of OpenGL resources
         * \param other The renderer to move from
         *
         * Implementation notes:
         * - Transfer all OpenGL handles (VAOs, buffers, shaders)
         * - Set source object's handles to zero/invalid
         * - Move shader objects using std::move()
         * - Copy view projection matrix data
         */
        ImmediateRenderer2D(ImmediateRenderer2D&& other) noexcept;

        /**
         * \brief Copy assignment deleted - renderer manages unique OpenGL resources
         */
        ImmediateRenderer2D& operator=(const ImmediateRenderer2D& other) = delete;

        /**
         * \brief Move assignment - transfer ownership of OpenGL resources
         * \param other The renderer to move from
         * \return Reference to this object
         *
         * Implementation notes:
         * - Use std::swap to exchange all resources
         * - Safely handles self-assignment
         * - Automatic cleanup of previous resources
         */
        ImmediateRenderer2D& operator=(ImmediateRenderer2D&& other) noexcept;

        /**
         * \brief Destructor - automatically cleans up OpenGL resources
         *
         * Implementation notes:
         * - Call Shutdown() to clean up resources
         * - Safe to call even if Init() was never called
         */
        ~ImmediateRenderer2D() override;

        /**
         * \brief Initialize OpenGL resources for rendering
         *
         * Implementation notes:
         * - Create index buffer with quad indices (0,1,2,2,3,0)
         * - Create vertex buffer with quad vertices (-0.5 to 0.5 range)
         * - Set up VAO with position and texture coordinate attributes
         * - Create SDF vertex buffer (position-only attributes)
         * - Load and compile vertex/fragment shaders from Assets/shaders/
         * - Create uniform buffer for camera/view-projection matrix
         * - Bind uniform buffer to both shaders with name "Camera"
         */
        void Init() override;

        /**
         * \brief Clean up all OpenGL resources
         *
         * Implementation notes:
         * - Delete all vertex arrays, buffers using OpenGL delete functions
         * - Destroy shader programs using OpenGL::DestroyShader()
         * - Set all handles back to zero/invalid
         * - Safe to call multiple times
         */
        void Shutdown() override;

        /**
         * \brief Begin a new frame with camera/view transformation
         * \param view_projection Combined view and projection matrix for the frame
         *
         * Implementation notes:
         * - Convert 3x3 matrix to 4x3 format for uniform buffer
         * - Update uniform buffer with new matrix data
         * - Bind uniform buffer for use by shaders
         * - Store matrix for potential later use
         */
        void BeginScene(const Math::TransformationMatrix& view_projection) override;

        /**
         * \brief End the current frame (currently no operations needed)
         *
         * Implementation notes:
         * - No cleanup or finalization needed for immediate mode
         * - Could be used for debugging/profiling in future
         */
        void EndScene() override;

        /**
         * \brief Draw a textured quad with transformation and tinting
         * \param transform World transformation matrix (position, rotation, scale)
         * \param texture OpenGL texture handle to sample from
         * \param texture_coord_bl Bottom-left texture coordinate (typically {0,0})
         * \param texture_coord_tr Top-right texture coordinate (typically {1,1})
         * \param tintColor Color to multiply with texture (RGBA::White for no tint)
         *
         * Implementation notes:
         * - Calculate texture coordinate transformation matrix
         * - Set shader uniforms: model matrix, depth, texture transform, tint color
         * - Bind texture to texture unit 0
         * - Draw using quad VAO and index buffer
         * - Use GL_TRIANGLES with 6 indices (2 triangles)
         */
        void DrawQuad(const Math::TransformationMatrix& transform, OpenGL::TextureHandle texture, Math::vec2 texture_coord_bl, Math::vec2 texture_coord_tr, CS200::RGBA tintColor) override;


    private:
        GLuint quad_vertex_array  = 0;
        GLuint quad_vertex_buffer = 0;
        
        GLuint quad_index_buffer  = 0;
        GLuint camera_uniform_buffer = 0;

        OpenGL::CompiledShader shader;

        // --- Cached view-projection matrix ---
        Math::TransformationMatrix view_projection;

        
    
    };
}
