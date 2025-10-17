/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Input.hpp"
#include "Engine.hpp"
#include "Logger.hpp"
#include <SDL.h>

namespace CS230
{

    Input::Input() {
        Init();
    }
    
    void Input::Init()
    {
        previousKeys.fill(false);
        currentKeys.fill(false);
    }

        // via SDL get keyboard state
        // mark each keyboard that is down
    void CS230::Input::Update() {
        previousKeys = currentKeys;
        const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
            for (Keys key = Keys::A; key < Keys::Count; key = static_cast<Keys>(static_cast<int>(key) + 1)) {
                auto sdl_key = convert_cs230_to_sdl(static_cast<Keys>(key));
                bool is_pressed = keyboard[sdl_key];

                std::size_t index = static_cast<std::size_t>(key);
                currentKeys[index] = is_pressed;
            if (KeyJustPressed(key)) {
                Engine::GetLogger().LogDebug("Key Pressed");
            }
            else if (KeyJustReleased(key)) {
                Engine::GetLogger().LogDebug("Key Released");
            }
    } 
}

    void Input::SetKeyDown(Keys key, bool is_pressed)
    {
        currentKeys[static_cast<std::size_t>(key)] = is_pressed;
    }

    bool Input::KeyDown(Keys key) const
    {
        return currentKeys[static_cast<std::size_t>(key)];
    }

    bool Input::KeyJustPressed(Keys key) const
    {
        const std::size_t index = static_cast<std::size_t>(key);
        return currentKeys[index] && !previousKeys[index];
    }

    bool Input::KeyJustReleased(Keys key) const
    {
        const std::size_t index = static_cast<std::size_t>(key);
        return !currentKeys[index] && previousKeys[index];
    }

    
}
