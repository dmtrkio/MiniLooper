find_package(portaudio QUIET)
if (portaudio_NOT_FOUND)
    message(STATUS "PortAudio not found on system, fetching and building statically")

    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG master
        EXCLUDE_FROM_ALL
    )

    set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(portaudio)
else()
    message(STATUS "Preinstalled PortAudio found (shared)")
endif()