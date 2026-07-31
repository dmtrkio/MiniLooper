if (MINILOOPER_USE_SHARED_PORTAUDIO)
    message(STATUS "Looking for PortAudio shared library...")

    find_package(portaudio QUIET)

    if (NOT portaudio_FOUND)
        find_package(PkgConfig QUIET)
        if (PkgConfig_FOUND)
            pkg_check_modules(PORTAUDIO portaudio-2.0)
            if (PORTAUDIO_FOUND)
                set(portaudio_FOUND TRUE)
            endif()
        endif()
    endif()

    if (NOT portaudio_FOUND)
        message(FATAL_ERROR 
            "No PortAudio shared library found, cannot proceed\n"
            "Install it with:\n"
            "  Ubuntu/Debian: sudo apt install portaudio19-dev\n"
            "  Fedora: sudo dnf install portaudio-devel\n"
            "  Arch: sudo pacman -S portaudio\n"
            "  Or set MINILOOPER_USE_SHARED_PORTAUDIO=OFF to build static"
        )
    endif()
else()
    message(STATUS "Fetching PortAudio and building statically")

    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG master
        EXCLUDE_FROM_ALL
    )

    set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(portaudio)
endif()