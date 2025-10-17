/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Engine/Matrix.hpp"
#include "Engine/Vec2.hpp"
#include "RGBA.hpp"
#include <array>
#include <optional>

namespace CS200::Renderer2DUtils
{
   
    using mat3 = std::array<float, 9>; 
    using vec2 = std::array<float, 2>; 

 
    inline mat3 to_opengl_mat3(const Math::TransformationMatrix& transform) noexcept
    {
        return { static_cast<float>(transform[0][0]), static_cast<float>(transform[1][0]), static_cast<float>(transform[2][0]),
                 static_cast<float>(transform[0][1]), static_cast<float>(transform[1][1]), static_cast<float>(transform[2][1]),
                 static_cast<float>(transform[0][2]), static_cast<float>(transform[1][2]), static_cast<float>(transform[2][2]) };
    }

    constexpr std::array<float, 4> unpack_color(CS200::RGBA rgba) noexcept
    {
        constexpr float scale = 1.0f / 255.0f;
        return { static_cast<float>((rgba & 0xff000000) >> 24) * scale, static_cast<float>((rgba & 0x00ff0000) >> 16) * scale, static_cast<float>((rgba & 0x0000ff00) >> 8) * scale,
                 static_cast<float>((rgba & 0x000000ff) >> 0) * scale };
    }

  
    Math::TransformationMatrix CalculateLineTransform(const Math::TransformationMatrix& transform, const Math::vec2& start_point, const Math::vec2& end_point, double line_width) noexcept;

   
    struct SDFTransform
    {
        mat3 QuadTransform; 
        vec2 WorldSize;     
        vec2 QuadSize;  
    };

    
    SDFTransform CalculateSDFTransform(const Math::TransformationMatrix& transform, double line_width) noexcept;
}
