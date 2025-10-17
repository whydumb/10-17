# author Rudy Castan
# date 2025 Fall
# CS200 Computer Graphics I
# copyright DigiPen Institute of Technology

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCE_CODE 

    CS200/Image.hpp CS200/Image.cpp
    CS200/ImGuiHelper.hpp CS200/ImGuiHelper.cpp
    CS200/ImmediateRenderer2D.hpp CS200/ImmediateRenderer2D.cpp
    CS200/IRenderer2D.hpp
    CS200/NDC.hpp
    CS200/Renderer2DUtils.hpp CS200/Renderer2DUtils.cpp
    CS200/RenderingAPI.hpp CS200/RenderingAPI.cpp
    CS200/RGBA.hpp

    Demo/DemoTexturing.hpp Demo/DemoTexturing.cpp
    Demo/DemoCS230Textures.hpp Demo/DemoCS230Textures.cpp

    Engine/Engine.hpp Engine/Engine.cpp
    Engine/Error.hpp
    Engine/FPS.hpp
    Engine/GameState.hpp
    Engine/GameStateManager.hpp Engine/GameStateManager.cpp
    Engine/Input.hpp Engine/Input.cpp
    Engine/Logger.hpp Engine/Logger.cpp
    Engine/Matrix.hpp Engine/Matrix.cpp
    Engine/Path.hpp Engine/Path.cpp
    Engine/Random.hpp Engine/Random.cpp
    Engine/Texture.hpp Engine/Texture.cpp
    Engine/TextureManager.hpp Engine/TextureManager.cpp
    Engine/Timer.hpp
    Engine/Vec2.hpp Engine/Vec2.cpp
    Engine/Window.hpp Engine/Window.cpp

    OpenGL/Buffer.hpp OpenGL/Buffer.cpp
    OpenGL/Environment.hpp
    OpenGL/GL.cpp OpenGL/GL.hpp
    OpenGL/GLConstants.hpp
    OpenGL/GLTypes.hpp
    OpenGL/Handle.hpp
    OpenGL/Shader.cpp OpenGL/Shader.hpp
    OpenGL/Texture.hpp OpenGL/Texture.cpp
    OpenGL/VertexArray.cpp OpenGL/VertexArray.hpp

    main.cpp
)

add_executable(cs200_fun ${SOURCE_CODE})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_CODE})

target_link_libraries(cs200_fun PRIVATE project_options dependencies)
target_include_directories(cs200_fun PRIVATE .)

# Check the IS_DEVELOPER_VERSION cache variable
# This is set by the cmake configure preset
if (IS_DEVELOPER_VERSION)
    target_compile_definitions(cs200_fun PRIVATE DEVELOPER_VERSION)
endif()

if(EMSCRIPTEN)

    # https://emscripten.org/docs/tools_reference/settings_reference.html
    # ASSERTIONS=1                  - we want asserts to work
    # WASM=1                        - we want web assembly generated rather than just javascript
    # DISABLE_EXCEPTION_CATCHING=0  - we are using exceptions so let's not disable them
    # ALLOW_MEMORY_GROWTH=1         - we don't know exactly how much heap space we'll need so allow it to grow
    # EXIT_RUNTIME=1                - have exiting actually stop the program
    # SINGLE_FILE=1                 - generate everything into one html file
    # --embed-file                  - https://emscripten.org/docs/tools_reference/emcc.html#emcc-embed-file
    # --use-preload-cache           - help with faster reloads : https://emscripten.org/docs/compiling/Deploying-Pages.html#providing-a-quick-second-time-load
    # -lembind                      - to call c++ from javascript https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
    # --shell-file                  - to customize the webpage https://emscripten.org/docs/compiling/Deploying-Pages.html#build-files-and-custom-shell
    target_link_options(cs200_fun PRIVATE 
    -sASSERTIONS=$<$<BOOL:${IS_DEVELOPER_VERSION}>:1>$<$<NOT:$<BOOL:${IS_DEVELOPER_VERSION}>>:0>
    -sWASM=1 
    -sDISABLE_EXCEPTION_CATCHING=0 
    -sALLOW_MEMORY_GROWTH=1 
    -sEXIT_RUNTIME=1 
    -sSINGLE_FILE=1 
    --embed-file ${CMAKE_SOURCE_DIR}/Assets@/Assets
    --use-preload-cache
    -lembind
    --shell-file ${CMAKE_SOURCE_DIR}/app_resources/web/index_shell.html
    )

    set_target_properties(cs200_fun PROPERTIES SUFFIX ".html")
elseif(WIN32)

    target_link_options(cs200_fun PRIVATE 
        $<IF:$<BOOL:${IS_DEVELOPER_VERSION}>,/SUBSYSTEM:CONSOLE,/SUBSYSTEM:WINDOWS>
        /ENTRY:mainCRTStartup)

    set(ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
    if(NOT EXISTS ${ICON_RC})
        set(ICON_FILE ${CMAKE_SOURCE_DIR}/app_resources/windows/icon.ico)
        file(WRITE ${ICON_RC} "IDR_MAINFRAME ICON \"${ICON_FILE}\"\nIDI_ICON1 ICON DISCARDABLE \"${ICON_FILE}\"")
    endif()

    target_sources(cs200_fun PRIVATE ${ICON_RC})

endif()
