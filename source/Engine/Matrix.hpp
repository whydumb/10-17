/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#pragma once
#include "Vec2.hpp"

namespace Math
{
    class TransformationMatrix
    {
    public:
        TransformationMatrix();

        constexpr double* operator[](int index)
        {
            return matrix[index];
        }

        constexpr const double* operator[](int index) const
        {
            return matrix[index];
        }

        TransformationMatrix  operator*(TransformationMatrix m) const;
        TransformationMatrix& operator*=(TransformationMatrix m);
        vec2                  operator*(vec2 v) const;
        void                  Reset();

    protected:
        double matrix[3][3];
    };

    class TranslationMatrix : public TransformationMatrix
    {
    public:
        TranslationMatrix(ivec2 translate);
        TranslationMatrix(vec2 translate);
    };

    class RotationMatrix : public TransformationMatrix
    {
    public:
        RotationMatrix(double theta);
    };

    class ScaleMatrix : public TransformationMatrix
    {
    public:
        ScaleMatrix(double scale);
        ScaleMatrix(vec2 scale);
    };
}
