include(FetchContent)

include(${CMAKE_CURRENT_SOURCE_DIR}/readerwriterqueue.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/portaudio.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/portmidi.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/sdl3.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/imgui.cmake)

add_library(dependencies INTERFACE)
add_library(${PROJECT_NAME}::dependencies ALIAS dependencies)

target_link_libraries(dependencies INTERFACE
    readerwriterqueue
    portaudio
    portmidi_with_porttime
    ${SDL3_TARGET}
    nlohmann_json::nlohmann_json
    imgui_lib
)