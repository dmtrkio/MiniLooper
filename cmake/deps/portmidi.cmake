FetchContent_Declare(
    portmidi
    GIT_REPOSITORY https://github.com/PortMidi/portmidi.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(portmidi)

add_library(portmidi_with_porttime INTERFACE)
target_link_libraries(portmidi_with_porttime INTERFACE portmidi)
target_include_directories(portmidi_with_porttime INTERFACE 
    $<BUILD_INTERFACE:${portmidi_SOURCE_DIR}/porttime>
)