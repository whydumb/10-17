/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <array>
#include <cstdint>

namespace CS200
{
    using RGBA           = uint32_t;
    constexpr RGBA WHITE = 0xffffffff;
    constexpr RGBA BLACK = 0x000000ff;
    constexpr RGBA CLEAR = 0x00000000;

    constexpr std::array<float, 4> unpack_color(RGBA rgba) noexcept
    {
        constexpr float scale = 1.0f / 255.0f;
        return { static_cast<float>((rgba & 0xff000000) >> 24) * scale, static_cast<float>((rgba & 0x00ff0000) >> 16) * scale, static_cast<float>((rgba & 0x0000ff00) >> 8) * scale,
                 static_cast<float>((rgba & 0x000000ff) >> 0) * scale };
    }

    constexpr RGBA pack_color(const std::array<float, 4>& color) noexcept
    {
        constexpr float scale = 255.0f;
        const float     r = color[0] * scale, g = color[1] * scale, b = color[2] * scale, a = color[3] * scale;
        const uint8_t   R = static_cast<uint8_t>(r <= 255.0f ? (r >= 0.0f ? r : 0.0f) : 255.0f);
        const uint8_t   G = static_cast<uint8_t>(g <= 255.0f ? (g >= 0.0f ? g : 0.0f) : 255.0f);
        const uint8_t   B = static_cast<uint8_t>(b <= 255.0f ? (b >= 0.0f ? b : 0.0f) : 255.0f);
        const uint8_t   A = static_cast<uint8_t>(a <= 255.0f ? (a >= 0.0f ? a : 0.0f) : 255.0f);
        return (static_cast<RGBA>(R) << 24) | (static_cast<RGBA>(G) << 16) | (static_cast<RGBA>(B) << 8) | (static_cast<RGBA>(A) << 0);
    }

    // Convert RGBA format (0xRRGGBBAA) to ABGR format (0xAABBGGRR) by reversing bytes
    constexpr uint32_t rgba_to_abgr(RGBA rgba) noexcept
    {
        const uint8_t r = static_cast<uint8_t>((rgba & 0xff000000) >> 24);
        const uint8_t g = static_cast<uint8_t>((rgba & 0x00ff0000) >> 16);
        const uint8_t b = static_cast<uint8_t>((rgba & 0x0000ff00) >> 8);
        const uint8_t a = static_cast<uint8_t>((rgba & 0x000000ff) >> 0);

        return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(r) << 0);
    }


}
