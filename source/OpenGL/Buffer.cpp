/**
 * \file
 * \author Rudy Castan
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Buffer.hpp"

#include <vector>

#include "GL.hpp"

namespace OpenGL
{
    BufferHandle CreateBuffer(BufferType type, GLsizeiptr size_in_bytes) noexcept
    {
        BufferHandle new_buffer{};
        GL::GenBuffers(1, &new_buffer);
        GL::BindBuffer(static_cast<GLenum>(type), new_buffer);
        GL::BufferData(static_cast<GLenum>(type), size_in_bytes, nullptr, GL_DYNAMIC_DRAW);
        GL::BindBuffer(static_cast<GLenum>(type), 0);
        // https://docs.gl/es3/glGenBuffers
        // https://docs.gl/es3/glBindBuffer
        // https://docs.gl/es3/glBufferData
        return new_buffer;
    }

    BufferHandle CreateBuffer(BufferType type, std::span<const std::byte> static_buffer_data) noexcept
    {
        BufferHandle new_buffer{};
        GL::GenBuffers(1, &new_buffer);
        GL::BindBuffer(static_cast<GLenum>(type), new_buffer);
        GL::BufferData(static_cast<GLenum>(type), static_cast<GLsizeiptr>(static_buffer_data.size() * sizeof(static_buffer_data[0])), static_buffer_data.data(), GL_STATIC_DRAW);
        GL::BindBuffer(static_cast<GLenum>(type), 0);
        // https://docs.gl/es3/glGenBuffers
        // https://docs.gl/es3/glBindBuffer
        // https://docs.gl/es3/glBufferData
        return new_buffer;
    }

    void UpdateBufferData(BufferType type, BufferHandle buffer, std::span<const std::byte> data_to_copy, GLsizei starting_offset) noexcept
    {
        // https://docs.gl/es3/glBindBuffer
        // https://docs.gl/es3/glBufferSubData
        GL::BindBuffer(static_cast<GLenum>(type), buffer);
        GL::BufferSubData(static_cast<GLenum>(type), starting_offset, static_cast<GLsizeiptr>(data_to_copy.size() * sizeof(data_to_copy[0])), data_to_copy.data());
        GL::BindBuffer(static_cast<GLenum>(type), 0);
    }
}
