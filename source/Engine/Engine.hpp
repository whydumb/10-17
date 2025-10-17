/**
 * \file
 * \author Rudy Castan
 * \author Jonathan Holmes
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "Vec2.hpp"
#include <gsl/gsl>
#include <memory>
#include <string_view>

namespace CS230
{
    class Logger;
    class Window;
    class Input;
    class GameState;
    class GameStateManager;
    class TextureManager;
}

namespace CS200
{
    class IRenderer2D;
}

/**
 * \brief Runtime information about the window and application state
 *
 * WindowEnvironment provides access to essential runtime metrics and window
 * properties that are commonly needed by game logic, rendering systems, and
 * debugging tools. This information is updated each frame to reflect the
 * current state of the application.
 *
 * Timing Information:
 * - DeltaTime: Time elapsed since the last frame (for frame-rate independent movement)
 * - ElapsedTime: Total time since application started (for animations and effects)
 * - FrameCount: Total number of frames rendered (for debugging and profiling)
 * - FPS: Current frames per second (for performance monitoring)
 *
 * Display Information:
 * - DisplaySize: Current viewport dimensions in pixels (for coordinate calculations)
 *
 * Common use cases:
 * - Frame-rate independent animations and movement
 * - Performance monitoring and debugging
 * - UI scaling based on viewport size
 * - Time-based effects and animations
 */
struct WindowEnvironment
{
    int        FPS         = 0;   ///< Current frames per second
    uint64_t   FrameCount  = 0;   ///< Total frames rendered since start
    double     DeltaTime   = 0.0; ///< Time in seconds since last frame
    double     ElapsedTime = 0.0; ///< Total time in seconds since application start
    Math::vec2 DisplaySize{};     ///< Current viewport size in pixels
};

/**
 * \brief Central application engine managing all core systems and subsystems
 *
 * Engine serves as the main application framework that coordinates all major
 * subsystems including windowing, input handling, rendering, game state management,
 * and resource management. It implements the singleton pattern to provide global
 * access to these systems while maintaining proper initialization order and
 * cleanup procedures.
 *
 * Architecture and Design:
 * - Singleton pattern ensures single engine instance across the application
 * - Pimpl idiom hides implementation details and reduces compilation dependencies
 * - Static accessor methods provide convenient access to subsystems
 * - Manages complete application lifecycle from startup to shutdown
 *
 * Core Subsystems:
 * - Window: SDL-based window management and OpenGL context creation
 * - Input: Keyboard, mouse, and controller input handling
 * - Renderer2D: High-level 2D graphics rendering system
 * - GameStateManager: State machine for different application screens/modes
 * - TextureManager: Resource management for texture assets
 * - Logger: Debug and event logging system
 *
 * Application Lifecycle:
 * 1. Start(): Initialize all subsystems and create window
 * 2. Update(): Process one frame of the main loop (input, logic, rendering)
 * 3. HasGameEnded(): Check if application should terminate
 * 4. Stop(): Clean shutdown of all subsystems
 *
 * The engine handles the complex initialization order required by modern
 * graphics applications, including OpenGL context creation, ImGui setup,
 * and proper resource management throughout the application lifecycle.
 */

class Engine
{
public:
    /**
     * \brief Get the singleton Engine instance
     * \return Reference to the single Engine instance
     *
     * Provides access to the singleton Engine instance, creating it on first
     * access using lazy initialization. This ensures the engine is available
     * throughout the application lifetime while maintaining single-instance
     * semantics.
     *
     * The singleton is implemented with a static local variable, providing
     * thread-safe initialization in C++11 and later, and automatic cleanup
     * when the application terminates.
     */
    static Engine& Instance();

    /**
     * \brief Access the application logging system
     * \return Reference to the Logger instance for debug and event logging
     *
     * Provides access to the centralized logging system for debug output,
     * error reporting, and event tracking. The logger configuration differs
     * between development and release builds, with more verbose output
     * available in development versions.
     *
     * Common uses:
     * - Debug information and variable values
     * - Error reporting and exception details
     * - Performance metrics and timing information
     * - Application lifecycle events
     */
    static CS230::Logger& GetLogger();

    /**
     * \brief Access the window management system
     * \return Reference to the Window instance for display and context management
     *
     * Provides access to the window system that manages the application window,
     * OpenGL context, and display properties. This includes window creation,
     * resizing, fullscreen control, and event handling integration.
     *
     * Window capabilities:
     * - Window creation and destruction
     * - OpenGL context management
     * - Display mode and resolution control
     * - Event system integration
     * - Cross-platform window handling
     */
    static CS230::Window& GetWindow();

    /**
     * \brief Access the input handling system
     * \return Reference to the Input instance for keyboard, mouse, and controller input
     *
     * Provides access to the unified input system that handles all user input
     * devices including keyboard, mouse, and game controllers. The system
     * provides both immediate state queries and event-based input handling.
     *
     * Input capabilities:
     * - Keyboard state and key press/release events
     * - Mouse position, button states, and scroll wheel
     * - Game controller support with analog and digital inputs
     * - Input mapping and configuration
     * - Frame-coherent input state management
     */
    static CS230::Input& GetInput();

    /**
     * \brief Access current window and timing environment information
     * \return Const reference to WindowEnvironment with current frame data
     *
     * Provides read-only access to essential runtime information including
     * timing data, frame statistics, and display properties. This information
     * is updated each frame and is commonly used for frame-rate independent
     * calculations and performance monitoring.
     *
     * Available information:
     * - Delta time for frame-rate independent movement
     * - Total elapsed time for animations and effects
     * - Current FPS for performance monitoring
     * - Frame count for debugging and profiling
     * - Current viewport size for coordinate calculations
     */
    static const WindowEnvironment& GetWindowEnvironment();

    /**
     * \brief Access the game state management system
     * \return Reference to GameStateManager for application state control
     *
     * Provides access to the state machine that manages different application
     * modes such as menus, gameplay, pause screens, and loading states. The
     * state manager handles transitions between states and ensures proper
     * initialization and cleanup of state-specific resources.
     *
     * State management features:
     * - State transitions with proper lifecycle management
     * - State stack for overlay states (pause menus, dialogs)
     * - Automatic resource management per state
     * - Event propagation to active states
     * - Clean separation of application logic by state
     */
    static CS230::GameStateManager& GetGameStateManager();

    /**
     * \brief Access the 2D rendering system
     * \return Reference to IRenderer2D for all 2D graphics operations
     *
     * Provides access to the high-level 2D rendering system that abstracts
     * OpenGL complexity and provides efficient rendering of 2D graphics
     * primitives. The renderer handles batching, state management, and
     * coordinate system transformations automatically.
     *
     * Rendering capabilities:
     * - Sprite and texture rendering with transformations
     * - Primitive shapes (rectangles, circles, lines)
     * - Text rendering with font support
     * - Efficient batch rendering for performance
     * - Camera and viewport transformations
     * - Blend modes and rendering effects
     */
    static CS200::IRenderer2D& GetRenderer2D();

    /**
     * \brief Access the texture resource management system
     * \return Reference to TextureManager for texture loading and caching
     *
     * Provides access to the texture management system that handles loading,
     * caching, and lifecycle management of texture resources. The manager
     * prevents duplicate loading of textures and ensures efficient GPU
     * memory usage.
     *
     * Texture management features:
     * - Automatic texture loading from various image formats
     * - Reference counting and automatic cleanup
     * - Texture caching to prevent duplicate resources
     * - Efficient GPU memory management
     * - Support for different texture formats and properties
     */
    static CS230::TextureManager& GetTextureManager();


public:
    /**
     * \brief Initialize and start the engine with all subsystems
     * \param window_title Title to display in the window title bar
     *
     * Performs complete engine initialization including window creation, OpenGL
     * context setup, subsystem initialization, and resource loading. This must
     * be called before any other engine operations and establishes the foundation
     * for all subsequent application functionality.
     *
     * Initialization sequence:
     * - Creates application window with OpenGL context
     * - Initializes OpenGL state and rendering API
     * - Sets up ImGui for debugging and development UI
     * - Initializes 2D renderer and graphics systems
     * - Configures input handling and event processing
     * - Starts timing systems for frame rate calculation
     *
     * The function handles the complex dependencies between subsystems and
     * ensures everything is properly configured before the application begins
     * its main execution loop.
     *
     * Error handling:
     * If initialization fails, the function may throw exceptions or log
     * critical errors. The application should handle these cases appropriately.
     */
    void Start(std::string_view window_title);

    /**
     * \brief Shutdown the engine and clean up all resources
     *
     * Performs orderly shutdown of all engine subsystems, ensuring proper
     * cleanup of resources and graceful termination. This should be called
     * when the application is ready to exit, typically after the main loop
     * has ended.
     *
     * Shutdown sequence:
     * - Cleans up 2D renderer and graphics resources
     * - Clears all game states and their resources
     * - Shuts down ImGui and development tools
     * - Releases OpenGL context and window resources
     * - Performs final logging and cleanup
     *
     * The shutdown process ensures that all GPU resources are properly
     * released and that the application can terminate cleanly without
     * resource leaks or system instability.
     */
    void Stop();

    /**
     * \brief Process one frame of the application main loop
     *
     * Executes a complete frame cycle including input processing, game logic
     * updates, rendering operations, and timing calculations. This function
     * should be called repeatedly in the main application loop until the
     * application is ready to terminate.
     *
     * Frame processing sequence:
     * - Updates timing information and frame statistics
     * - Processes window events and input state
     * - Updates the current game state logic
     * - Sets up rendering viewport and coordinate systems
     * - Renders the current game state with 2D graphics
     * - Handles ImGui rendering for development tools
     * - Prepares for the next frame
     *
     * The function handles the coordination between all subsystems and
     * ensures that each frame is processed consistently and efficiently.
     * It also manages the rendering pipeline and coordinate system
     * transformations automatically.
     */
    void Update();

    /**
     * \brief Check if the application should terminate
     * \return True if the application should exit, false to continue running
     *
     * Determines whether the application main loop should continue or terminate
     * based on window close events, user input, or game state conditions.
     * This is typically checked after each Update() call to decide whether
     * to continue the main loop.
     *
     * Termination conditions:
     * - User closes the window (clicking X button or Alt+F4)
     * - Game state manager signals application end
     * - Critical errors that require application termination
     *
     * The function provides a clean way to check for termination conditions
     * without requiring direct access to window or state management internals.
     */
    bool HasGameEnded();

private:
    // Forward declaration for Pimpl (Pointer to Implementation) idiom
    // This hides implementation details and reduces compilation dependencies
    class Impl;
    gsl::owner<Impl*> impl = nullptr;

private:
    // Singleton pattern: private constructor and destructor
    // prevent external instantiation and ensure single instance
    Engine();
    ~Engine();

    // Internal method for updating frame timing and window environment
    // Called each frame to maintain current runtime statistics
    void updateEnvironment();
};
