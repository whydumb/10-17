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
#include "Handle.hpp"
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

namespace OpenGL
{
    using BufferHandle      = Handle;
    using VertexArrayHandle = Handle;

    namespace Attribute
    {
        struct Type
        {
            uint16_t GLType         : 16; // Bits 15-0   (16 bits): OpenGL component type (GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, etc.)
            uint8_t  ComponentCount : 3;  // Bits 18-16   (3 bits): Component count (1, 2, 3, 4)
            uint8_t  SizeBytes      : 5;  // Bits 23-19   (5 bits): Attribute size in bytes (max size is 16 - vec4)
            bool     Normalize      : 1;  // Bit  24      (1 bit) : Normalized flag (0 = false, 1 = true)
            bool     IntAttribute   : 1;  // Bit  25      (1 bit) : Integer attribute flag (0 = use glVertexAttribPointer, 1 = use glVertexAttribIPointer)
            uint8_t  Divisor        : 6;  // Bits 26-31   (6 bits): Divisor Value to support instancing (0-63)

            constexpr auto operator<=>(const Type&) const noexcept = default;

            constexpr Type& WithDivisor(uint8_t divisor_value) noexcept
            {
                Divisor = divisor_value & 0x3F; // only 6 bits
                return *this;
            }
        };

        static_assert(sizeof(Type) == sizeof(uint32_t));
    }

    struct BufferLayout
    {
        uint32_t                     BufferStartingByteOffset = 0;
        std::vector<Attribute::Type> Attributes{};

        BufferLayout() = default;

        BufferLayout(std::initializer_list<Attribute::Type> attributes) : BufferStartingByteOffset{ 0 }, Attributes{ attributes }
        {
        }

        BufferLayout(uint32_t starting_byte_offset, std::initializer_list<Attribute::Type> attributes) : BufferStartingByteOffset{ starting_byte_offset }, Attributes{ attributes }
        {
        }
    };

    struct VertexBuffer
    {
        BufferHandle Handle{ 0 };
        BufferLayout Layout{};
    };

    VertexArrayHandle CreateVertexArrayObject(std::initializer_list<VertexBuffer> vertices, BufferHandle index_buffer = 0);
    VertexArrayHandle CreateVertexArrayObject(VertexBuffer vertices, BufferHandle index_buffer = 0);

    namespace Attribute
    {
        namespace details
        {
            // Constants for encoding
            constexpr bool NORMALIZE    = true;
            constexpr bool NO_NORMALIZE = false;
            constexpr bool TO_INT       = true;  // Use glVertexAttribIPointer
            constexpr bool TO_FLOAT     = false; // Use glVertexAttribPointer

        }

        constexpr Type None                = { 0, 0, 0, 0, 0, 0 };
        constexpr Type Bool                = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool -> bool
        constexpr Type Bool2               = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool[2] -> bvec2
        constexpr Type Bool3               = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool[3] -> bvec3
        constexpr Type Bool4               = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // bool[4] -> bvec4
        constexpr Type Byte                = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t -> int
        constexpr Type Byte2               = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t[2] -> ivec2
        constexpr Type Byte2ToFloat        = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t[2] -> vec2
        constexpr Type Byte2ToNormalized   = { GL_BYTE, 2, 2 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t[2] -> vec2 [-1, 1]
        constexpr Type Byte3               = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t[3] -> ivec3
        constexpr Type Byte3ToFloat        = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t[3] -> vec3
        constexpr Type Byte3ToNormalized   = { GL_BYTE, 3, 3 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t[3] -> vec3 [-1, 1]
        constexpr Type Byte4               = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_INT, 0 };                     // int8_t[4] -> ivec4
        constexpr Type Byte4ToFloat        = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t[4] -> vec4
        constexpr Type Byte4ToNormalized   = { GL_BYTE, 4, 4 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t[4] -> vec4 [-1, 1]
        constexpr Type ByteToFloat         = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // int8_t -> float
        constexpr Type ByteToNormalized    = { GL_BYTE, 1, 1 * sizeof(int8_t), details::NORMALIZE, details::TO_FLOAT, 0 };                      // int8_t -> float [-1, 1]
        constexpr Type Float               = { GL_FLOAT, 1, 1 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float -> float
        constexpr Type Float2              = { GL_FLOAT, 2, 2 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float[2] -> vec2
        constexpr Type Float3              = { GL_FLOAT, 3, 3 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float[3] -> vec3
        constexpr Type Float4              = { GL_FLOAT, 4, 4 * sizeof(float), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // float[4] -> vec4
        constexpr Type Int                 = { GL_INT, 1, 1 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int -> int
        constexpr Type Int2                = { GL_INT, 2, 2 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int[2] -> ivec2
        constexpr Type Int2ToFloat         = { GL_INT, 2, 2 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int[2] -> vec2
        constexpr Type Int2ToNormalized    = { GL_INT, 2, 2 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int[2] -> vec2 [-1, 1]
        constexpr Type Int3                = { GL_INT, 3, 3 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int[3] -> ivec3
        constexpr Type Int3ToFloat         = { GL_INT, 3, 3 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int[3] -> vec3
        constexpr Type Int3ToNormalized    = { GL_INT, 3, 3 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int[3] -> vec3 [-1, 1]
        constexpr Type Int4                = { GL_INT, 4, 4 * sizeof(int), details::NO_NORMALIZE, details::TO_INT, 0 };                         // int[4] -> ivec4
        constexpr Type Int4ToFloat         = { GL_INT, 4, 4 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int[4] -> vec4
        constexpr Type Int4ToNormalized    = { GL_INT, 4, 4 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int[4] -> vec4 [-1, 1]
        constexpr Type IntToFloat          = { GL_INT, 1, 1 * sizeof(int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                       // int -> float
        constexpr Type IntToNormalized     = { GL_INT, 1, 1 * sizeof(int), details::NORMALIZE, details::TO_FLOAT, 0 };                          // int -> float [-1, 1]
        constexpr Type Short               = { GL_SHORT, 1, 1 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short -> int
        constexpr Type Short2              = { GL_SHORT, 2, 2 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short[2] -> ivec2
        constexpr Type Short2ToFloat       = { GL_SHORT, 2, 2 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short[2] -> vec2
        constexpr Type Short2ToNormalized  = { GL_SHORT, 2, 2 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short[2] -> vec2 [-1, 1]
        constexpr Type Short3              = { GL_SHORT, 3, 3 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short[3] -> ivec3
        constexpr Type Short3ToFloat       = { GL_SHORT, 3, 3 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short[3] -> vec3
        constexpr Type Short3ToNormalized  = { GL_SHORT, 3, 3 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short[3] -> vec3 [-1, 1]
        constexpr Type Short4              = { GL_SHORT, 4, 4 * sizeof(short), details::NO_NORMALIZE, details::TO_INT, 0 };                     // short[4] -> ivec4
        constexpr Type Short4ToFloat       = { GL_SHORT, 4, 4 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short[4] -> vec4
        constexpr Type Short4ToNormalized  = { GL_SHORT, 4, 4 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short[4] -> vec4 [-1, 1]
        constexpr Type ShortToFloat        = { GL_SHORT, 1, 1 * sizeof(short), details::NO_NORMALIZE, details::TO_FLOAT, 0 };                   // short -> float
        constexpr Type ShortToNormalized   = { GL_SHORT, 1, 1 * sizeof(short), details::NORMALIZE, details::TO_FLOAT, 0 };                      // short -> float [-1, 1]
        constexpr Type UByte               = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t -> uint
        constexpr Type UByte2              = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t[2] -> uvec2
        constexpr Type UByte2ToFloat       = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t[2] -> vec2
        constexpr Type UByte2ToNormalized  = { GL_UNSIGNED_BYTE, 2, 2 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t[2] -> vec2 [0, 1]
        constexpr Type UByte3              = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t[3] -> uvec3
        constexpr Type UByte3ToFloat       = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t[3] -> vec3
        constexpr Type UByte3ToNormalized  = { GL_UNSIGNED_BYTE, 3, 3 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t[3] -> vec3 [0, 1]
        constexpr Type UByte4              = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_INT, 0 };           // uint8_t[4] -> uvec4
        constexpr Type UByte4ToFloat       = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t[4] -> vec4
        constexpr Type UByte4ToNormalized  = { GL_UNSIGNED_BYTE, 4, 4 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t[4] -> vec4 [0, 1]
        constexpr Type UByteToFloat        = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NO_NORMALIZE, details::TO_FLOAT, 0 };         // uint8_t -> float
        constexpr Type UByteToNormalized   = { GL_UNSIGNED_BYTE, 1, 1 * sizeof(uint8_t), details::NORMALIZE, details::TO_FLOAT, 0 };            // uint8_t -> float [0, 1]
        constexpr Type UInt                = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint -> uint
        constexpr Type UInt2               = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint[2] -> uvec2
        constexpr Type UInt2ToFloat        = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint[2] -> vec2
        constexpr Type UInt2ToNormalized   = { GL_UNSIGNED_INT, 2, 2 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint[2] -> vec2 [0, 1]
        constexpr Type UInt3               = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint[3] -> uvec3
        constexpr Type UInt3ToFloat        = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint[3] -> vec3
        constexpr Type UInt3ToNormalized   = { GL_UNSIGNED_INT, 3, 3 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint[3] -> vec3 [0, 1]
        constexpr Type UInt4               = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_INT, 0 };       // uint[4] -> uvec4
        constexpr Type UInt4ToFloat        = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint[4] -> vec4
        constexpr Type UInt4ToNormalized   = { GL_UNSIGNED_INT, 4, 4 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint[4] -> vec4 [0, 1]
        constexpr Type UIntToFloat         = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NO_NORMALIZE, details::TO_FLOAT, 0 };     // uint -> float
        constexpr Type UIntToNormalized    = { GL_UNSIGNED_INT, 1, 1 * sizeof(unsigned int), details::NORMALIZE, details::TO_FLOAT, 0 };        // uint -> float [0, 1]
        constexpr Type UShort              = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort -> uint
        constexpr Type UShort2             = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort[2] -> uvec2
        constexpr Type UShort2ToFloat      = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort[2] -> vec2
        constexpr Type UShort2ToNormalized = { GL_UNSIGNED_SHORT, 2, 2 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort[2] -> vec2 [0, 1]
        constexpr Type UShort3             = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort[3] -> uvec3
        constexpr Type UShort3ToFloat      = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort[3] -> vec3
        constexpr Type UShort3ToNormalized = { GL_UNSIGNED_SHORT, 3, 3 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort[3] -> vec3 [0, 1]
        constexpr Type UShort4             = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_INT, 0 };   // ushort[4] -> uvec4
        constexpr Type UShort4ToFloat      = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort[4] -> vec4
        constexpr Type UShort4ToNormalized = { GL_UNSIGNED_SHORT, 4, 4 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort[4] -> vec4 [0, 1]
        constexpr Type UShortToFloat       = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NO_NORMALIZE, details::TO_FLOAT, 0 }; // ushort -> float
        constexpr Type UShortToNormalized  = { GL_UNSIGNED_SHORT, 1, 1 * sizeof(unsigned short), details::NORMALIZE, details::TO_FLOAT, 0 };    // ushort -> float [0, 1]
    };


}
