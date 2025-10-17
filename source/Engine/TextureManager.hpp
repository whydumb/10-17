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
#include <filesystem>
#include <map>
#include <vector>
#include <unordered_set>

namespace CS230
{
    class Texture;

    /**
     * \brief Centralized texture resource management system with caching and render-to-texture capabilities
     *
     * TextureManager serves as the central hub for all texture-related operations in the engine,
     * providing efficient resource management, automatic caching, and advanced render-to-texture
     * functionality. The manager prevents duplicate texture loading, handles memory cleanup,
     * and provides tools for creating procedural textures through render-to-texture operations.
     *
     * Core Responsibilities:
     * - Texture loading and caching to prevent duplicate GPU resources
     * - Memory management and cleanup of all texture resources
     * - Render-to-texture functionality for creating dynamic textures
     * - Resource lifecycle tracking and automatic cleanup
     *
     * Caching System:
     * The manager maintains a cache of loaded textures indexed by file path, ensuring that
     * the same image file is never loaded multiple times into GPU memory. This significantly
     * reduces memory usage and loading times when the same textures are referenced by
     * multiple game objects or UI elements.
     *
     * Render-to-Texture Features:
     * Provides a complete render-to-texture system that allows drawing operations to be
     * captured into texture objects rather than displayed on screen. This enables:
     * - Procedural texture generation through drawing operations
     * - Text rendering to textures for efficient text display
     * - Dynamic UI element creation and caching
     * - Off-screen rendering for special effects and post-processing
     *
     * Resource Management:
     * All textures created through the manager are automatically tracked and cleaned up
     * during shutdown, preventing memory leaks and ensuring proper resource disposal.
     * The manager handles both file-loaded textures and runtime-generated textures
     * through a unified interface.
     *
     * Integration with Engine:
     * The TextureManager integrates seamlessly with the 2D renderer and coordinate
     * system, automatically handling viewport management and coordinate transformations
     * during render-to-texture operations while preserving the normal rendering state.
     */
    class TextureManager
    {
    public:
        /**
         * \brief Load a texture from file with automatic caching
         * \param file_name Path to the image file to load
         * \return Pointer to the loaded texture (cached if previously loaded)
         *
         * Loads a texture from the specified image file, utilizing an internal cache
         * to ensure that the same file is never loaded multiple times. If the texture
         * has been previously loaded, the cached version is returned immediately
         * without performing file I/O or GPU operations.
         *
         * Supported Formats:
         * The function supports common image formats through the underlying image
         * loading system, including PNG, JPEG, BMP, and other formats supported
         * by the engine's image loader.
         *
         * Caching Behavior:
         * - First load: Reads file, creates GPU texture, stores in cache
         * - Subsequent loads: Returns cached texture immediately
         * - Cache key: Full file path (different paths to same file create separate entries)
         * - Memory efficiency: Prevents duplicate GPU resources for same image
         *
         * Resource Ownership:
         * The returned texture remains owned by the TextureManager and will be
         * automatically cleaned up when Unload() is called or the manager is
         * destroyed. Callers should not manually delete the returned texture.
         *
         * Error Handling:
         * If the file cannot be loaded (missing file, unsupported format, etc.),
         * the function behavior depends on the underlying image loading system.
         * The manager logs loading events for debugging and monitoring purposes.
         *
         * Performance Characteristics:
         * - First load: File I/O + GPU texture creation overhead
         * - Cached loads: Very fast hash table lookup with no I/O
         * - Memory usage: One GPU texture per unique file path
         */
        Texture* Load(const std::filesystem::path& file_name);

        /**
         * \brief Unload and clean up all managed textures
         *
         * Performs comprehensive cleanup of all textures managed by the TextureManager,
         * including both file-loaded textures and render-to-texture generated textures.
         * This function ensures proper GPU memory cleanup and prevents resource leaks.
         *
         * Cleanup Operations:
         * - Deletes all cached file-based textures and clears the cache
         * - Deletes all render-to-texture generated textures
         * - Releases associated GPU memory and OpenGL resources
         * - Logs cleanup operation for debugging purposes
         *
         * When to Call:
         * - Application shutdown to ensure clean resource cleanup
         * - Level transitions when textures are no longer needed
         * - Memory management when switching between different content sets
         * - Error recovery situations requiring resource reset
         *
         * Post-Cleanup State:
         * After calling Unload(), all previously returned texture pointers become
         * invalid and should not be used. The manager returns to its initial empty
         * state and is ready to load new textures.
         *
         */
        void Unload();

    private:
        std::map<std::filesystem::path, Texture*> textures; 
        // unordered map
        std::vector<Texture*> rendered_textures;
        std::unordered_set<std::filesystem::path> protected_paths;
        
    };
}
