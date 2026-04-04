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

# portaudio - audio io library
FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG master
)
set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(portaudio)

FetchContent_Declare(
    portmidi
    GIT_REPOSITORY https://github.com/PortMidi/portmidi.git
    GIT_TAG master
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(portmidi)

# SDL3 - cross-platform windowing, inputs and graphics
find_package(SDL3 QUIET)
if (NOT SDL3_FOUND)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.2
    )
    FetchContent_MakeAvailable(SDL3)
endif()

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    dr_wav
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG master
)
set(DR_LIBS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(dr_wav)

# Setting up DearImGui, which is not a cmake project
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

set(imgui_INCLUDES
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

set(imgui_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# Faust Generated DSP
add_subdirectory(faust)

# Source files
set(SOURCE_FILES
    src/audio/audio_engine.cpp
    src/audio/wav_writer.cpp
    src/main_application.cpp
    src/main_callbacks.cpp
    src/looper/looper_processor.cpp
    src/looper/looper_commands.cpp
    src/looper/looper.cpp
    src/ui/parameter_ui.cpp
    ${imgui_SOURCES}
)

# Build Executable
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${portmidi_SOURCE_DIR}/porttime
    ${imgui_INCLUDES}
    ${dr_wav_SOURCE_DIR}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${SDL3_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    readerwriterqueue
    portaudio
    portmidi
    SDL3::SDL3-static
    nlohmann_json::nlohmann_json
)

target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_USE_CALLBACKS)

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/RTC1 /W3>>
    $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/O2 /W3>>
    $<$<CONFIG:Debug>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-g -Wall -Wextra>>
    $<$<CONFIG:Release>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O2 -Wall -Wextra>>
)

# Install rules
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)