cmake_minimum_required(VERSION 3.22)

# Project Settings
project(MiniLooper VERSION 0.0.1)

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
if(WIN32)
    set(RTAUDIO_API_ASIO ON CACHE INTERNAL "")
else()
    set(RTAUDIO_API_JACK OFF CACHE INTERNAL "")
endif()
set(RTAUDIO_BUILD_TESTING OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(rtaudio)

# portaudio - audio io library
FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG master
)
set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(portaudio)

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

# Raylib - windowing/inputs/graphics library
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
set(SUPPORT_MODULE_RMODELS OFF CACHE INTERNAL "")
set(SUPPORT_MODULE_RAUDIO OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(raylib)

# Faust Generated DSP
add_subdirectory(faust)

# Source files
set(SOURCE_FILES
    src/audio/audio_engine.cpp
    src/main.cpp
    src/looper/looper.cpp
    src/looper/looper_commands.cpp
    src/audio/portaudio_backend.cpp
src/audio/rtaudio_backend.cpp
)

# Build Executable
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    portaudio
    rtaudio
    libremidi
    readerwriterqueue
    kissfft 
    faust_dsp_lib
    raylib
)

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/RTC1 /W3>>
    $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/O2 /W3>>
    $<$<CONFIG:Debug>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-g -Wall -Wextra>>
    $<$<CONFIG:Release>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O2 -Wall -Wextra>>
)

# Install rules
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
