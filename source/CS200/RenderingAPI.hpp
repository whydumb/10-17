/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Vec2.hpp"
#include "RGBA.hpp"

namespace CS200::RenderingAPI
{
    void Init() noexcept;
    void SetClearColor(CS200::RGBA color) noexcept;
    void Clear() noexcept;
    void SetViewport(Math::ivec2 size, Math::ivec2 anchor_left_bottom = { 0, 0 }) noexcept;
}
