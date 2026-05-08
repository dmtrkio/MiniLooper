include(FetchContent)

include(${CMAKE_CURRENT_SOURCE_DIR}/readerwriterqueue.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/portaudio.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/portmidi.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/sdl3.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/dr_wav.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/imgui.cmake)

add_library(dependencies INTERFACE)
add_library(MiniLooper::dependencies ALIAS dependencies)

target_link_libraries(dependencies INTERFACE
    readerwriterqueue
    portaudio
    portmidi_with_porttime
    SDL3::SDL3-static
    nlohmann_json::nlohmann_json
    dr_wav
    imgui_lib
)