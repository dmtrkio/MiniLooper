cmake_minimum_required(VERSION 3.22)

# Project Settings
set(APP_NAME MiniLooper)
project(${APP_NAME} VERSION 0.0.1)

# Dependencies
include(FetchContent)

# readerwriterqueue - lock-free single producer single consumer fifo
FetchContent_Declare(
    readerwriterqueue
    GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
    GIT_TAG master
)
FetchContent_MakeAvailable(readerwriterqueue)

# RtAudio - audio io library
FetchContent_Declare(
    rtaudio
    GIT_REPOSITORY https://github.com/thestk/rtaudio.git
    GIT_TAG master
)
set(RTAUDIO_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(RTAUDIO_API_JACK OFF CACHE INTERNAL "")
set(RTAUDIO_BUILD_TESTING OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(rtaudio)

# libremidi - midi library
FetchContent_Declare(
    libremidi
    GIT_REPOSITORY https://github.com/celtera/libremidi.git
    GIT_TAG master
)
set(LIBREMIDI_NO_JACK OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(libremidi)

# kissfft - FFT(Fast Fourier Transform) library
FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG master
)
set(KISSFFT_STATIC ON CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(KISSFFT_PKGCONFIG OFF CACHE INTERNAL "")
set(KISSFFT_TEST OFF CACHE INTERNAL "")
set(KISSFFT_TOOLS OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(kissfft)

# Faust Generated DSP
add_subdirectory(faust)

# Build command
set(CMAKE_CXX_STANDARD 23)
add_compile_options(-O2 -Wall -Wextra -fdiagnostics-color=always)

# Source files
set(SOURCE_FILES
    src/audio_engine.cpp
    src/main.cpp
    src/looper/looper.cpp
    src/looper/looper_commands.cpp
)

# Executable
add_executable(${APP_NAME} ${SOURCE_FILES})

# Include directories
target_include_directories(${APP_NAME} PRIVATE
    ${rtaudio_SOURCE_DIR}/include
    ${libremidi_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${APP_NAME} PRIVATE
    rtaudio
    libremidi
    readerwriterqueue
    kissfft 
    faust_dsp_lib
)

# Install rules
install(TARGETS ${APP_NAME} RUNTIME DESTINATION bin)
