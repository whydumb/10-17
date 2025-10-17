/**
 * \file
 * \author Rudy Castan
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Texture.hpp"
#include "CS200/Image.hpp"
#include "Environment.hpp"
#include "GL.hpp"

namespace OpenGL
{
    /**
     * \brief Create OpenGL texture from loaded image data
     * \param image Image object containing loaded pixel data and dimensions
     * \param filtering Texture sampling method (default: nearest pixel for crisp graphics)
     * \param wrapping Texture coordinate wrapping behavior (default: repeat for tiling)
     * \return Handle to the created OpenGL texture object
     *
     * Creates an OpenGL texture from a pre-loaded Image object, transferring the
     * pixel data to GPU memory and configuring the texture for rendering use.
     * This is the standard approach for loading textures from image files.
     *
     * The image data is transferred in RGBA format with the specified filtering
     * and wrapping settings applied immediately. The texture is ready for use
     * in rendering operations after creation.
     *
     * Common usage patterns:
     * - Loading sprite textures from PNG/JPEG files
     * - Creating texture atlases for efficient batch rendering
     * - Loading UI element graphics and icons
     * - Importing procedurally generated images
     *
     * The function extracts size and pixel data from the Image object and
     * delegates to CreateTextureFromMemory() for the actual OpenGL setup.
     * This provides a convenient interface while maintaining implementation
     * consistency across different texture creation methods.
     */
    [[nodiscard]] TextureHandle CreateTextureFromImage(const CS200::Image& image, Filtering filtering, Wrapping wrapping) noexcept {
        auto size = image.GetSize();
        std::span<const CS200::RGBA> pixels(image.data(), static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y));
        return CreateTextureFromMemory(Math::ivec2{ size.x, size.y }, pixels, filtering, wrapping);
    }

    /**
     * \brief Create OpenGL texture from raw pixel data in memory
     * \param size Texture dimensions in pixels (width, height)
     * \param colors Span of RGBA pixel data in row-major order
     * \param filtering Texture sampling method (default: nearest pixel)
     * \param wrapping Texture coordinate wrapping behavior (default: repeat)
     * \return Handle to the created OpenGL texture object
     *
     * Creates an OpenGL texture directly from a span of RGBA color data,
     * providing maximum flexibility for programmatically generated textures
     * or custom pixel data sources. The pixel data is transferred to GPU
     * memory and configured for immediate rendering use.
     *
     * Pixel data requirements:
     * - Must contain exactly (width × height) RGBA values
     * - Data is interpreted in row-major order (left-to-right, top-to-bottom)
     * - Each pixel is a packed 32-bit RGBA value
     *
     * Common use cases:
     * - Procedurally generated textures (noise, patterns, gradients)
     * - Runtime texture modification and updates
     * - Converting between different color formats
     * - Creating textures from mathematical functions
     * - Importing from custom or binary file formats
     *
     * The implementation creates the OpenGL texture object, applies the
     * specified filtering and wrapping settings, and uploads the pixel
     * data using GL::TexImage2D() for immediate GPU availability.
     */
    [[nodiscard]] TextureHandle CreateTextureFromMemory(Math::ivec2 size, std::span<const CS200::RGBA> colors, Filtering filtering, Wrapping wrapping) noexcept {
        TextureHandle textureID;
        GL::GenTextures(1, &textureID);
        GL::BindTexture(GL_TEXTURE_2D, textureID);

        GLint gl_filter = (filtering == Filtering::NearestPixel) ? GL_NEAREST : GL_LINEAR;
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);

        GLenum gl_wrap = GL_REPEAT;
        switch (wrapping) {
            case Wrapping::Repeat: 
                gl_wrap = GL_REPEAT; 
                break;
            case Wrapping::ClampToEdge: 
                gl_wrap = GL_CLAMP_TO_EDGE; 
                break;
            case Wrapping::MirroredRepeat: 
                gl_wrap = GL_MIRRORED_REPEAT; 
                break;
            default: 
                gl_wrap = GL_REPEAT; 
                break; 
        }

        GLint gl_wrap_s = static_cast<GLint>(gl_wrap);
        GLint gl_wrap_t = static_cast<GLint>(gl_wrap);
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap_s);
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap_t);

        GL::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors.data());

        return textureID;
    }

    /**
     * \brief Create empty RGBA texture without initial pixel data
     * \param size Texture dimensions in pixels (width, height)
     * \param filtering Texture sampling method (default: nearest pixel)
     * \param wrapping Texture coordinate wrapping behavior (default: repeat)
     * \return Handle to the created OpenGL texture object
     *
     * Creates an OpenGL texture with allocated GPU memory but no initial pixel
     * data. This is optimal for textures that will be used as render targets
     * (framebuffer attachments) or will have their data uploaded later through
     * other means.
     *
     * The texture is created with RGBA8 format, providing 8 bits per channel
     * for standard color representation. The memory is allocated but contains
     * undefined initial values until populated through rendering operations
     * or explicit data uploads.
     *
     * Primary use cases:
     * - Framebuffer color attachments for render-to-texture operations
     * - Dynamic textures that will be updated at runtime
     * - Temporary render targets for multi-pass rendering
     * - Screen capture or screenshot buffers
     * - Procedural texture generation targets
     *
     * The implementation uses different OpenGL functions depending on the
     * available OpenGL version: GL::TexStorage2D() for newer versions (more
     * efficient) or GL::TexImage2D() with null data for compatibility.
     *
     * Memory efficiency:
     * Creating empty textures avoids unnecessary data transfers and is
     * particularly efficient when the texture will be written to by
     * rendering operations rather than CPU-provided data.
     */
    [[nodiscard]] TextureHandle CreateRGBATexture(Math::ivec2 size, Filtering filtering, Wrapping wrapping) noexcept {
        TextureHandle textureID = 0;
        GL::GenTextures(1, &textureID);
        GL::BindTexture(GL_TEXTURE_2D, textureID);

        // Set texture filtering
        GLint filter = (filtering == Filtering::NearestPixel) ? GL_NEAREST : GL_LINEAR;
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

        // Set texture wrappingGLenum gl_wrap = GL_REPEAT; 
        GLenum gl_wrap = GL_REPEAT; 
        switch (wrapping) {
            case Wrapping::Repeat: gl_wrap = GL_REPEAT; break;
            case Wrapping::ClampToEdge: gl_wrap = GL_CLAMP_TO_EDGE; break;
            case Wrapping::MirroredRepeat: gl_wrap = GL_MIRRORED_REPEAT; break;
            default: gl_wrap = GL_REPEAT; break; 
        }
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(gl_wrap));
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(gl_wrap));

        // Create empty texture based on OpenGL version or platform
        if (OpenGL::IsWebGL || OpenGL::current_version() >= OpenGL::version(4, 2)) {
            // Use TexStorage2D for immutable storage (better for WebGL and newer versions)
            GL::TexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size.x, size.y);
        } else {
            // Fallback for older OpenGL versions
            GL::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        return textureID;
    }

    void SetFiltering(TextureHandle texture_handle, Filtering filtering) noexcept {
        GL::BindTexture(GL_TEXTURE_2D, texture_handle);

        GLint gl_filter = (filtering == Filtering::NearestPixel) ? GL_NEAREST : GL_LINEAR;
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
        GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);

        GL::BindTexture(GL_TEXTURE_2D, 0);
    }

    void SetWrapping(TextureHandle texture_handle, Wrapping wrapping, TextureCoordinate coord) noexcept {
        GL::BindTexture(GL_TEXTURE_2D, texture_handle);

        GLenum gl_wrap = GL_REPEAT;
        switch (wrapping) {
            case Wrapping::Repeat: 
                gl_wrap = GL_REPEAT; 
                break;
            case Wrapping::ClampToEdge: 
                gl_wrap = GL_CLAMP_TO_EDGE; 
                break;
            case Wrapping::MirroredRepeat: 
                gl_wrap = GL_MIRRORED_REPEAT; 
                break;
            default: 
                gl_wrap = GL_REPEAT; 
                break; 
        }

        GLint gl_wrap_s = static_cast<GLint>(gl_wrap);
        GLint gl_wrap_t = static_cast<GLint>(gl_wrap);

        if (coord == TextureCoordinate::S || coord == TextureCoordinate::Both)
            GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap_s);

        if (coord == TextureCoordinate::T || coord == TextureCoordinate::Both)
            GL::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap_t);

        GL::BindTexture(GL_TEXTURE_2D, 0);
    }

}
