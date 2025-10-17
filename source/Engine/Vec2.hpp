/**
 * \file
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include <limits>
#include <cmath>

namespace Math {
    struct vec2 {
        double x{ 0.0 };
        double y{ 0.0 };

        constexpr vec2() = default;
        constexpr vec2(double _x, double _y) : x(_x), y(_y) {}

        bool operator==(const vec2& v) const;
        bool operator!=(const vec2& v) const;

        vec2 operator+(const vec2& v) const;
        vec2& operator+=(const vec2& v);

        vec2 operator-(const vec2& v) const;
        vec2& operator-=(const vec2& v);

        vec2 operator*(double scale) const;
        vec2& operator*=(double scale);

        vec2 operator/(double divisor) const;
        vec2& operator/=(double divisor);

        double Length() const {
            return std::sqrt(x * x + y * y);
        }

        vec2 Normalize() const {
            double len = Length();
            if (len > std::numeric_limits<double>::epsilon())
                return { x / len, y / len };
            return { 0.0, 0.0 };
        }
    };

    vec2 operator*(double scale, const vec2& v);


    struct ivec2 {
        int x{ 0 };
        int y{ 0 };

        constexpr ivec2() = default;
        constexpr ivec2(int _x, int _y) : x(_x), y(_y) {}
        
        explicit operator vec2() const;

        bool operator==(const ivec2& v) const;
        bool operator!=(const ivec2& v) const;

        ivec2 operator+(const ivec2& v) const;
        ivec2& operator+=(const ivec2& v);

        ivec2 operator-(const ivec2& v) const;
        ivec2& operator-=(const ivec2& v);

        ivec2 operator*(int scale) const;
        ivec2& operator*=(int scale);

        ivec2 operator/(int divisor) const;
        ivec2& operator/=(int divisor);

        vec2 operator*(double scale) const;
        vec2 operator/(double divisor) const;
    };
}
