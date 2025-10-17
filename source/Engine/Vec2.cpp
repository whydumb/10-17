/**
 * \file
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Vec2.hpp" 
#include <cmath>

namespace Math
{
    bool vec2::operator==(const vec2& v) const
    {
        return x == v.x && y == v.y;
    }

    bool vec2::operator!=(const vec2& v) const
    {
        return !(*this == v);
    }

    vec2 vec2::operator+(const vec2& v) const
    {
        return { x + v.x, y + v.y };
    }

    vec2& vec2::operator+=(const vec2& v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    vec2 vec2::operator-(const vec2& v) const
    {
        return { x - v.x, y - v.y };
    }

    vec2& vec2::operator-=(const vec2& v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    vec2 vec2::operator*(double scale) const
    {
        return { x * scale, y * scale };
    }

    vec2& vec2::operator*=(double scale)
    {
        x *= scale;
        y *= scale;
        return *this;
    }

    vec2 vec2::operator/(double divisor) const
    {
        return { x / divisor, y / divisor };
    }

    vec2& vec2::operator/=(double divisor)
    {
        x /= divisor;
        y /= divisor;
        return *this;
    }

    vec2 operator*(double scale, const vec2& v)
    {
        return { v.x * scale, v.y * scale };
    }

    bool ivec2::operator==(const ivec2& v) const
    {
        return x == v.x && y == v.y;
    }

    bool ivec2::operator!=(const ivec2& v) const
    {
        return !(*this == v);
    }

    ivec2 ivec2::operator+(const ivec2& v) const
    {
        return { x + v.x, y + v.y };
    }

    ivec2& ivec2::operator+=(const ivec2& v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    ivec2 ivec2::operator-(const ivec2& v) const
    {
        return { x - v.x, y - v.y };
    }

    ivec2& ivec2::operator-=(const ivec2& v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    ivec2 ivec2::operator*(int scale) const
    {
        return { x * scale, y * scale };
    }

    ivec2& ivec2::operator*=(int scale)
    {
        x *= scale;
        y *= scale;
        return *this;
    }

    ivec2 ivec2::operator/(int divisor) const
    {
        return { x / divisor, y / divisor };
    }

    ivec2& ivec2::operator/=(int divisor)
    {
        x /= divisor;
        y /= divisor;
        return *this;
    }

    vec2 ivec2::operator*(double scale) const
    {
        return { x * scale, y * scale };
    }

    vec2 ivec2::operator/(double divisor) const
    {
        return { x / divisor, y / divisor };
    }

    ivec2::operator vec2() const
    {
        return vec2{ static_cast<double>(x), static_cast<double>(y) };
    }
}
