# author Rudy Castan
# date 2025 Fall
# CS200 Computer Graphics I
# copyright DigiPen Institute of Technology
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

# #  Helper to print out all environment variables
# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

project(cs200_HW1)

include(cmake/StandardProjectSettings.cmake)

include(cmake/CompilerWarnings.cmake)
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)
set_project_warnings(project_options)

include(cmake/Dependencies.cmake)

add_subdirectory(source)

