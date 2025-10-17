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
#include <gsl/gsl>
#include <vector>
#include <SDL.h> 


namespace CS230
{
    class Input
    {
    public:
        enum class Keys
        {
            A,
            B,
            C,
            D,
            E,
            F,
            G,
            H,
            I,
            J,
            K,
            L,
            M,
            N,
            O,
            P,
            Q,
            R,
            S,
            T,
            U,
            V,
            W,
            X,
            Y,
            Z,
            Space,
            Enter,
            Left,
            Up,
            Right,
            Down,
            Escape,
            Tab,
            Count
        };

        Input();
        void Update();

        bool KeyDown(Keys key) const;
        bool KeyJustReleased(Keys key) const;
        bool KeyJustPressed(Keys key) const;
        void Init();

    private:
        std::array<bool, static_cast<size_t>(Keys::Count)> previousKeys;
        std::array<bool, static_cast<size_t>(Keys::Count)> currentKeys;
    private:
        void SetKeyDown(Keys key, bool value);
    };

    constexpr Input::Keys& operator++(Input::Keys& the_key) noexcept
    {
        the_key = static_cast<Input::Keys>(static_cast<unsigned>(the_key) + 1);
        return the_key;
    }

    constexpr gsl::czstring to_string(Input::Keys key) noexcept
    {
        switch (key)
        {
            case Input::Keys::A: return "A";
            case Input::Keys::B: return "B";
            case Input::Keys::C: return "C";
            case Input::Keys::D: return "D";
            case Input::Keys::E: return "E";
            case Input::Keys::F: return "F";
            case Input::Keys::G: return "G";
            case Input::Keys::H: return "H";
            case Input::Keys::I: return "I";
            case Input::Keys::J: return "J";
            case Input::Keys::K: return "K";
            case Input::Keys::L: return "L";
            case Input::Keys::M: return "M";
            case Input::Keys::N: return "N";
            case Input::Keys::O: return "O";
            case Input::Keys::P: return "P";
            case Input::Keys::Q: return "Q";
            case Input::Keys::R: return "R";
            case Input::Keys::S: return "S";
            case Input::Keys::T: return "T";
            case Input::Keys::U: return "U";
            case Input::Keys::V: return "V";
            case Input::Keys::W: return "W";
            case Input::Keys::X: return "X";
            case Input::Keys::Y: return "Y";
            case Input::Keys::Z: return "Z";
            case Input::Keys::Space: return "Space";
            case Input::Keys::Enter: return "Enter";
            case Input::Keys::Left: return "Left";
            case Input::Keys::Up: return "Up";
            case Input::Keys::Right: return "Right";
            case Input::Keys::Down: return "Down";
            case Input::Keys::Escape: return "Escape";
            case Input::Keys::Tab: return "Tab";
            case Input::Keys::Count: return "Count";
        }
        return "Unknown";
    }

    inline SDL_Scancode convert_cs230_to_sdl(Input::Keys cs230_key)
    {
        switch (cs230_key)
        {
            case Input::Keys::A: return SDL_SCANCODE_A;
            case Input::Keys::B: return SDL_SCANCODE_B;
            case Input::Keys::C: return SDL_SCANCODE_C;
            case Input::Keys::D: return SDL_SCANCODE_D;
            case Input::Keys::E: return SDL_SCANCODE_E;
            case Input::Keys::F: return SDL_SCANCODE_F;
            case Input::Keys::G: return SDL_SCANCODE_G;
            case Input::Keys::H: return SDL_SCANCODE_H;
            case Input::Keys::I: return SDL_SCANCODE_I;
            case Input::Keys::J: return SDL_SCANCODE_J;
            case Input::Keys::K: return SDL_SCANCODE_K;
            case Input::Keys::L: return SDL_SCANCODE_L;
            case Input::Keys::M: return SDL_SCANCODE_M;
            case Input::Keys::N: return SDL_SCANCODE_N;
            case Input::Keys::O: return SDL_SCANCODE_O;
            case Input::Keys::P: return SDL_SCANCODE_P;
            case Input::Keys::Q: return SDL_SCANCODE_Q;
            case Input::Keys::R: return SDL_SCANCODE_R;
            case Input::Keys::S: return SDL_SCANCODE_S;
            case Input::Keys::T: return SDL_SCANCODE_T;
            case Input::Keys::U: return SDL_SCANCODE_U;
            case Input::Keys::V: return SDL_SCANCODE_V;
            case Input::Keys::W: return SDL_SCANCODE_W;
            case Input::Keys::X: return SDL_SCANCODE_X;
            case Input::Keys::Y: return SDL_SCANCODE_Y;
            case Input::Keys::Z: return SDL_SCANCODE_Z;

            case Input::Keys::Space: return SDL_SCANCODE_SPACE;
            case Input::Keys::Enter: return SDL_SCANCODE_RETURN;
            case Input::Keys::Left: return SDL_SCANCODE_LEFT;
            case Input::Keys::Up: return SDL_SCANCODE_UP;
            case Input::Keys::Right: return SDL_SCANCODE_RIGHT;
            case Input::Keys::Down: return SDL_SCANCODE_DOWN;
            case Input::Keys::Escape: return SDL_SCANCODE_ESCAPE;
            case Input::Keys::Tab: return SDL_SCANCODE_TAB;

            default: return SDL_SCANCODE_UNKNOWN;
        }
    }
}
