cmake_minimum_required(VERSION 3.22)

# Project Settings
project(MiniLooper VERSION 0.0.1)
set(APP_ORGANIZATION "MiniLooper")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(cmake)
add_subdirectory(src)