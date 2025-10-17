/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include "GLConstants.hpp"
#include "GLTypes.hpp"


#if defined(DEVELOPER_VERSION)
#    include <source_location>
#    define VOID_SOURCE_LOCATION const std::source_location caller_location = std::source_location::current()
#    define SOURCE_LOCATION      , VOID_SOURCE_LOCATION
#else
#    define SOURCE_LOCATION
#    define VOID_SOURCE_LOCATION void
#endif


namespace GL
{
    // Opengl Version 2.0
    const GLubyte* GetString(GLenum name SOURCE_LOCATION);
    GLboolean      IsBuffer(GLuint buffer SOURCE_LOCATION);
    GLboolean      IsEnabled(GLenum cap SOURCE_LOCATION);
    GLboolean      IsProgram(GLuint program SOURCE_LOCATION);
    GLboolean      IsShader(GLuint shader SOURCE_LOCATION);
    GLboolean      IsTexture(GLuint texture SOURCE_LOCATION);
    GLenum         GetError(void);
    GLint          GetAttribLocation(GLuint program, const GLchar* name SOURCE_LOCATION);
    GLint          GetUniformLocation(GLuint program, const GLchar* name SOURCE_LOCATION);
    GLuint         CreateProgram(VOID_SOURCE_LOCATION);
    GLuint         CreateShader(GLenum shaderType SOURCE_LOCATION);
    void           ActiveTexture(GLenum texture SOURCE_LOCATION);
    void           AttachShader(GLuint program, GLuint shader SOURCE_LOCATION);
    void           BindBuffer(GLenum target, GLuint buffer SOURCE_LOCATION);
    void           BindBufferBase(GLenum target, GLuint index, GLuint buffer SOURCE_LOCATION);
    void           BindTexture(GLenum target, GLuint texture SOURCE_LOCATION);
    void           BlendEquation(GLenum mode SOURCE_LOCATION);
    void           BlendFunc(GLenum sfactor, GLenum dfactor SOURCE_LOCATION);
    void           BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage SOURCE_LOCATION);
    void           BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data SOURCE_LOCATION);
    void           Clear(GLbitfield mask SOURCE_LOCATION);
    void           ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha SOURCE_LOCATION);
    void           ClearDepth(GLdouble depth SOURCE_LOCATION);
    void           ClearStencil(GLint s SOURCE_LOCATION);
    void           ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha SOURCE_LOCATION);
    void           CompileShader(GLuint shader SOURCE_LOCATION);
    void           CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border SOURCE_LOCATION);
    void           CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION);
    void           CullFace(GLenum mode SOURCE_LOCATION);
    void           DeleteBuffers(GLsizei n, const GLuint* buffers SOURCE_LOCATION);
    void           DeleteProgram(GLuint program SOURCE_LOCATION);
    void           DeleteShader(GLuint shader SOURCE_LOCATION);
    void           DeleteTextures(GLsizei n, const GLuint* textures SOURCE_LOCATION);
    void           DepthMask(GLboolean flag SOURCE_LOCATION);
    void           DepthRange(GLdouble nearVal, GLdouble farVal SOURCE_LOCATION);
    void           DetachShader(GLuint program, GLuint shader SOURCE_LOCATION);
    void           Disable(GLenum cap SOURCE_LOCATION);
    void           DisableVertexAttribArray(GLuint index SOURCE_LOCATION);
    void           DrawArrays(GLenum mode, GLint first, GLsizei count SOURCE_LOCATION);
    void           DrawBuffers(GLsizei n, const GLenum* bufs SOURCE_LOCATION);
    void           DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices SOURCE_LOCATION);
    void           DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices SOURCE_LOCATION);
    void           Enable(GLenum cap SOURCE_LOCATION);
    void           EnableVertexAttribArray(GLuint index SOURCE_LOCATION);
    void           FrontFace(GLenum mode SOURCE_LOCATION);
    void           GenBuffers(GLsizei n, GLuint* buffers SOURCE_LOCATION);
    void           GenTextures(GLsizei n, GLuint* textures SOURCE_LOCATION);
    void           GetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION);
    void           GetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name SOURCE_LOCATION);
    void           GetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders SOURCE_LOCATION);
    void           GetBooleanv(GLenum pname, GLboolean* data SOURCE_LOCATION);
    void           GetFloatv(GLenum pname, GLfloat* data SOURCE_LOCATION);
    void           GetIntegerv(GLenum pname, GLint* data SOURCE_LOCATION);
    void           GetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION);
    void           GetProgramiv(GLuint program, GLenum pname, GLint* params SOURCE_LOCATION);
    void           GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog SOURCE_LOCATION);
    void           GetShaderiv(GLuint shader, GLenum pname, GLint* params SOURCE_LOCATION);
    void           GetShaderSource(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source SOURCE_LOCATION);
    void           GetTexParameterfv(GLenum target, GLenum pname, GLfloat* params SOURCE_LOCATION);
    void           GetTexParameteriv(GLenum target, GLenum pname, GLint* params SOURCE_LOCATION);
    void           GetUniformfv(GLuint program, GLint location, GLfloat* params SOURCE_LOCATION);
    void           GetUniformiv(GLuint program, GLint location, GLint* params SOURCE_LOCATION);
    void           GetUniformuiv(GLuint program, GLint location, GLuint* params SOURCE_LOCATION);
    void           GetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params SOURCE_LOCATION);
    void           GetVertexAttribiv(GLuint index, GLenum pname, GLint* params SOURCE_LOCATION);
    void           GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer SOURCE_LOCATION);
    void           Hint(GLenum target, GLenum mode SOURCE_LOCATION);
    void           LineWidth(GLfloat width SOURCE_LOCATION);
    void           LinkProgram(GLuint program SOURCE_LOCATION);
    void           PixelStorei(GLenum pname, GLint param SOURCE_LOCATION);
    void           PolygonOffset(GLfloat factor, GLfloat units SOURCE_LOCATION);
    void           ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels SOURCE_LOCATION);
    void           Scissor(GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION);
    void           ShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length SOURCE_LOCATION);
    void           StencilMask(GLuint mask SOURCE_LOCATION);
    void           StencilMaskSeparate(GLenum face, GLuint mask SOURCE_LOCATION);
    void           TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION);
    void           TexParameterf(GLenum target, GLenum pname, GLfloat param SOURCE_LOCATION);
    void           TexParameterfv(GLenum target, GLenum pname, const GLfloat* params SOURCE_LOCATION);
    void           TexParameteri(GLenum target, GLenum pname, GLint param SOURCE_LOCATION);
    void           TexParameteriv(GLenum target, GLenum pname, const GLint* params SOURCE_LOCATION);
    void           TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels SOURCE_LOCATION);
    void           Uniform1f(GLint location, GLfloat v0 SOURCE_LOCATION);
    void           Uniform1fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION);
    void           Uniform1i(GLint location, GLint v0 SOURCE_LOCATION);
    void           Uniform1iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION);
    void           Uniform1ui(GLint location, GLuint v0 SOURCE_LOCATION);
    void           Uniform1uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION);
    void           Uniform2f(GLint location, GLfloat v0, GLfloat v1 SOURCE_LOCATION);
    void           Uniform2fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION);
    void           Uniform2i(GLint location, GLint v0, GLint v1 SOURCE_LOCATION);
    void           Uniform2iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION);
    void           Uniform2ui(GLint location, GLuint v0, GLuint v1 SOURCE_LOCATION);
    void           Uniform2uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION);
    void           Uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2 SOURCE_LOCATION);
    void           Uniform3fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION);
    void           Uniform3i(GLint location, GLint v0, GLint v1, GLint v2 SOURCE_LOCATION);
    void           Uniform3iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION);
    void           Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2 SOURCE_LOCATION);
    void           Uniform3uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION);
    void           Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 SOURCE_LOCATION);
    void           Uniform4fv(GLint location, GLsizei count, const GLfloat* value SOURCE_LOCATION);
    void           Uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3 SOURCE_LOCATION);
    void           Uniform4iv(GLint location, GLsizei count, const GLint* value SOURCE_LOCATION);
    void           Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 SOURCE_LOCATION);
    void           Uniform4uiv(GLint location, GLsizei count, const GLuint* value SOURCE_LOCATION);
    void           UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value SOURCE_LOCATION);
    void           UseProgram(GLuint program SOURCE_LOCATION);
    void           ValidateProgram(GLuint program SOURCE_LOCATION);
    void           VertexAttrib1f(GLuint index, GLfloat v0 SOURCE_LOCATION);
    void           VertexAttrib1fv(GLuint index, const GLfloat* v SOURCE_LOCATION);
    void           VertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1 SOURCE_LOCATION);
    void           VertexAttrib2fv(GLuint index, const GLfloat* v SOURCE_LOCATION);
    void           VertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2 SOURCE_LOCATION);
    void           VertexAttrib3fv(GLuint index, const GLfloat* v SOURCE_LOCATION);
    void           VertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 SOURCE_LOCATION);
    void           VertexAttrib4fv(GLuint index, const GLfloat* v SOURCE_LOCATION);
    void           VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION);
    void           Viewport(GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION);


    // Opengl Version 3.0
    GLboolean IsFramebuffer(GLuint framebuffer SOURCE_LOCATION);
    GLboolean IsQuery(GLuint id SOURCE_LOCATION);
    GLboolean IsRenderbuffer(GLuint renderbuffer SOURCE_LOCATION);
    GLboolean IsSampler(GLuint id SOURCE_LOCATION);
    GLboolean IsSync(GLsync sync SOURCE_LOCATION);
    GLboolean IsTransformFeedback(GLuint id SOURCE_LOCATION);
    GLenum    CheckFramebufferStatus(GLenum target SOURCE_LOCATION);
    GLenum    ClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout SOURCE_LOCATION);
    GLint     GetFragDataLocation(GLuint program, const char* name SOURCE_LOCATION);
    GLsync    FenceSync(GLenum condition, GLbitfield flags SOURCE_LOCATION);
    GLuint    GetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName SOURCE_LOCATION);
    void      BeginQuery(GLenum target, GLuint id SOURCE_LOCATION);
    void      BeginTransformFeedback(GLenum primitiveMode SOURCE_LOCATION);
    void      BindFramebuffer(GLenum target, GLuint framebuffer SOURCE_LOCATION);
    void      BindRenderbuffer(GLenum target, GLuint renderbuffer SOURCE_LOCATION);
    void      BindVertexArray(GLuint array SOURCE_LOCATION);
    void      ClearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil SOURCE_LOCATION);
    void      ClearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat* value SOURCE_LOCATION);
    void      ClearBufferiv(GLenum buffer, GLint drawBuffer, const GLint* value SOURCE_LOCATION);
    void      ClearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint* value SOURCE_LOCATION);
    void      ClearDepthf(GLfloat depth SOURCE_LOCATION);
    void      CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION);
    void CompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION);
    void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION);
    void CompressedTexSubImage3D(
        GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data SOURCE_LOCATION);
    void CopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size SOURCE_LOCATION);
    void CopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height SOURCE_LOCATION);
    void CreateRenderbuffers(GLsizei n, GLuint* renderbuffers SOURCE_LOCATION);
    void CreateSamplers(GLsizei n, GLuint* samplers SOURCE_LOCATION);
    void CreateTransformFeedbacks(GLsizei n, GLuint* ids SOURCE_LOCATION);
    void DeleteFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION);
    void DeleteQueries(GLsizei n, const GLuint* ids SOURCE_LOCATION);
    void DeleteRenderbuffers(GLsizei n, GLuint* renderbuffers SOURCE_LOCATION);
    void DeleteSamplers(GLsizei n, const GLuint* samplers SOURCE_LOCATION);
    void DeleteSync(GLsync sync SOURCE_LOCATION);
    void DeleteTransformFeedbacks(GLsizei n, const GLuint* ids SOURCE_LOCATION);
    void DeleteVertexArrays(GLsizei n, const GLuint* arrays SOURCE_LOCATION);
    void DepthRangef(GLfloat n, GLfloat f SOURCE_LOCATION);
    void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount SOURCE_LOCATION);
    void DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount SOURCE_LOCATION);
    void EndQuery(GLenum target SOURCE_LOCATION);
    void EndTransformFeedback(VOID_SOURCE_LOCATION);
    void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer SOURCE_LOCATION);
    void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level SOURCE_LOCATION);
    void FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer SOURCE_LOCATION);
    void GenerateMipmap(GLenum target SOURCE_LOCATION);
    void GenFramebuffers(GLsizei n, GLuint* framebuffers SOURCE_LOCATION);
    void GenQueries(GLsizei n, GLuint* ids SOURCE_LOCATION);
    void GenRenderbuffers(GLsizei n, GLuint* renderbuffers SOURCE_LOCATION);
    void GenSamplers(GLsizei n, GLuint* samplers SOURCE_LOCATION);
    void GenTransformFeedbacks(GLsizei n, GLuint* ids SOURCE_LOCATION);
    void GenVertexArrays(GLsizei n, GLuint* arrays SOURCE_LOCATION);
    void GetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params SOURCE_LOCATION);
    void GetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName SOURCE_LOCATION);
    void GetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params SOURCE_LOCATION);
    void GetBooleani_v(GLenum target, GLuint index, GLboolean* data SOURCE_LOCATION);
    void GetBufferParameteri64v(GLenum target, GLenum value, GLint64* data SOURCE_LOCATION);
    void GetBufferParameteriv(GLenum target, GLenum value, GLint* data SOURCE_LOCATION);
    void GetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data SOURCE_LOCATION);
    void GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params SOURCE_LOCATION);
    void GetInteger64i_v(GLenum target, GLuint index, GLint64* data SOURCE_LOCATION);
    void GetInteger64v(GLenum pname, GLint64* data SOURCE_LOCATION);
    void GetIntegeri_v(GLenum target, GLuint index, GLint* data SOURCE_LOCATION);
    void GetQueryiv(GLenum target, GLenum pname, GLint* params SOURCE_LOCATION);
    void GetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params SOURCE_LOCATION);
    void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params SOURCE_LOCATION);
    void GetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params SOURCE_LOCATION);
    void GetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params SOURCE_LOCATION);
    void GetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values SOURCE_LOCATION);
    void GetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, char* name SOURCE_LOCATION);
    void GetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar** uniformNames, GLuint* uniformIndices SOURCE_LOCATION);
    void GetVertexAttribIiv(GLuint index, GLenum pname, GLint* params SOURCE_LOCATION);
    void GetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params SOURCE_LOCATION);
    void PauseTransformFeedback(VOID_SOURCE_LOCATION);
    void ReadBuffer(GLenum mode SOURCE_LOCATION);
    void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION);
    void RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION);
    void ResumeTransformFeedback(VOID_SOURCE_LOCATION);
    void SamplerParameterf(GLuint sampler, GLenum pname, GLfloat param SOURCE_LOCATION);
    void SamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* params SOURCE_LOCATION);
    void SamplerParameteri(GLuint sampler, GLenum pname, GLint param SOURCE_LOCATION);
    void SamplerParameteriv(GLuint sampler, GLenum pname, const GLint* params SOURCE_LOCATION);
    void TexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION);
    void TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth SOURCE_LOCATION);
    void TexSubImage3D(
        GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* data SOURCE_LOCATION);
    void TransformFeedbackVaryings(GLuint program, GLsizei count, const char** varyings, GLenum bufferMode SOURCE_LOCATION);
    void UniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding SOURCE_LOCATION);
    void VertexAttribDivisor(GLuint index, GLuint divisor SOURCE_LOCATION);
    void VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer SOURCE_LOCATION);
    void WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout SOURCE_LOCATION);

    // Opengl Version 3.2
    void TexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations SOURCE_LOCATION);

    // Opengl ES 3.0 or Opengl Version 4.2
    void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height SOURCE_LOCATION);


    // Opengl 4.3
    void DebugMessageCallback(DEBUGPROC callback, const void* userParam SOURCE_LOCATION);
    void DebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled SOURCE_LOCATION);


}

#undef SOURCE_LOCATION
#undef VOID_SOURCE_LOCATION
