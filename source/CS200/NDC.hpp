/**
 * \file
 * \author Rudy Castan
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once
#include "Engine/Matrix.hpp"
#include "Engine/Vec2.hpp"

namespace CS200
{
    /**
     * \brief Create a transformation matrix that converts screen coordinates to NDC
     * \param view_size Width and height of the viewport/screen in pixels
     * \return Transformation matrix for screen-to-NDC conversion
     *
     * \note The returned matrix transforms FROM screen coordinates TO NDC coordinates
     * \note This is typically used once per frame in BeginScene()
     * \note Screen coordinates assume (0,0) at bottom-left, (width,height) at top-right
     *
     * NDC (Normalized Device Coordinates) is a standard coordinate system used by graphics
     * APIs where all visible coordinates range from -1 to +1 in both X and Y axes.
     * This system is hardware-standard and allows graphics pipelines to work consistently
     * across different screen resolutions and aspect ratios.
     *
     * Purpose and Benefits:
     * - Provides resolution-independent coordinate system for rendering
     * - Standardizes coordinate space for graphics hardware (GPUs expect NDC)
     * - Enables consistent camera/viewport transformations
     * - Simplifies clipping and culling operations in graphics pipeline
     * - Makes shaders and rendering code portable across different screen sizes
     *
     * Coordinate System Mapping:
     * \code
     * We want to map (0, w) to (-1,1) and (0,h) to (-1,1)
     *             (w,h)                  (1,1)
     *      +--------+             +--------+
     *      |        |             |        |
     *      | cam    |     --->    | ndc    |
     *      +--------+             +--------+
     *    (0,0)                 (-1,-1)
     * \endcode
     *
     * Mathematical Transformation:
     * 1. Scale: Divide by half-dimensions to get 0-2 range
     * 2. Translate: Subtract 1 to center around origin (-1 to +1)
     * 3. Formula: ndc = (screen_coord / (dimension/2)) - 1
     *
     * Common Use Cases:
     * - Camera/view matrix setup for 2D rendering
     * - Converting mouse coordinates to world coordinates
     * - Setting up orthographic projections
     * - Viewport-independent UI positioning
     * - Cross-platform graphics coordinate normalization
     *
     * Integration with Graphics Pipeline:
     * Window Coordinates → World Coordinates → NDC → GPU Rendering
     */
    inline Math::TransformationMatrix build_ndc_matrix(Math::ivec2 view_size) noexcept
    {

        double width = static_cast<double>(2.0f) / static_cast<double>(view_size.x);
        double height = static_cast<double>(2.0f) / static_cast<double>(view_size.y);
        Math::ScaleMatrix S{{width,height}};
        Math::TranslationMatrix T(Math::vec2{ -1, -1 });

        return T * S;
    }
}
