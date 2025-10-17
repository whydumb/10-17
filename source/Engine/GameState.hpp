/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <gsl/gsl>

namespace CS230
{
    class GameState
    {
    public:
        virtual void          Load()          = 0;
        virtual void          Update()        = 0;
        virtual void          Unload()        = 0;
        virtual void          Draw() const    = 0;
        virtual void          DrawImGui()     = 0;
        virtual gsl::czstring GetName() const = 0;
        virtual ~GameState()                  = default;
    };
}
