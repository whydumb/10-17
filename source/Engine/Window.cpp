/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \author Junyoung Ki
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#include "Window.hpp"
#include "CS200/RenderingAPI.hpp"
#include "Engine.hpp"
#include "Error.hpp"
#include "Logger.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <functional>
#include <sstream>

namespace
{
    void hint_gl(SDL_GLattr attr, int value)
    {
        // // https://wiki.libsdl.org/SDL2/SDL_GL_SetAttribute
        if (const auto success = SDL_GL_SetAttribute(attr, value); success != 0)    
        {
            Engine::GetLogger().LogError(std::string{ "Failed to Set GL Attribute: " } + SDL_GetError());
        }
    }
}

namespace CS230
{
    
    void Window::Start(std::string_view title)
    {
        setupSDLWindow(title);
        setupOpenGL();
        
        // Get actual drawable size for high-DPI displays
        SDL_GL_GetDrawableSize(sdlWindow, &size.x, &size.y);
        
        // Set initial clear color through our rendering abstraction
        CS200::RenderingAPI::SetClearColor(default_background);
    }

    // Window.cpp

    void Window::Update()
    {
        // Swap front and back buffers to present the completed frame
        SDL_GL_SwapWindow(sdlWindow);
        
        // Process SDL events (input, window events, etc.)
        SDL_Event event{0};
        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT) {
                is_closed = true;
                return;
            }

            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    is_closed = true;
                    return;
                }
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || 
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_GL_GetDrawableSize(sdlWindow, &size.x, &size.y);
                }
            }
            if (eventCallback)  {
                eventCallback(event);
            }
            // Handle window-specific events like SDL_WINDOWEVENT_CLOSE,
            //  SDL_WINDOWEVENT_RESIZED and SDL_WINDOWEVENT_SIZE_CHANGED
            // ...
        }
    }

    bool Window::IsClosed() const{
        return is_closed;
    }

    Math::ivec2 Window::GetSize() const{
        return size;
    }

    void Window::Clear(CS200::RGBA color){
        CS200::RenderingAPI::SetClearColor(color);
        CS200::RenderingAPI::Clear();
    }

    void Window::ForceResize(int desired_width, int desired_height){
        SDL_SetWindowSize(sdlWindow, desired_width, desired_height); //change in window size
        size = { desired_width, desired_height }; //change in size
    }

    SDL_Window* Window::GetSDLWindow() const{
        return sdlWindow;
    }

    SDL_GLContext Window::GetGLContext() const{
        return glContext;
    }

    void Window::SetEventCallback(WindowEventCallback callback){
        eventCallback = callback;
    }

    Window::~Window()
    {
        if (glContext) {
            SDL_GL_DeleteContext(glContext);
        }
        if (sdlWindow) {
            SDL_DestroyWindow(sdlWindow);
        }

        SDL_Quit();
    }

    void CS230::Window::setupSDLWindow(std::string_view title)
    {
        // Part 1 - Initialize SDL for visual use
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw_error_message("Failed to init SDK error: ", SDL_GetError());
        }

        // Part 2 - Configure OpenGL context attributes (before window creation)
    #if defined(IS_WEBGL2)
        hint_gl(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        hint_gl(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        hint_gl(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    #else
        hint_gl(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    #endif
        hint_gl(SDL_GL_DOUBLEBUFFER, true);
        hint_gl(SDL_GL_STENCIL_SIZE, 8);
        hint_gl(SDL_GL_DEPTH_SIZE, 24);
        hint_gl(SDL_GL_RED_SIZE, 8);
        hint_gl(SDL_GL_GREEN_SIZE, 8);
        hint_gl(SDL_GL_BLUE_SIZE, 8);
        hint_gl(SDL_GL_ALPHA_SIZE, 8);
        hint_gl(SDL_GL_MULTISAMPLEBUFFERS, 1);
        hint_gl(SDL_GL_MULTISAMPLESAMPLES, 4);

        // Part 3 - Create the SDL window
        sdlWindow = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                    default_width, default_height, 
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (sdlWindow == nullptr)
        {
            throw_error_message("Failed to create window: ", SDL_GetError());
        }
    }
    void CS230::Window::setupOpenGL()
    {
        // Create OpenGL context
        if (glContext = SDL_GL_CreateContext(sdlWindow); glContext == nullptr)
        {
            throw_error_message("Failed to create opengl context: ", SDL_GetError());
        }

        // Make context current
        SDL_GL_MakeCurrent(sdlWindow, glContext);

        // Initialize GLEW for extension loading
        if (const auto result = glewInit(); GLEW_OK != result)
        {
            throw_error_message("Unable to initialize GLEW - error: ", glewGetErrorString(result));
        }

        // Configure VSync
        constexpr int ADAPTIVE_VSYNC = -1;
        constexpr int VSYNC = 1;
        if (const auto result = SDL_GL_SetSwapInterval(ADAPTIVE_VSYNC); result != 0)
        {
            SDL_GL_SetSwapInterval(VSYNC);
        }

        // Initialize our rendering abstraction layer
        CS200::RenderingAPI::Init();
    }
}
