# author Rudy Castan
# date 2025 Fall
# CS200 Computer Graphics I
# copyright DigiPen Institute of Technology

FetchContent_Declare(
    stb_github
    GIT_REPOSITORY https://github.com/nothings/stb.git
    # latest of master branch as of 8/22/2025
    GIT_TAG f58f558c120e9b32c217290b80bad1a0729fbb2c
)

FetchContent_MakeAvailable(stb_github)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/stb_implementation.cpp)
    set(STB_IMPLEMENTATION_CODE "// This file is auto-generated from cmake/depenendencies/STB.cmake
    #define STB_IMAGE_IMPLEMENTATION
    #include \"stb_image.h\"
    #define STB_PERLIN_IMPLEMENTATION
    #include \"stb_perlin.h\"
    //#include \"stb_vorbis.c\"
    ")

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/stb_implementation.cpp ${STB_IMPLEMENTATION_CODE})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/stb_implementation.cpp PROPERTIES GENERATED TRUE)
endif()

add_library(the_stb STATIC ${CMAKE_CURRENT_BINARY_DIR}/stb_implementation.cpp)
target_include_directories(the_stb SYSTEM PUBLIC ${stb_github_SOURCE_DIR})

