/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Matrix.hpp"
#include <cmath>



namespace Math
{
    TransformationMatrix::TransformationMatrix()
    {
        Reset();
    }

    void TransformationMatrix::Reset()
    {
        matrix[0][0] = 1.0; matrix[0][1] = 0.0; matrix[0][2] = 0.0;
        matrix[1][0] = 0.0; matrix[1][1] = 1.0; matrix[1][2] = 0.0;
        matrix[2][0] = 0.0; matrix[2][1] = 0.0; matrix[2][2] = 1.0;
    }

    

    TransformationMatrix TransformationMatrix::operator*(TransformationMatrix rhs) const
    {
        TransformationMatrix result;

        for (int row = 0; row < 3; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                result[row][col] = 0.0;
                for (int k = 0; k < 3; ++k)
                {
                    result[row][col] += matrix[row][k] * rhs[k][col];
                }
            }
        }

        return result;
    }

    TransformationMatrix& TransformationMatrix::operator*=(TransformationMatrix rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    vec2 TransformationMatrix::operator*(vec2 v) const
    {
        double x = matrix[0][0] * v.x + matrix[0][1] * v.y + matrix[0][2];
        double y = matrix[1][0] * v.x + matrix[1][1] * v.y + matrix[1][2];

        return vec2{ static_cast<double>(x), static_cast<double>(y) };
    }

    TranslationMatrix::TranslationMatrix(ivec2 translate)
    {
        Reset();
        matrix[0][2] = static_cast<double>(translate.x);
        matrix[1][2] = static_cast<double>(translate.y);
    }

    TranslationMatrix::TranslationMatrix(vec2 translate)
    {
        Reset();
        matrix[0][2] = static_cast<double>(translate.x);
        matrix[1][2] = static_cast<double>(translate.y);
    }

    ScaleMatrix::ScaleMatrix(double scale)
    {
        Reset();
        matrix[0][0] = scale;
        matrix[1][1] = scale;
    }

    ScaleMatrix::ScaleMatrix(vec2 scale)
    {
        Reset();
        matrix[0][0] = static_cast<double>(scale.x);
        matrix[1][1] = static_cast<double>(scale.y);
    }

    RotationMatrix::RotationMatrix(double theta)
    {
        Reset();
        double cos_theta = std::cos(theta);
        double sin_theta = std::sin(theta);

        matrix[0][0] = cos_theta;
        matrix[0][1] = -sin_theta;
        matrix[1][0] = sin_theta;
        matrix[1][1] = cos_theta;
    }
}
