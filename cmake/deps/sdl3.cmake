find_package(SDL3 QUIET)
if (NOT SDL3_FOUND)
    message(STATUS "SDL3 not found on system, fetching and building statically")

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.2
        EXCLUDE_FROM_ALL
    )

    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable SDL3 tests" FORCE)
    set(SDL_AUDIO OFF CACHE BOOL "Disable SDL3 audio" FORCE)
    set(SDL_HAPTIC OFF CACHE BOOL "Disable SDL3 haptic" FORCE)
    set(SDL_POWER OFF CACHE BOOL "Disable SDL3 power" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "Disable SDL3 sensor" FORCE)
    set(SDL_CAMERA OFF CACHE BOOL "Disable SDL3 camera" FORCE)
    set(SDL_GPU OFF CACHE BOOL "Disable SDL3 GPU subsystem" FORCE)
    set(SDL_TRAY OFF CACHE BOOL "Disable SDL3 tray" FORCE)
    set(SDL_NOTIFICATION OFF CACHE BOOL "Disable SDL3 notification" FORCE)
    set(SDL_PROCESS OFF CACHE BOOL "Disable SDL3 process" FORCE)
    set(SDL_STORAGE OFF CACHE BOOL "Disable SDL3 storage" FORCE)

    FetchContent_MakeAvailable(SDL3)
    set(SDL3_TARGET SDL3::SDL3-static)
else()
    message(STATUS "Preinstalled SDL3 found (shared)")

    if(TARGET SDL3::SDL3-static)
        set(SDL3_TARGET SDL3::SDL3-static)
    else()
        set(SDL3_TARGET SDL3::SDL3)
    endif()
endif()