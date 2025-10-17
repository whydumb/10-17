/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <filesystem>
#include <gsl/gsl>

struct SDL_Window;
typedef void* SDL_GLContext;
union SDL_Event;

namespace ImGuiHelper
{
    void Initialize(gsl::not_null<SDL_Window*> sdl_window, gsl::not_null<SDL_GLContext> gl_context, const std::filesystem::path& ini_file_path = "imgui.ini");
    void FeedEvent(const SDL_Event& event);

    struct Viewport
    {
        int  x = 0, y = 0;
        int  width = 0, height = 0;
        bool operator==(const Viewport&) const = default;
    };

    Viewport Begin();
    void     End();
    void     Shutdown();

}
