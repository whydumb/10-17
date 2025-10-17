# author Rudy Castan
# date 2025 Fall
# CS200 Computer Graphics I
# copyright DigiPen Institute of Technology

FetchContent_Declare(
    gsl
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    URL https://github.com/microsoft/GSL/archive/refs/tags/v4.2.0.tar.gz
    URL_HASH MD5=5f3d2f7ef9c6351503c0c8f1f9390144
)
FetchContent_MakeAvailable(gsl)

add_library(the_gsl INTERFACE)
target_include_directories(the_gsl SYSTEM INTERFACE ${gsl_SOURCE_DIR}/include)
