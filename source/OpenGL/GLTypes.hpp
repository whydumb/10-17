/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include <cstddef> // for ptrdiff_t
#include <cstdint> // for fixed-width integer types

// OpenGL Type Definitions using precise fixed-width types
typedef uint32_t         GLenum;     // OpenGL enumeration values
typedef uint32_t         GLbitfield; // OpenGL bitfield values
typedef uint32_t         GLuint;     // Unsigned 32-bit integer
typedef int32_t          GLint;      // Signed 32-bit integer
typedef int32_t          GLsizei;    // Non-negative size values
typedef uint8_t          GLboolean;  // Boolean values (0 or 1)
typedef int8_t           GLbyte;     // Signed 8-bit integer
typedef int16_t          GLshort;    // Signed 16-bit integer
typedef uint8_t          GLubyte;    // Unsigned 8-bit integer
typedef uint16_t         GLushort;   // Unsigned 16-bit integer
typedef unsigned long    GLulong;    // Unsigned 64-bit integer
typedef float            GLfloat;    // 32-bit floating point
typedef float            GLclampf;   // Clamped 32-bit floating point
typedef double           GLdouble;   // 64-bit floating point
typedef double           GLclampd;   // Clamped 64-bit floating point
typedef void             GLvoid;     // Void type
typedef char             GLchar;     // Character type
typedef ptrdiff_t        GLintptr;   // Pointer-sized signed integer
typedef ptrdiff_t        GLsizeiptr; // Pointer-sized size value
typedef int64_t          GLint64;    // Signed 64-bit integer
typedef uint64_t         GLuint64;   // Unsigned 64-bit integer
typedef struct __GLsync* GLsync;     // Sync object pointer


#if not defined(APIENTRY)
#    if defined(__MINGW32__) || defined(__CYGWIN__) || (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
#        define APIENTRY __stdcall
#    else
#        define APIENTRY
#    endif
#endif

typedef void(APIENTRY* DEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
