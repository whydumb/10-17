/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "GLConstants.hpp"
#include "Handle.hpp"
#include <span>

namespace OpenGL
{
    /**
     * \brief Descriptive alias for OpenGL buffer object handles
     *
     * BufferHandle provides a more specific and readable name for the generic
     * OpenGL handle type. While it doesn't add type safety (it's still the same
     * underlying type as other OpenGL handles), it makes the code's intent clearer
     * by indicating that this handle specifically refers to buffer objects.
     *
     * This alias helps with:
     * - Code readability: immediately clear this refers to a buffer
     * - Documentation: function signatures clearly indicate buffer parameters
     * - Consistency: uniform naming convention across the OpenGL wrapper
     *
     * Note: This is purely a naming convenience - the compiler treats BufferHandle
     * identically to the generic Handle type, so mixing them won't cause errors.
     * The real type safety comes from the BufferType enum and API design.
     */
    using BufferHandle = Handle;

    /**
     * \brief Enumeration of OpenGL buffer types for different rendering purposes
     *
     * BufferType categorizes buffers based on their intended use in the graphics
     * pipeline. Each type corresponds to a specific OpenGL buffer target that
     * determines how the GPU will access and use the buffer data.
     *
     * The enum values map directly to OpenGL constants but provide a more
     * type-safe and readable interface. This helps prevent binding buffers
     * to incorrect targets and makes the code's intent clearer.
     *
     * Buffer Types:
     * - Vertices: Stores vertex attribute data (positions, colors, texture coordinates)
     * - Indices: Stores element indices for indexed drawing operations
     * - UniformBlocks: Stores uniform data shared across multiple shader invocations
     */
    enum class BufferType : GLenum
    {
        Vertices      = GL_ARRAY_BUFFER,         ///< Vertex attribute data buffer
        Indices       = GL_ELEMENT_ARRAY_BUFFER, ///< Element index buffer for indexed drawing
        UniformBlocks = GL_UNIFORM_BUFFER        ///< Uniform block data buffer
    };
    /**
     * \brief Create an empty buffer with specified size for dynamic data
     * \param type The type of buffer to create (Vertices, Indices, or UniformBlocks)
     * \param size_in_bytes Size of the buffer in bytes
     * \return Handle to the newly created buffer object
     *
     * Creates an OpenGL buffer object with allocated storage but no initial data.
     * This is optimal for buffers that will be updated frequently during runtime,
     * such as dynamic vertex data, animated geometry, or per-frame uniform data.
     *
     * The buffer is configured for dynamic usage patterns, making it suitable for:
     * - Vertex data that changes every frame (particle systems, UI elements)
     * - Temporary geometry that gets rebuilt frequently
     * - Buffers that will be updated with new data using UpdateBufferData()
     *
     * Memory allocation happens on the GPU, but no data transfer occurs during
     * creation. The buffer contents are undefined until data is uploaded.
     *
     * The implementation uses GL_DYNAMIC_DRAW usage hint, indicating that the
     * buffer contents will be modified repeatedly and used for drawing operations.
     */
    [[nodiscard]] BufferHandle CreateBuffer(BufferType type, GLsizeiptr size_in_bytes) noexcept;

    /**
     * \brief Create a buffer initialized with static data
     * \param type The type of buffer to create (Vertices, Indices, or UniformBlocks)
     * \param static_buffer_data Span containing the initial data to upload
     * \return Handle to the newly created buffer object
     *
     * Creates an OpenGL buffer object and immediately uploads the provided data.
     * This is optimal for buffers containing static data that won't change during
     * the application's lifetime, such as fixed mesh geometry, constant lookup
     * tables, or unchanging uniform block data.
     *
     * The buffer is configured for static usage patterns, making it suitable for:
     * - Static mesh geometry (terrain, buildings, fixed models)
     * - Constant index arrays for standard shapes (quads, cubes)
     * - Lookup tables and constant uniform data
     * - Any data that remains unchanged after initial upload
     *
     * Memory allocation and data transfer happen atomically during creation,
     * ensuring the buffer is immediately ready for use in rendering operations.
     *
     * The implementation uses GL_STATIC_DRAW usage hint, indicating that the
     * buffer contents will be set once and used repeatedly for drawing.
     */
    [[nodiscard]] BufferHandle CreateBuffer(BufferType type, std::span<const std::byte> static_buffer_data) noexcept;

    /**
     * \brief Update existing buffer with new data
     * \param type The type of buffer being updated
     * \param buffer Handle to the buffer object to update
     * \param data_to_copy Span containing the new data to upload
     * \param starting_offset Byte offset within the buffer where copying begins (default: 0)
     *
     * Uploads new data to an existing buffer object, either replacing all contents
     * or updating a specific region. This is essential for dynamic buffers that
     * need frequent updates, such as animated vertex data, changing uniform values,
     * or streaming geometry data.
     *
     * The function provides flexibility for partial updates:
     * - Full buffer replacement: starting_offset = 0, data spans entire buffer
     * - Partial updates: starting_offset > 0, data spans subset of buffer
     * - Streaming updates: sequential calls with different offsets
     *
     * Common usage patterns:
     * - Updating vertex positions for animated objects
     * - Streaming new geometry data for dynamic scenes
     * - Modifying uniform block data for per-object parameters
     * - Updating index buffers for dynamic mesh topology
     *
     * The buffer must have sufficient capacity to hold the new data starting
     * from the specified offset. The function performs efficient GPU memory
     * transfers using OpenGL's buffer sub-data operations.
     */
    void UpdateBufferData(BufferType type, BufferHandle buffer, std::span<const std::byte> data_to_copy, GLsizei starting_offset = 0) noexcept;
}
