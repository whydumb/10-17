/**
 * \file
 * \author Rudy Castan
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include "Engine/Vec2.hpp"
#include "RGBA.hpp"
#include <filesystem>
#include <gsl/gsl>

#include <stb_image.h>

namespace CS200
{
    /**
     * \brief RAII wrapper for loading and managing image pixel data from files
     *
     * The Image class provides a safe, automatic way to load image files (PNG, JPG, etc.)
     * and access their pixel data for use in graphics applications. It handles all the
     * complexity of file loading, memory management, and data conversion.
     *
     * Key Features:
     * - Automatic file loading using stb_image library
     * - Always converts to consistent RGBA format (4 bytes per pixel)
     * - RAII memory management (automatic cleanup in destructor)
     * - Move-only semantics to prevent expensive copying
     * - Optional vertical flipping for different coordinate systems
     *
     * Common Use Cases:
     * - Loading textures for sprites, backgrounds, UI elements
     * - Reading image data for procedural generation or analysis
     * - Converting various image formats to a consistent RGBA format
     * - Preparing pixel data for upload to GPU textures
     *
     * Memory Layout:
     * - Pixels stored in row-major order (left-to-right, top-to-bottom)
     * - Each pixel is 4 bytes: Red, Green, Blue, Alpha (0-255 each)
     * - Total memory = width × height × 4 bytes
     *
     * Example Usage:
     * \code
     * // Load an image file
     * Image sprite_image("sprites/player.png");
     *
     * // Get image properties
     * auto size = sprite_image.GetSize();
     * int width = size.x, height = size.y;
     *
     * // Access pixel data
     * const RGBA* pixels = sprite_image.data();
     * RGBA top_left_pixel = pixels[0];
     *
     * // Move to another Image (efficient)
     * Image moved_image = std::move(sprite_image);
     * \endcode
     
     */
    class Image
    {
    public:
        /**
         * \brief Load an image from file and store its pixel data
         * \param image_path Path to the image file (relative to Assets folder, like "Assets/ship.png")
         * \param flip_vertical Whether to flip the image vertically when loading (default: false)
         *
         * Implementation notes:
         * - Use assets::locate_asset() to find the full file path
         * - Use stb_image library functions to load the image data
         * - Always load as 4-channel RGBA regardless of source format
         * - Set stbi_set_flip_vertically_on_load() before loading
         * - Throw an error if loading fails
         * - Store the loaded pixel data and image dimensions
         */
        explicit Image(const std::filesystem::path& image_path, bool flip_vertical = false);

        /**
         * \brief Copy constructor - deleted to prevent accidental copying
         * Images manage dynamic memory and should not be copied
         */
        Image(const Image&) = delete;

        /**
         * \brief Copy assignment - deleted to prevent accidental copying
         * Images manage dynamic memory and should not be copied
         */
        Image& operator=(const Image&) = delete;

        /**
         * \brief Move constructor - transfer ownership of image data
         * \param temporary The temporary Image object to move from
         *
         * Implementation notes:
         * - Transfer ownership of pixel data pointer and size
         * - Set the source object's data to nullptr and size to {0,0}
         * - Use noexcept to enable move optimizations
         */
        Image(Image&& temporary) noexcept;

        /**
         * \brief Move assignment - transfer ownership of image data
         * \param temporary The temporary Image object to move from
         * \return Reference to this object
         *
         * Implementation notes:
         * - Use std::swap to exchange data between objects
         * - This safely handles self-assignment and cleanup
         */
        Image& operator=(Image&& temporary) noexcept;

        /**
         * \brief Destructor - free the allocated image data
         *
         * Implementation notes:
         * - Check if pixel data pointer is not nullptr
         * - Use stbi_image_free() to properly release memory allocated by stb_image
         */
        ~Image();

        /**
         * \brief Get read-only access to the pixel data array
         * \return Const pointer to the first RGBA pixel
         *
         * Implementation notes:
         * - Return the stored pixel data pointer as const
         * - Pixels are stored in row-major order (left-to-right, top-to-bottom)
         */
        const RGBA* data() const noexcept;

        /**
         * \brief Get read-write access to the pixel data array
         * \return Pointer to the first RGBA pixel for modification
         *
         * Implementation notes:
         * - Return the stored pixel data pointer for modification
         * - Pixels are stored in row-major order (left-to-right, top-to-bottom)
         */
        RGBA* data() noexcept;

        /**
         * \brief Get the dimensions of the loaded image
         * \return Vector containing width (x) and height (y) in pixels
         *
         * Implementation notes:
         * - Return the stored image dimensions
         * - Width and height are set during image loading
         */
        Math::ivec2 GetSize() const noexcept;

    private:
        // Add private member variables to store image data and dimensions
        stbi_uc*    image_data = nullptr;
        Math::ivec2 size;
    };

}
