cmake_minimum_required(VERSION 3.22)

# Project Settings
project(MiniLooper VERSION 0.0.1)
set(APP_ORGANIZATION "MiniLooper")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies.cmake)

# Faust Generated DSP
add_subdirectory(faust)

# Source files
set(SOURCE_FILES
    src/audio/audio_engine.cpp
    src/audio/wav_writer.cpp
    src/main_application.cpp
    src/main_callbacks.cpp
    src/session_manager.cpp
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

if (WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
else()
    add_executable(${PROJECT_NAME} ${SOURCE_FILES})
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/config.h
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
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