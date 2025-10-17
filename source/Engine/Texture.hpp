/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "Matrix.hpp"
#include "OpenGL/Texture.hpp"
#include "Vec2.hpp"
#include <filesystem>
#include "Engine.hpp"

namespace CS230
{
    class Font;

    /**
     * \brief High-level texture wrapper for 2D graphics rendering and resource management
     *
     * Texture provides a convenient, RAII-compliant wrapper around OpenGL texture objects
     * that simplifies texture loading, rendering, and resource management. The class handles
     * the complex details of OpenGL texture creation, coordinate system transformations,
     * and automatic memory cleanup while providing an intuitive interface for 2D graphics.
     *
     * Key Features:
     * - Automatic texture loading from image files with proper format conversion
     * - RAII resource management ensuring textures are properly cleaned up
     * - High-level drawing interface with transformation matrix support
     * - Sprite sheet support for animation and texture atlases
     * - Integration with the 2D renderer for efficient batched rendering
     * - Move semantics for efficient texture transfers and storage
     *
     * Resource Management:
     * The texture automatically handles OpenGL texture creation and deletion through
     * RAII principles. Move-only semantics prevent accidental texture duplication
     * while allowing efficient transfer of texture ownership between objects.
     *
     * Coordinate Systems:
     * The class handles the conversion between different coordinate systems:
     * - Image coordinates (pixel-based, top-left origin)
     * - Texture coordinates (normalized 0-1 range, bottom-left origin)
     * - World coordinates (transformation matrix-based positioning)
     *
     * Common Use Cases:
     * - Sprite rendering for games and interactive applications
     * - UI element graphics and interface components
     * - Texture atlas rendering for efficient sprite animation
     * - Background and environment art rendering
     * - Font glyph rendering (through Font class integration)
     *
     * The texture integrates seamlessly with the engine's transformation system,
     * allowing for easy scaling, rotation, and positioning of graphics elements.
     */
    class Texture
    {
    public:
        friend class TextureManager;
        friend class Font;

        /**
         * \brief Draw the entire texture with transformation and color tinting
         * \param display_matrix Transformation matrix for positioning, scaling, and rotation
         * \param color RGBA color value for tinting the texture (default: white/no tint)
         *
         * Renders the complete texture to the screen using the provided transformation
         * matrix to control positioning, scaling, and rotation. This is the primary
         * method for drawing sprites, UI elements, and other texture-based graphics.
         *
         * Transformation Handling:
         * The method automatically handles the necessary coordinate system conversions
         * and applies appropriate scaling and translation to ensure the texture appears
         * correctly positioned according to the transformation matrix. The texture is
         * drawn with its center as the transformation origin.
         *
         * Color Tinting:
         * The color parameter allows for real-time tinting of the texture without
         * modifying the original image data. This enables effects like:
         * - Color-based visual feedback (damage flashing, selection highlighting)
         * - Transparency and fade effects through alpha channel modification
         * - Uniform lighting or environmental color effects
         * - UI state visualization (disabled buttons, hover states)
         *
         * Rendering Integration:
         * The method integrates with the engine's 2D renderer for efficient batched
         * rendering, automatically managing texture binding and shader state for
         * optimal performance when drawing multiple textures.
         */
        void Draw(const Math::TransformationMatrix& display_matrix, unsigned int color = 0xFFFFFFFF);

        /**
         * \brief Draw a rectangular region of the texture (sprite sheet support)
         * \param display_matrix Transformation matrix for positioning, scaling, and rotation
         * \param texel_position Top-left corner position in pixel coordinates within the texture
         * \param frame_size Size of the region to draw in pixels
         * \param color RGBA color value for tinting the texture (default: white/no tint)
         *
         * Renders a specific rectangular region of the texture, enabling sprite sheet
         * functionality, texture atlases, and animation frame rendering. This method
         * is essential for efficient graphics where multiple sprites or animation
         * frames are packed into a single texture file.
         *
         * Sprite Sheet Applications:
         * - Character animation frames stored in a grid layout
         * - UI element collections (buttons, icons, decorative elements)
         * - Tile sets for 2D game environments
         * - Font glyph rendering from character atlases
         * - Particle effect textures with multiple variations
         *
         * Coordinate System:
         * The texel_position uses pixel coordinates with (0,0) at the top-left
         * of the texture, following standard image coordinate conventions. The
         * method automatically converts these to the appropriate OpenGL texture
         * coordinates for rendering.
         *
         * Performance Benefits:
         * Using sprite sheets reduces texture binding overhead and improves
         * rendering performance by allowing multiple related graphics to be
         * stored in a single texture object, enabling more efficient batching.
         *
         * The transformation matrix affects the final rendered size and position,
         * while frame_size determines which portion of the texture is sampled.
         */
        void Draw(const Math::TransformationMatrix& display_matrix, Math::ivec2 texel_position, Math::ivec2 frame_size, unsigned int color = 0xFFFFFFFF);

        /**
         * \brief Get the dimensions of the texture in pixels
         * \return Vector containing width and height of the texture
         *
         * Returns the original size of the texture as loaded from the image file,
         * providing essential information for layout calculations, collision detection,
         * and coordinate system conversions. This size information is particularly
         * useful when working with sprite sheets or when precise positioning is required.
         *
         * Common uses:
         * - Calculating sprite sheet frame positions and sizes
         * - Determining collision boundaries for texture-based objects
         * - Computing appropriate scaling factors for different screen resolutions
         * - Layout calculations for UI elements and text rendering
         * - Animation frame calculations and sprite positioning
         *
         * The returned size reflects the texture's native resolution and remains
         * constant regardless of how the texture is drawn or transformed during rendering.
         */
        Math::ivec2 GetSize() const;

        /**
         * \brief Destructor ensuring proper OpenGL resource cleanup
         *
         * Automatically releases the underlying OpenGL texture object when the
         * Texture instance is destroyed, preventing GPU memory leaks and ensuring
         * proper resource management. This follows RAII principles to guarantee
         * cleanup even in the presence of exceptions or early returns.
         */
        ~Texture();

        /**
         * \brief Get the underlying OpenGL texture handle for advanced operations
         * \return OpenGL texture handle for direct OpenGL operations
         *
         * Provides access to the raw OpenGL texture handle for advanced rendering
         * operations that require direct OpenGL texture binding or integration
         * with custom rendering systems. This method should be used sparingly
         * and only when the high-level interface is insufficient.
         *
         * Advanced use cases:
         * - Integration with custom shader programs
         * - Direct OpenGL texture operations
         * - Interfacing with third-party graphics libraries
         * - Performance-critical rendering paths that bypass the 2D renderer
         *
         * Handle Ownership:
         * The returned handle remains owned by the Texture object and should not
         * be manually deleted or modified. The handle becomes invalid when the
         * Texture object is destroyed.
         */
        [[nodiscard]] OpenGL::TextureHandle GetHandle() const
        {
            return textureHandle;
        }

    private:
        // Private constructors - textures can only be created through TextureManager or Font
        // This ensures proper resource management and prevents accidental texture duplication
        explicit Texture(const std::filesystem::path& file_name);
        Texture(OpenGL::TextureHandle given_texture, Math::ivec2 the_size);

    public:
        /**
         * \brief Deleted copy constructor and assignment operator
         *
         * Textures use move-only semantics to prevent accidental duplication of
         * OpenGL resources. Copying textures would require expensive GPU operations
         * and could lead to resource management complications, so copy operations
         * are explicitly disabled to encourage efficient resource usage patterns.
         */
        Texture(const Texture&)            = delete;
        Texture& operator=(const Texture&) = delete;

        /**
         * \brief Move constructor for efficient texture transfer
         * \param temporary Texture object to move from (will be left in valid but empty state)
         *
         * Efficiently transfers ownership of the OpenGL texture resource from one
         * Texture object to another without copying GPU data. The source texture
         * is left in a safe, destructible state with no valid texture handle.
         *
         * Move semantics enable:
         * - Efficient storage of textures in containers (std::vector, std::map)
         * - Return of textures from factory functions without copying
         * - Transfer of texture ownership between different systems
         * - Optimal performance when reorganizing texture collections
         */
        Texture(Texture&& temporary) noexcept;

        /**
         * \brief Move assignment operator for efficient texture replacement
         * \param temporary Texture object to move from (will be left in valid but empty state)
         * \return Reference to this texture after the move operation
         *
         * Replaces the current texture with another texture's resources using
         * efficient move semantics. The previous texture resource is properly
         * cleaned up, and the new resource is transferred without GPU data copying.
         *
         * The operation uses swap semantics to ensure exception safety and
         * guarantee that both the old and new textures are handled correctly
         * even if unexpected conditions occur during the transfer.
         */
        Texture& operator=(Texture&& temporary) noexcept;
        std::string GetPath() const {
            return path.string(); 
        }

    private:
        OpenGL::TextureHandle textureHandle{};
        Math::ivec2 size{};
        std::filesystem::path path; 
    };
}
