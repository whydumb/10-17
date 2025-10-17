# author Rudy Castan
# date 2025 Fall
# CS200 Computer Graphics I
# copyright DigiPen Institute of Technology

# Depends on SDL2 and OpenGL

FetchContent_Declare(
    dearimgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG a28cb615ff2c2ed8901397b00762d557cae24760 # latest from docking branch as of 8/22/2025
)
FetchContent_MakeAvailable(dearimgui)

add_library(the_imgui STATIC
    ${dearimgui_SOURCE_DIR}/imgui.cpp ${dearimgui_SOURCE_DIR}/imgui.h
    ${dearimgui_SOURCE_DIR}/imconfig.h ${dearimgui_SOURCE_DIR}/imgui_internal.h
    ${dearimgui_SOURCE_DIR}/imgui_draw.cpp
    ${dearimgui_SOURCE_DIR}/imgui_tables.cpp
    ${dearimgui_SOURCE_DIR}/imgui_widgets.cpp
    ${dearimgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp ${dearimgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    ${dearimgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${dearimgui_SOURCE_DIR}/backends/imgui_impl_sdl2.h
)

target_include_directories(the_imgui SYSTEM PUBLIC ${dearimgui_SOURCE_DIR} ${dearimgui_SOURCE_DIR}/backends)
target_link_libraries(the_imgui PRIVATE the_sdl2)
