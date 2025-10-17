/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Renderer2DUtils.hpp"

#include <cmath>

namespace CS200::Renderer2DUtils
{
    Math::TransformationMatrix CalculateLineTransform(const Math::TransformationMatrix& transform, const Math::vec2& start_point, const Math::vec2& end_point, double line_width) noexcept
    {
        const Math::vec2 line_vector = end_point - start_point;

        const double            angle             = std::atan2(line_vector.y, line_vector.x);
        constexpr double        center_multiplier = 0.5;
        const Math::vec2        center_point      = (start_point + end_point) * center_multiplier;
        const Math::ScaleMatrix scale_matrix{
            { line_vector.Length(), line_width }
        };
        const Math::RotationMatrix    rotation_matrix{ angle };
        const Math::TranslationMatrix translation_matrix{ center_point };
        const auto                    final_transform = transform * translation_matrix * rotation_matrix * scale_matrix;
        return final_transform;
    }

    SDFTransform CalculateSDFTransform(const Math::TransformationMatrix& transform, double line_width) noexcept
    {
        const vec2      world_size{ static_cast<float>(std::sqrt(transform[0][0] * transform[0][0] + transform[1][0] * transform[1][0])),
                               static_cast<float>(std::sqrt(transform[0][1] * transform[0][1] + transform[1][1] * transform[1][1])) };
        constexpr float line_width_scale = 2.0;
        const vec2      quad_size        = { world_size[0] + line_width_scale * static_cast<float>(line_width), world_size[1] + line_width_scale * static_cast<float>(line_width) };

        const vec2 scale_up       = { quad_size[0] / world_size[0], quad_size[1] / world_size[1] };
        mat3       quad_transform = to_opengl_mat3(transform);
        quad_transform[0] *= scale_up[0];
        quad_transform[1] *= scale_up[0];
        quad_transform[3] *= scale_up[1];
        quad_transform[4] *= scale_up[1];
        return { quad_transform, world_size, quad_size };
    }
}
