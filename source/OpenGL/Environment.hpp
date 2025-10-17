/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

namespace OpenGL
{
    inline int MajorVersion         = 0;
    inline int MinorVersion         = 0;
    inline int MaxTextureImageUnits = 2;
    inline int MaxTextureSize       = 64;

    constexpr int version(int major, int minor) noexcept
    {
        return major * 100 + minor * 10;
    }

    inline int current_version() noexcept
    {
        return version(MajorVersion, MinorVersion);
    }

#if defined(IS_WEBGL2)
    constexpr bool IsWebGL                     = true;
    // WEBGL2 is expressed as 3.0 ES
    constexpr int  MinimumRequiredMajorVersion = 3;
    constexpr int  MinimumRequiredMinorVersion = 0;
#else
    constexpr bool IsWebGL                     = false;
    constexpr int  MinimumRequiredMajorVersion = 3;
    constexpr int  MinimumRequiredMinorVersion = 3;
#endif
}
