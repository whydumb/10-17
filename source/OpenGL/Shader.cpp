/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Shader.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "Engine/Path.hpp"
#include "GL.hpp"
#include <algorithm>

namespace
{
    void                                                 print_glsl_text(std::string_view source);
    [[nodiscard]] OpenGL::Handle                         compile_shader_source(GLenum type, std::string_view glsl_text);
    [[nodiscard]] OpenGL::Handle                         compile_shader_file(GLenum type, const std::filesystem::path& file_path);
    [[nodiscard]] OpenGL::ShaderHandle                   link_shader_program(OpenGL::Handle vertex_handle, OpenGL::Handle fragment_handle);
    [[nodiscard]] std::unordered_map<std::string, GLint> get_uniform_locations(OpenGL::ShaderHandle shader);
}

namespace OpenGL
{
    CompiledShader CreateShader(std::filesystem::path vertex_filepath, std::filesystem::path fragment_filepath)
    {
        const auto     vertex_handle   = compile_shader_file(GL_VERTEX_SHADER, vertex_filepath);
        const auto     fragment_handle = compile_shader_file(GL_FRAGMENT_SHADER, fragment_filepath);
        CompiledShader cs{};
        cs.Shader           = link_shader_program(vertex_handle, fragment_handle);
        cs.UniformLocations = get_uniform_locations(cs.Shader);
        return cs;
    }

    CompiledShader CreateShader(std::string_view vertex_source, std::string_view fragment_source)
    {
        const auto     vertex_handle   = compile_shader_source(GL_VERTEX_SHADER, vertex_source);
        const auto     fragment_handle = compile_shader_source(GL_FRAGMENT_SHADER, fragment_source);
        CompiledShader cs{};
        cs.Shader           = link_shader_program(vertex_handle, fragment_handle);
        cs.UniformLocations = get_uniform_locations(cs.Shader);
        return cs;
    }

    void DestroyShader(CompiledShader& shader) noexcept
    {
        GL::DeleteProgram(shader.Shader);
        shader.Shader = 0;

        shader.UniformLocations.clear();
    }

    void BindUniformBufferToShader(ShaderHandle shader_handle, GLuint binding_number, Handle uniform_bufer, std::string_view uniform_block_name)
    {
        const auto block_index = GL::GetUniformBlockIndex(shader_handle, uniform_block_name.data());
        if (block_index != GL_INVALID_INDEX)
        {
            GL::UniformBlockBinding(shader_handle, block_index, binding_number);
            GL::BindBufferBase(GL_UNIFORM_BUFFER, binding_number, uniform_bufer);
        }
        else
        {
            Engine::GetLogger().LogError("Uniform block '" + std::string(uniform_block_name) + "' not found in shader.");
        }
    }
}

namespace
{
    void print_glsl_text(std::string_view source)
    {
        using CountInt                          = decltype(std::count(source.begin(), source.end(), '\n'));
        CountInt           num_lines            = std::count(source.begin(), source.end(), '\n');
        const int          max_linenumber_width = static_cast<int>(std::to_string(num_lines).size());
        CountInt           line_number          = 1;
        std::string        line;
        std::ostringstream sout;
        std::istringstream source_stream(source.data());
        while (std::getline(source_stream, line))
        {
            sout << std::setw(max_linenumber_width) << std::right << line_number << "| " << line << '\n';
            ++line_number;
        }
        Engine::GetLogger().LogVerbose(sout.str());
    }

    OpenGL::Handle compile_shader_source(GLenum type, std::string_view glsl_text)
    {
        OpenGL::Handle shader = GL::CreateShader(type);
        GLchar const*  source[]{ glsl_text.data() };
        GL::ShaderSource(shader, 1, source, nullptr);
        GL::CompileShader(shader);
        GLint is_compiled = 0;
        GL::GetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
        if (is_compiled == GL_FALSE)
        {
            GLint log_length = 0;
            GL::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            std::string error_log;
            error_log.resize(static_cast<std::string::size_type>(log_length) + 1);
            GL::GetShaderInfoLog(shader, log_length, nullptr, error_log.data());
            GL::DeleteShader(shader);
            shader = 0;
            Engine::GetLogger().LogError(error_log);
            print_glsl_text(glsl_text);
            throw std::runtime_error(error_log);
        }
        return shader;
    }

    OpenGL::Handle compile_shader_file(GLenum type, const std::filesystem::path& file_path)
    {
        const auto    shader_file_path = assets::locate_asset(file_path);
        std::ifstream ifs(shader_file_path, std::ios::in);
        if (!ifs)
        {
            Engine::GetLogger().LogError("Cannot open " + file_path.string());
            return 0;
        }
        std::string glsl_text;
        glsl_text.reserve(gsl::narrow<std::size_t>(std::filesystem::file_size(shader_file_path)));
        std::copy((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>(), std::back_insert_iterator(glsl_text));
        return compile_shader_source(type, std::string_view(glsl_text));
    }

    OpenGL::ShaderHandle link_shader_program(OpenGL::Handle vertex_handle, OpenGL::Handle fragment_handle)
    {
        OpenGL::ShaderHandle program_handle = GL::CreateProgram();
        if (program_handle == 0)
        {
            throw std::runtime_error("Unable to create program\n");
        }

        GL::AttachShader(program_handle, vertex_handle);
        GL::AttachShader(program_handle, fragment_handle);

        GL::LinkProgram(program_handle);

        GL::DeleteShader(vertex_handle);
        GL::DeleteShader(fragment_handle);

        GLint is_linked = 0;
        GL::GetProgramiv(program_handle, GL_LINK_STATUS, &is_linked);
        if (is_linked == GL_FALSE)
        {
            GLint log_length = 0;
            GL::GetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &log_length);
            std::string error;
            error.resize(static_cast<unsigned>(log_length) + 1);
            GL::GetProgramInfoLog(program_handle, log_length, nullptr, error.data());
            Engine::GetLogger().LogError(error);
            throw std::runtime_error(error);
        }
        return program_handle;
    }

    std::unordered_map<std::string, GLint> get_uniform_locations(OpenGL::ShaderHandle shader)
    {
        std::unordered_map<std::string, GLint> uniform_locations;
        GLint                                  num_uniforms = 0;
        GL::GetProgramiv(shader, GL_ACTIVE_UNIFORMS, &num_uniforms);
        if (num_uniforms <= 0)
        {
            return uniform_locations;
        }
        GLint max_name_length = 0;
        GL::GetProgramiv(shader, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length);
        uniform_locations.reserve(static_cast<std::size_t>(num_uniforms));
        std::string uniform_name;
        uniform_name.resize(static_cast<std::size_t>(max_name_length));

        for (GLint i = 0; i < num_uniforms; ++i)
        {
            GLsizei length = 0;
            GLint   size   = 0;
            GLenum  type   = 0;
            GL::GetActiveUniform(shader, static_cast<GLuint>(i), max_name_length, &length, &size, &type, uniform_name.data());
            uniform_name.resize(static_cast<std::size_t>(length));
            GLint location = GL::GetUniformLocation(shader, uniform_name.c_str());
            if (location != -1)
            {
                uniform_locations[uniform_name] = location;
            }
            uniform_name.resize(static_cast<std::size_t>(max_name_length));
        }
        return uniform_locations;
    }
}
