/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Handle.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace OpenGL
{
    using ShaderHandle = Handle;

    struct [[nodiscard]] CompiledShader
    {
        ShaderHandle                           Shader;
        std::unordered_map<std::string, GLint> UniformLocations;
    };

    CompiledShader CreateShader(std::filesystem::path vertex_filepath, std::filesystem::path fragment_filepath);
    CompiledShader CreateShader(std::string_view vertex_source, std::string_view fragment_source);
    void           DestroyShader(CompiledShader& shader) noexcept;
    void           BindUniformBufferToShader(ShaderHandle shader_handle, GLuint binding_number, Handle uniform_bufer, std::string_view uniform_block_name);
}
