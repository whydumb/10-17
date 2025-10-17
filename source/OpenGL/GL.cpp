/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
// Include GLEW first to define OpenGL functions and constants
#include <GL/glew.h>

#include "Engine/Engine.hpp"
#include "Engine/Logger.hpp"
#include "GL.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>


#if defined(DEVELOPER_VERSION)
#    include <source_location>
#    define VOID_SOURCE_LOCATION const std::source_location caller_location
#    define SOURCE_LOCATION      , VOID_SOURCE_LOCATION
#    define glCheck(expression)                                                                                                                                                                        \
        expression;                                                                                                                                                                                    \
        glCheckError(caller_location.file_name(), caller_location.line(), caller_location.function_name(), #expression)

namespace
{
    inline void glCheckError(const char* file, unsigned line, const char* function_name, const char* opengl_function)
    {
        GLenum errorCode = glGetError();

        if (errorCode == GL_NO_ERROR)
            return;

        const std::string  fileString  = file;
        std::string        error       = "Unknown error";
        std::string        description = "No description";
        std::ostringstream serr;

        serr << "OpenGL call " << opengl_function << " failed in " << fileString.substr(fileString.find_last_of("\\/") + 1) << "(" << line << ")."
             << "\nwithin Function:\n   " << function_name << "\nError description:\n   ";
        int loop_limit = 0;
        while (errorCode != GL_NO_ERROR && loop_limit < 3)
        {
            ++loop_limit;
            switch (errorCode)
            {
                case GL_INVALID_ENUM:
                    {
                        error       = "GL_INVALID_ENUM";
                        description = "An unacceptable value has been specified for an enumerated argument.";
                        break;
                    }

                case GL_INVALID_VALUE:
                    {
                        error       = "GL_INVALID_VALUE";
                        description = "A numeric argument is out of range.";
                        break;
                    }

                case GL_INVALID_OPERATION:
                    {
                        error       = "GL_INVALID_OPERATION";
                        description = "The specified operation is not allowed in the current state.";
                        break;
                    }

                case GL_STACK_OVERFLOW:
                    {
                        error       = "GL_STACK_OVERFLOW";
                        description = "This command would cause a stack overflow.";
                        break;
                    }

                case GL_STACK_UNDERFLOW:
                    {
                        error       = "GL_STACK_UNDERFLOW";
                        description = "This command would cause a stack underflow.";
                        break;
                    }

                case GL_OUT_OF_MEMORY:
                    {
                        error       = "GL_OUT_OF_MEMORY";
                        description = "There is not enough memory left to execute the command.";
                        break;
                    }

                default:
                    {
                        error += " " + std::to_string(errorCode);
                        break;
                    }
            }

            serr << error << "\n   " << description << "\n\n";
            errorCode = glGetError();
        }
        Engine::GetLogger().LogError(serr.str());
        assert(false);
    }
}
#else
#    define SOURCE_LOCATION
#    define VOID_SOURCE_LOCATION void
#    define glCheck(expression)  expression
#endif


namespace GL
{
    const GLubyte* GetString(GLenum name SOURCE_LOCATION)
    {
        glCheck(const auto the_string = glGetString(name));
        return the_string;
    }

    GLboolean IsBuffer(GLuint buffer SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsBuffer(buffer));
        return result;
    }

    GLboolean IsEnabled(GLenum cap SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsEnabled(cap));
        return result;
    }

    GLboolean IsProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsProgram(program));
        return result;
    }

    GLboolean IsShader(GLuint shader SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsShader(shader));
        return result;
    }

    GLboolean IsTexture(GLuint texture SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsTexture(texture));
        return result;
    }

    GLenum GetError(void)
    {
        // Note: We don't use glCheck here since this function IS the error checker
        return glGetError();
    }

    GLint GetAttribLocation(GLuint program, const GLchar* name SOURCE_LOCATION)
    {
        glCheck(const auto location = glGetAttribLocation(program, name));
        return location;
    }

    GLint GetUniformLocation(GLuint program, const GLchar* name SOURCE_LOCATION)
    {
        glCheck(const auto location = glGetUniformLocation(program, name));
        return location;
    }

    GLuint CreateProgram(VOID_SOURCE_LOCATION)
    {
        glCheck(const auto program = glCreateProgram());
        return program;
    }

    GLuint CreateShader(GLenum shaderType SOURCE_LOCATION)
    {
        glCheck(const auto shader = glCreateShader(shaderType));
        return shader;
    }

    void ActiveTexture(GLenum texture SOURCE_LOCATION)
    {
        glCheck(glActiveTexture(texture));
    }

    void AttachShader(GLuint program, GLuint shader SOURCE_LOCATION)
    {
        glCheck(glAttachShader(program, shader));
    }

    void BindBuffer(GLenum target, GLuint buffer SOURCE_LOCATION)
    {
        glCheck(glBindBuffer(target, buffer));
    }

    void BindBufferBase(GLenum target, GLuint index, GLuint buffer SOURCE_LOCATION)
    {
        glCheck(glBindBufferBase(target, index, buffer));
    }

    void BindTexture(GLenum target, GLuint texture SOURCE_LOCATION)
    {
        glCheck(glBindTexture(target, texture));
    }

    void BlendEquation(GLenum mode SOURCE_LOCATION)
    {
        glCheck(glBlendEquation(mode));
    }

    void BlendFunc(GLenum sfactor, GLenum dfactor SOURCE_LOCATION)
    {
        glCheck(glBlendFunc(sfactor, dfactor));
    }

    void BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage SOURCE_LOCATION)
    {
        glCheck(glBufferData(target, size, data, usage));
    }

    void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glBufferSubData(target, offset, size, data));
    }

    void Clear(GLbitfield mask SOURCE_LOCATION)
    {
        glCheck(glClear(mask));
    }

    void ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha SOURCE_LOCATION)
    {
        glCheck(glClearColor(red, green, blue, alpha));
    }

    void CompileShader(GLuint shader SOURCE_LOCATION)
    {
        glCheck(glCompileShader(shader));
    }

    void CullFace(GLenum mode SOURCE_LOCATION)
    {
        glCheck(glCullFace(mode));
    }

    void DeleteBuffers(GLsizei n, const GLuint* buffers SOURCE_LOCATION)
    {
        glCheck(glDeleteBuffers(n, buffers));
    }

    void DeleteProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glDeleteProgram(program));
    }

    void DeleteShader(GLuint shader SOURCE_LOCATION)
    {
        glCheck(glDeleteShader(shader));
    }

    void DeleteTextures(GLsizei n, const GLuint* textures SOURCE_LOCATION)
    {
        glCheck(glDeleteTextures(n, textures));
    }

    void DepthMask(GLboolean flag SOURCE_LOCATION)
    {
        glCheck(glDepthMask(flag));
    }

    void Disable(GLenum cap SOURCE_LOCATION)
    {
        glCheck(glDisable(cap));
    }

    void DrawArrays(GLenum mode, GLint first, GLsizei count SOURCE_LOCATION)
    {
        glCheck(glDrawArrays(mode, first, count));
    }

    void DrawBuffers(GLsizei n, const GLenum* bufs SOURCE_LOCATION)
    {
        glCheck(glDrawBuffers(n, bufs));
    }

    void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices SOURCE_LOCATION)
    {
        glCheck(glDrawElements(mode, count, type, indices));
    }

    void Enable(GLenum cap SOURCE_LOCATION)
    {
        glCheck(glEnable(cap));
    }

    void EnableVertexAttribArray(GLuint index SOURCE_LOCATION)
    {
        glCheck(glEnableVertexAttribArray(index));
    }

    void FrontFace(GLenum mode SOURCE_LOCATION)
    {
        glCheck(glFrontFace(mode));
    }

    void GenBuffers(GLsizei n, GLuint* buffers SOURCE_LOCATION)
    {
        glCheck(glGenBuffers(n, buffers));
    }

    void GenTextures(GLsizei n, GLuint* textures SOURCE_LOCATION)
    {
        glCheck(glGenTextures(n, textures));
    }

    void GetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION)
    {
        glCheck(glGetActiveAttrib(program, index, bufSize, length, size, type, name));
    }

    void GetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION)
    {
        glCheck(glGetActiveUniform(program, index, bufSize, length, size, type, name));
    }

    void GetBooleanv(GLenum pname, GLboolean* data SOURCE_LOCATION)
    {
        glCheck(glGetBooleanv(pname, data));
    }

    void GetIntegerv(GLenum pname, GLint* data SOURCE_LOCATION)
    {
        glCheck(glGetIntegerv(pname, data));
    }

    void GetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION)
    {
        glCheck(glGetProgramInfoLog(program, maxLength, length, infoLog));
    }

    void GetProgramiv(GLuint program, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetProgramiv(program, pname, params));
    }

    void GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION)
    {
        glCheck(glGetShaderInfoLog(shader, maxLength, length, infoLog));
    }

    void GetShaderiv(GLuint shader, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetShaderiv(shader, pname, params));
    }

    void GetUniformfv(GLuint program, GLint location, GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glGetUniformfv(program, location, params));
    }

    void GetUniformiv(GLuint program, GLint location, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetUniformiv(program, location, params));
    }

    void GetUniformuiv(GLuint program, GLint location, GLuint* params SOURCE_LOCATION)
    {
        glCheck(glGetUniformuiv(program, location, params));
    }

    void Hint(GLenum target, GLenum mode SOURCE_LOCATION)
    {
        glCheck(glHint(target, mode));
    }

    void LinkProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glLinkProgram(program));
    }

    void PolygonOffset(GLfloat factor, GLfloat units SOURCE_LOCATION)
    {
        glCheck(glPolygonOffset(factor, units));
    }

    void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels SOURCE_LOCATION)
    {
        glCheck(glReadPixels(x, y, width, height, format, type, pixels));
    }

    void ShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length SOURCE_LOCATION)
    {
        glCheck(glShaderSource(shader, count, string, length));
    }

    void TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glTexImage2D(target, level, internalFormat, width, height, border, format, type, data));
    }

    void TexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations SOURCE_LOCATION)
    {
        glCheck(glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations));
    }

    void TexParameterfv(GLenum target, GLenum pname, const GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glTexParameterfv(target, pname, params));
    }

    void TexParameteri(GLenum target, GLenum pname, GLint param SOURCE_LOCATION)
    {
        glCheck(glTexParameteri(target, pname, param));
    }

    void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels SOURCE_LOCATION)
    {
        glCheck(glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels));
    }

    void Uniform1f(GLint location, GLfloat v0 SOURCE_LOCATION)
    {
        glCheck(glUniform1f(location, v0));
    }

    void Uniform1i(GLint location, GLint v0 SOURCE_LOCATION)
    {
        glCheck(glUniform1i(location, v0));
    }

    void Uniform1iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION)
    {
        glCheck(glUniform1iv(location, count, value));
    }

    void Uniform1ui(GLint location, GLuint v0 SOURCE_LOCATION)
    {
        glCheck(glUniform1ui(location, v0));
    }

    void Uniform2f(GLint location, GLfloat v0, GLfloat v1 SOURCE_LOCATION)
    {
        glCheck(glUniform2f(location, v0, v1));
    }

    void Uniform2fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniform2fv(location, count, value));
    }

    void Uniform2i(GLint location, GLint v0, GLint v1 SOURCE_LOCATION)
    {
        glCheck(glUniform2i(location, v0, v1));
    }

    void Uniform2ui(GLint location, GLuint v0, GLuint v1 SOURCE_LOCATION)
    {
        glCheck(glUniform2ui(location, v0, v1));
    }

    void Uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2 SOURCE_LOCATION)
    {
        glCheck(glUniform3f(location, v0, v1, v2));
    }

    void Uniform3fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniform3fv(location, count, value));
    }

    void Uniform3i(GLint location, GLint v0, GLint v1, GLint v2 SOURCE_LOCATION)
    {
        glCheck(glUniform3i(location, v0, v1, v2));
    }

    void Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2 SOURCE_LOCATION)
    {
        glCheck(glUniform3ui(location, v0, v1, v2));
    }

    void Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 SOURCE_LOCATION)
    {
        glCheck(glUniform4f(location, v0, v1, v2, v3));
    }

    void Uniform4fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniform4fv(location, count, value));
    }

    void Uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3 SOURCE_LOCATION)
    {
        glCheck(glUniform4i(location, v0, v1, v2, v3));
    }

    void Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 SOURCE_LOCATION)
    {
        glCheck(glUniform4ui(location, v0, v1, v2, v3));
    }

    void UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix2fv(location, count, transpose, value));
    }

    void UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix2x3fv(location, count, transpose, value));
    }

    void UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix2x4fv(location, count, transpose, value));
    }

    void UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix3fv(location, count, transpose, value));
    }

    void UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix3x2fv(location, count, transpose, value));
    }

    void UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix3x4fv(location, count, transpose, value));
    }

    void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix4fv(location, count, transpose, value));
    }

    void UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix4x2fv(location, count, transpose, value));
    }

    void UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniformMatrix4x3fv(location, count, transpose, value));
    }

    void UseProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glUseProgram(program));
    }

    void ClearDepth(GLdouble depth SOURCE_LOCATION)
    {
        glCheck(glClearDepth(depth));
    }

    void ClearStencil(GLint s SOURCE_LOCATION)
    {
        glCheck(glClearStencil(s));
    }

    void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha SOURCE_LOCATION)
    {
        glCheck(glColorMask(red, green, blue, alpha));
    }

    void CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border SOURCE_LOCATION)
    {
        glCheck(glCopyTexImage2D(target, level, internalformat, x, y, width, height, border));
    }

    void CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height));
    }

    void DepthRange(GLdouble nearVal, GLdouble farVal SOURCE_LOCATION)
    {
        glCheck(glDepthRange(nearVal, farVal));
    }

    void DetachShader(GLuint program, GLuint shader SOURCE_LOCATION)
    {
        glCheck(glDetachShader(program, shader));
    }

    void DisableVertexAttribArray(GLuint index SOURCE_LOCATION)
    {
        glCheck(glDisableVertexAttribArray(index));
    }

    void DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices SOURCE_LOCATION)
    {
        glCheck(glDrawRangeElements(mode, start, end, count, type, indices));
    }

    void GetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders SOURCE_LOCATION)
    {
        glCheck(glGetAttachedShaders(program, maxCount, count, shaders));
    }

    void GetFloatv(GLenum pname, GLfloat* data SOURCE_LOCATION)
    {
        glCheck(glGetFloatv(pname, data));
    }

    void GetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source SOURCE_LOCATION)
    {
        glCheck(glGetShaderSource(shader, bufSize, length, source));
    }

    void GetTexParameterfv(GLenum target, GLenum pname, GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glGetTexParameterfv(target, pname, params));
    }

    void GetTexParameteriv(GLenum target, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetTexParameteriv(target, pname, params));
    }

    void GetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glGetVertexAttribfv(index, pname, params));
    }

    void GetVertexAttribiv(GLuint index, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetVertexAttribiv(index, pname, params));
    }

    void GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer SOURCE_LOCATION)
    {
        glCheck(glGetVertexAttribPointerv(index, pname, pointer));
    }

    void LineWidth(GLfloat width SOURCE_LOCATION)
    {
        glCheck(glLineWidth(width));
    }

    void PixelStorei(GLenum pname, GLint param SOURCE_LOCATION)
    {
        glCheck(glPixelStorei(pname, param));
    }

    void Scissor(GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glScissor(x, y, width, height));
    }

    void StencilMask(GLuint mask SOURCE_LOCATION)
    {
        glCheck(glStencilMask(mask));
    }

    void StencilMaskSeparate(GLenum face, GLuint mask SOURCE_LOCATION)
    {
        glCheck(glStencilMaskSeparate(face, mask));
    }

    void TexParameterf(GLenum target, GLenum pname, GLfloat param SOURCE_LOCATION)
    {
        glCheck(glTexParameterf(target, pname, param));
    }

    void TexParameteriv(GLenum target, GLenum pname, const GLint* params SOURCE_LOCATION)
    {
        glCheck(glTexParameteriv(target, pname, params));
    }

    void Uniform1fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glUniform1fv(location, count, value));
    }

    void Uniform1uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION)
    {
        glCheck(glUniform1uiv(location, count, value));
    }

    void Uniform2iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION)
    {
        glCheck(glUniform2iv(location, count, value));
    }

    void Uniform2uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION)
    {
        glCheck(glUniform2uiv(location, count, value));
    }

    void Uniform3iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION)
    {
        glCheck(glUniform3iv(location, count, value));
    }

    void Uniform3uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION)
    {
        glCheck(glUniform3uiv(location, count, value));
    }

    void Uniform4iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION)
    {
        glCheck(glUniform4iv(location, count, value));
    }

    void Uniform4uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION)
    {
        glCheck(glUniform4uiv(location, count, value));
    }

    void VertexAttrib1f(GLuint index, GLfloat v0 SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib1f(index, v0));
    }

    void VertexAttrib1fv(GLuint index, const GLfloat* v SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib1fv(index, v));
    }

    void VertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1 SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib2f(index, v0, v1));
    }

    void VertexAttrib2fv(GLuint index, const GLfloat* v SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib2fv(index, v));
    }

    void VertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2 SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib3f(index, v0, v1, v2));
    }

    void VertexAttrib3fv(GLuint index, const GLfloat* v SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib3fv(index, v));
    }

    void VertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib4f(index, v0, v1, v2, v3));
    }

    void VertexAttrib4fv(GLuint index, const GLfloat* v SOURCE_LOCATION)
    {
        glCheck(glVertexAttrib4fv(index, v));
    }

    void ValidateProgram(GLuint program SOURCE_LOCATION)
    {
        glCheck(glValidateProgram(program));
    }

    void VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION)
    {
        glCheck(glVertexAttribPointer(index, size, type, normalized, stride, pointer));
    }

    void Viewport(GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glViewport(x, y, width, height));
    }

    GLenum CheckFramebufferStatus(GLenum target SOURCE_LOCATION)
    {
        glCheck(const GLenum status = glCheckFramebufferStatus(target));
        return status;
    }

    void BindFramebuffer(GLenum target, GLuint framebuffer SOURCE_LOCATION)
    {
        glCheck(glBindFramebuffer(target, framebuffer));
    }

    void BindRenderbuffer(GLenum target, GLuint renderbuffer SOURCE_LOCATION)
    {
        glCheck(glBindRenderbuffer(target, renderbuffer));
    }

    void BindVertexArray(GLuint array SOURCE_LOCATION)
    {
        glCheck(glBindVertexArray(array));
    }

    void DeleteFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION)
    {
        glCheck(glDeleteFramebuffers(n, framebuffers));
    }

    void DeleteVertexArrays(GLsizei n, const GLuint* arrays SOURCE_LOCATION)
    {
        glCheck(glDeleteVertexArrays(n, arrays));
    }

    void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level SOURCE_LOCATION)
    {
        glCheck(glFramebufferTexture2D(target, attachment, textarget, texture, level));
    }

    void GenFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION)
    {
        glCheck(glGenFramebuffers(n, framebuffers));
    }

    void GenVertexArrays(GLsizei n, GLuint* arrays SOURCE_LOCATION)
    {
        glCheck(glGenVertexArrays(n, arrays));
    }

    GLboolean IsFramebuffer(GLuint framebuffer SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsFramebuffer(framebuffer));
        return result;
    }

    GLboolean IsRenderbuffer(GLuint renderbuffer SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsRenderbuffer(renderbuffer));
        return result;
    }

    GLboolean IsQuery(GLuint id SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsQuery(id));
        return result;
    }

    GLboolean IsSampler(GLuint id SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsSampler(id));
        return result;
    }

    GLboolean IsSync(GLsync sync SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsSync(sync));
        return result;
    }

    GLboolean IsTransformFeedback(GLuint id SOURCE_LOCATION)
    {
        glCheck(const auto result = glIsTransformFeedback(id));
        return result;
    }

    GLenum ClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout SOURCE_LOCATION)
    {
        glCheck(const auto result = glClientWaitSync(sync, flags, timeout));
        return result;
    }

    GLint GetFragDataLocation(GLuint program, const char* name SOURCE_LOCATION)
    {
        glCheck(const auto location = glGetFragDataLocation(program, name));
        return location;
    }

    GLsync FenceSync(GLenum condition, GLbitfield flags SOURCE_LOCATION)
    {
        glCheck(const auto sync = glFenceSync(condition, flags));
        return sync;
    }

    GLuint GetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName SOURCE_LOCATION)
    {
        glCheck(const auto index = glGetUniformBlockIndex(program, uniformBlockName));
        return index;
    }

    void BeginQuery(GLenum target, GLuint id SOURCE_LOCATION)
    {
        glCheck(glBeginQuery(target, id));
    }

    void BeginTransformFeedback(GLenum primitiveMode SOURCE_LOCATION)
    {
        glCheck(glBeginTransformFeedback(primitiveMode));
    }

    void ClearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil SOURCE_LOCATION)
    {
        glCheck(glClearBufferfi(buffer, drawBuffer, depth, stencil));
    }

    void ClearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value SOURCE_LOCATION)
    {
        glCheck(glClearBufferfv(buffer, drawBuffer, value));
    }

    void ClearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value SOURCE_LOCATION)
    {
        glCheck(glClearBufferiv(buffer, drawBuffer, value));
    }

    void ClearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value SOURCE_LOCATION)
    {
        glCheck(glClearBufferuiv(buffer, drawBuffer, value));
    }

    void ClearDepthf(GLfloat depth SOURCE_LOCATION)
    {
        glCheck(glClearDepthf(depth));
    }

    void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data));
    }

    void CompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data));
    }

    void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data));
    }

    void CompressedTexSubImage3D(
        GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data));
    }

    void CopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size SOURCE_LOCATION)
    {
        glCheck(glCopyBufferSubData(readtarget, writetarget, readoffset, writeoffset, size));
    }

    void CopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height));
    }

    void CreateRenderbuffers(GLsizei n, GLuint* renderbuffers SOURCE_LOCATION)
    {
        glCheck(glCreateRenderbuffers(n, renderbuffers));
    }

    void CreateSamplers(GLsizei n, GLuint* samplers SOURCE_LOCATION)
    {
        glCheck(glCreateSamplers(n, samplers));
    }

    void CreateTransformFeedbacks(GLsizei n, GLuint* ids SOURCE_LOCATION)
    {
        glCheck(glCreateTransformFeedbacks(n, ids));
    }

    void DeleteQueries(GLsizei n, const GLuint* ids SOURCE_LOCATION)
    {
        glCheck(glDeleteQueries(n, ids));
    }

    void DeleteRenderbuffers(GLsizei n, GLuint* renderbuffers SOURCE_LOCATION)
    {
        glCheck(glDeleteRenderbuffers(n, renderbuffers));
    }

    void DeleteSamplers(GLsizei n, const GLuint* samplers SOURCE_LOCATION)
    {
        glCheck(glDeleteSamplers(n, samplers));
    }

    void DeleteSync(GLsync sync SOURCE_LOCATION)
    {
        glCheck(glDeleteSync(sync));
    }

    void DeleteTransformFeedbacks(GLsizei n, const GLuint* ids SOURCE_LOCATION)
    {
        glCheck(glDeleteTransformFeedbacks(n, ids));
    }

    void DepthRangef(GLfloat n, GLfloat f SOURCE_LOCATION)
    {
        glCheck(glDepthRangef(n, f));
    }

    void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount SOURCE_LOCATION)
    {
        glCheck(glDrawArraysInstanced(mode, first, count, primcount));
    }

    void DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount SOURCE_LOCATION)
    {
        glCheck(glDrawElementsInstanced(mode, count, type, indices, primcount));
    }

    void EndQuery(GLenum target SOURCE_LOCATION)
    {
        glCheck(glEndQuery(target));
    }

    void EndTransformFeedback(VOID_SOURCE_LOCATION)
    {
        glCheck(glEndTransformFeedback());
    }

    void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer SOURCE_LOCATION)
    {
        glCheck(glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer));
    }

    void FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer SOURCE_LOCATION)
    {
        glCheck(glFramebufferTextureLayer(target, attachment, texture, level, layer));
    }

    void GenerateMipmap(GLenum target SOURCE_LOCATION)
    {
        glCheck(glGenerateMipmap(target));
    }

    void GenQueries(GLsizei n, GLuint* ids SOURCE_LOCATION)
    {
        glCheck(glGenQueries(n, ids));
    }

    void GenRenderbuffers(GLsizei n, GLuint* renderbuffers SOURCE_LOCATION)
    {
        glCheck(glGenRenderbuffers(n, renderbuffers));
    }

    void GenSamplers(GLsizei n, GLuint* samplers SOURCE_LOCATION)
    {
        glCheck(glGenSamplers(n, samplers));
    }

    void GenTransformFeedbacks(GLsizei n, GLuint* ids SOURCE_LOCATION)
    {
        glCheck(glGenTransformFeedbacks(n, ids));
    }

    void GetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params));
    }

    void GetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName SOURCE_LOCATION)
    {
        glCheck(glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName));
    }

    void GetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params));
    }

    void GetBooleani_v(GLenum target, GLuint index, GLboolean* data SOURCE_LOCATION)
    {
        glCheck(glGetBooleani_v(target, index, data));
    }

    void GetBufferParameteri64v(GLenum target, GLenum value, GLint64* data SOURCE_LOCATION)
    {
        glCheck(glGetBufferParameteri64v(target, value, data));
    }

    void GetBufferParameteriv(GLenum target, GLenum value, GLint* data SOURCE_LOCATION)
    {
        glCheck(glGetBufferParameteriv(target, value, data));
    }

    void GetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glGetBufferSubData(target, offset, size, data));
    }

    void GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetFramebufferAttachmentParameteriv(target, attachment, pname, params));
    }

    void GetInteger64i_v(GLenum target, GLuint index, GLint64* data SOURCE_LOCATION)
    {
        glCheck(glGetInteger64i_v(target, index, data));
    }

    void GetInteger64v(GLenum pname, GLint64* data SOURCE_LOCATION)
    {
        glCheck(glGetInteger64v(pname, data));
    }

    void GetIntegeri_v(GLenum target, GLuint index, GLint* data SOURCE_LOCATION)
    {
        glCheck(glGetIntegeri_v(target, index, data));
    }

    void GetQueryiv(GLenum target, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetQueryiv(target, pname, params));
    }

    void GetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params SOURCE_LOCATION)
    {
        glCheck(glGetQueryObjectuiv(id, pname, params));
    }

    void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetRenderbufferParameteriv(target, pname, params));
    }

    void GetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glGetSamplerParameterfv(sampler, pname, params));
    }

    void GetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetSamplerParameteriv(sampler, pname, params));
    }

    void GetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values SOURCE_LOCATION)
    {
        glCheck(glGetSynciv(sync, pname, bufSize, length, values));
    }

    void GetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name SOURCE_LOCATION)
    {
        glCheck(glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name));
    }

    void GetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar** uniformNames, GLuint* uniformIndices SOURCE_LOCATION)
    {
        glCheck(glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices));
    }

    void GetVertexAttribIiv(GLuint index, GLenum pname, GLint* params SOURCE_LOCATION)
    {
        glCheck(glGetVertexAttribIiv(index, pname, params));
    }

    void GetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params SOURCE_LOCATION)
    {
        glCheck(glGetVertexAttribIuiv(index, pname, params));
    }

    void PauseTransformFeedback(VOID_SOURCE_LOCATION)
    {
        glCheck(glPauseTransformFeedback());
    }

    void ReadBuffer(GLenum mode SOURCE_LOCATION)
    {
        glCheck(glReadBuffer(mode));
    }

    void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glRenderbufferStorage(target, internalformat, width, height));
    }

    void RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glRenderbufferStorageMultisample(target, samples, internalformat, width, height));
    }

    void ResumeTransformFeedback(VOID_SOURCE_LOCATION)
    {
        glCheck(glResumeTransformFeedback());
    }

    void SamplerParameterf(GLuint sampler, GLenum pname, GLfloat param SOURCE_LOCATION)
    {
        glCheck(glSamplerParameterf(sampler, pname, param));
    }

    void SamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params SOURCE_LOCATION)
    {
        glCheck(glSamplerParameterfv(sampler, pname, params));
    }

    void SamplerParameteri(GLuint sampler, GLenum pname, GLint param SOURCE_LOCATION)
    {
        glCheck(glSamplerParameteri(sampler, pname, param));
    }

    void SamplerParameteriv(GLuint sampler, GLenum pname, const GLint* params SOURCE_LOCATION)
    {
        glCheck(glSamplerParameteriv(sampler, pname, params));
    }

    void TexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, data));
    }

    void TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth SOURCE_LOCATION)
    {
        glCheck(glTexStorage3D(target, levels, internalformat, width, height, depth));
    }

    void TexSubImage3D(
        GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION)
    {
        glCheck(glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data));
    }

    void TransformFeedbackVaryings(GLuint program, GLsizei count, const char** varyings, GLenum bufferMode SOURCE_LOCATION)
    {
        glCheck(glTransformFeedbackVaryings(program, count, varyings, bufferMode));
    }

    void UniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding SOURCE_LOCATION)
    {
        glCheck(glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding));
    }

    void VertexAttribDivisor(GLuint index, GLuint divisor SOURCE_LOCATION)
    {
        glCheck(glVertexAttribDivisor(index, divisor));
    }

    void VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION)
    {
        glCheck(glVertexAttribIPointer(index, size, type, stride, pointer));
    }

    void WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout SOURCE_LOCATION)
    {
        glCheck(glWaitSync(sync, flags, timeout));
    }

    void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION)
    {
        glCheck(glTexStorage2D(target, levels, internalformat, width, height));
    }

#if !defined(IS_WEBGL2)

    // OpenGL 4.3+ Debug functions
    void DebugMessageCallback(DEBUGPROC callback, const void* userParam SOURCE_LOCATION)
    {
        glCheck(glDebugMessageCallback(callback, userParam));
    }

    void DebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled SOURCE_LOCATION)
    {
        glCheck(glDebugMessageControl(source, type, severity, count, ids, enabled));
    }

#endif

}
