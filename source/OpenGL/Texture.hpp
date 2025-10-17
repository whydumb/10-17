/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "CS200/RGBA.hpp"
#include "Engine/Vec2.hpp"
#include "GLConstants.hpp"
#include "GLTypes.hpp"
#include "Handle.hpp"
#include <filesystem>
#include <span>

namespace CS200
{
    class Image;
}

namespace OpenGL
{
    /**
     * \brief Texture filtering modes for controlling pixel sampling behavior
     *
     * Filtering determines how OpenGL samples texture pixels when the texture
     * is displayed at a different size than its native resolution. The choice
     * between nearest-pixel and linear filtering dramatically affects the
     * visual appearance of textures.
     *
     * Filtering affects both magnification (when texture appears larger than
     * its pixel resolution) and minification (when texture appears smaller).
     * The choice depends on the desired visual style and performance requirements.
     *
     * Visual characteristics:
     * - NearestPixel: Sharp, pixelated appearance with hard edges
     * - Linear: Smooth, blended appearance with soft edges
     *
     * Performance considerations:
     * - NearestPixel: Faster sampling, lower memory bandwidth
     * - Linear: More expensive sampling, higher memory bandwidth
     */
    enum class Filtering : GLint
    {
        NearestPixel = GL_NEAREST, ///< Sharp pixelated sampling, ideal for pixel art and crisp graphics
        Linear       = GL_LINEAR   ///< Smooth interpolated sampling, ideal for photographs and realistic textures
    };

    /**
     * \brief Texture wrapping modes for controlling behavior outside texture boundaries
     *
     * Wrapping determines what happens when texture coordinates fall outside
     * the [0,1] range. Different wrapping modes enable various tiling and
     * clamping behaviors essential for different rendering scenarios.
     *
     * Each mode serves specific use cases:
     * - Tiling textures (Repeat, MirroredRepeat)
     * - UI elements and single textures (ClampToEdge, ClampToBorder)
     * - Special effects and seamless patterns (MirrorClampToEdge)
     *
     * The wrapping mode is applied to both S (horizontal) and T (vertical)
     * texture coordinate axes, affecting how textures extend beyond their
     * original boundaries.
     */
    enum class Wrapping : GLint
    {
        Repeat            = GL_REPEAT,              ///< Tile texture infinitely in all directions
        ClampToEdge       = GL_CLAMP_TO_EDGE,       ///< Stretch edge pixels, preventing tiling artifacts
        MirroredRepeat    = GL_MIRRORED_REPEAT,     ///< Tile with alternating mirror reflections
    };

    /**
     * \brief Descriptive alias for OpenGL texture object handles
     *
     * TextureHandle provides a more specific and readable name for the generic
     * OpenGL handle type when referring to texture objects. This improves code
     * clarity without adding compile-time type safety.
     */
    using TextureHandle = Handle;

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
    [[nodiscard]] TextureHandle CreateTextureFromImage(const CS200::Image& image, Filtering filtering = Filtering::NearestPixel, Wrapping wrapping = Wrapping::Repeat) noexcept;

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
    [[nodiscard]] TextureHandle
        CreateTextureFromMemory(Math::ivec2 size, std::span<const CS200::RGBA> colors, Filtering filtering = Filtering::NearestPixel, Wrapping wrapping = Wrapping::Repeat) noexcept;

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
    [[nodiscard]] TextureHandle CreateRGBATexture(Math::ivec2 size, Filtering filtering = Filtering::NearestPixel, Wrapping wrapping = Wrapping::Repeat) noexcept;

    /**
     * \brief Update texture filtering mode after creation
     * \param texture_handle Handle to the texture object to modify
     * \param filtering New filtering mode to apply
     *
     * Changes the texture sampling behavior for an existing texture object.
     * This allows runtime switching between crisp pixel-perfect rendering
     * and smooth interpolated rendering based on visual requirements or
     * user preferences.
     *
     * The filtering setting affects both magnification and minification,
     * determining how the texture appears when scaled larger or smaller
     * than its native resolution.
     *
     * Common scenarios:
     * - Switching between pixel art and smooth rendering modes
     * - Adjusting texture quality based on performance requirements
     * - Creating visual effects with different sampling characteristics
     * - Implementing user-configurable graphics quality settings
     *
     * The function temporarily binds the texture, updates both MIN_FILTER
     * and MAG_FILTER parameters, then unbinds the texture. This ensures
     * the filtering change takes effect immediately for subsequent rendering.
     */
    void SetFiltering(TextureHandle texture_handle, Filtering filtering) noexcept;

    enum TextureCoordinate
    {
        S,
        T,
        Both
    };

    /**
     * \brief Update texture wrapping mode after creation
     * \param texture_handle Handle to the texture object to modify
     * \param wrapping New wrapping mode to apply
     * \param coord Texture coordinate axis to apply the wrapping mode (default: both S and T)
     *
     * Changes how the texture behaves when texture coordinates extend beyond
     * the [0,1] range. This enables runtime switching between different tiling
     * and clamping behaviors without recreating the texture.
     *
     * The wrapping mode can be applied to S (horizontal), T (vertical), or both
     * texture coordinate axes, affecting how the texture extends in all
     * directions beyond its boundaries.
     *
     * Dynamic wrapping use cases:
     * - Switching between tiled and non-tiled rendering modes
     * - Adapting texture behavior for different rendering contexts
     * - Creating visual effects with changing boundary conditions
     * - Implementing different texture addressing for UI vs. world geometry
     *
     * The function temporarily binds the texture, updates WRAP_S,
     * WRAP_T, or both parameters with the new wrapping mode, then unbinds the texture.
     * Changes take effect immediately for subsequent texture sampling operations.
     */
    void SetWrapping(TextureHandle texture_handle, Wrapping wrapping, TextureCoordinate coord = TextureCoordinate::Both) noexcept;
}
