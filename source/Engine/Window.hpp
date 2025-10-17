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
#include "CS200/RGBA.hpp"
#include "Vec2.hpp"
#include <functional>
#include <gsl/gsl>
#include <string_view>

struct SDL_Window;
typedef void*           SDL_GLContext;
typedef union SDL_Event SDL_Event;

namespace CS230
{
    class Window
    {
    public:
        void          Start(std::string_view title);
        void          Update();
        bool          IsClosed() const;
        Math::ivec2   GetSize() const;
        void          Clear(CS200::RGBA color);
        void          ForceResize(int desired_width, int desired_height);
        SDL_Window*   GetSDLWindow() const;
        SDL_GLContext GetGLContext() const;

        using WindowEventCallback = std::function<void(const SDL_Event&)>;
        void SetEventCallback(WindowEventCallback callback);

        Window() = default;
        ~Window();

        Window(const Window&)                = delete;
        Window& operator=(const Window&)     = delete;
        Window(Window&&) noexcept            = delete;
        Window& operator=(Window&&) noexcept = delete;

    private:
        void setupSDLWindow(std::string_view title);
        void setupOpenGL();

        // Platform-specific members
        gsl::owner<SDL_Window*>         sdlWindow = nullptr;
        gsl::owner<SDL_GLContext>       glContext = nullptr;
        Math::ivec2                     size{800, 600};
        WindowEventCallback             eventCallback;
        bool                            is_closed = false;
        
        // Constants
        static constexpr int default_width = 800;
        static constexpr int default_height = 600;
        static constexpr CS200::RGBA default_background{CS200::WHITE};
    };
}
