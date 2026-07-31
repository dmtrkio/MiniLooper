option(MINILOOPER_BUILD_TESTS "Build unit tests" OFF)

if (NOT WIN32)
    option(MINILOOPER_USE_SHARED_PORTAUDIO "Use shared portaudio" ON)
    option(MINILOOPER_USE_SHARED_SDL3 "Use shared SDL3" ON)
else()
    option(MINILOOPER_USE_SHARED_PORTAUDIO "Use shared portaudio" OFF)
    option(MINILOOPER_USE_SHARED_SDL3 "Use shared SDL3" OFF)
endif()