/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <cmath>

namespace util
{
    class FPS
    {
        double timeCounter = 0;
        double numFrames   = 0;
        double fps         = 0;

    public:
        constexpr void Update(double delta_time_seconds) noexcept
        {
            ++numFrames;
            timeCounter += delta_time_seconds;
            if (timeCounter > 1.0)
            {
                fps = numFrames / timeCounter;
                timeCounter -= 1.0;
                numFrames = 0;
            }
        }

        operator long long() const noexcept
        {
            return std::llround(fps);
        }

        operator int() const noexcept
        {
            return static_cast<int>(std::lround(fps));
        }
    };
}
