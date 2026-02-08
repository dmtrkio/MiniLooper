cmake_minimum_required(VERSION 3.22)

add_library(faust_dsp_lib INTERFACE)

target_include_directories(faust_dsp_lib
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)
