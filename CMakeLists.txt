cmake_minimum_required(VERSION 3.16)

add_compile_options(
    -Wno-unused-variable
    -Wno-error=unused-variable
    -Wno-error=implicit-function-declaration
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(page)
