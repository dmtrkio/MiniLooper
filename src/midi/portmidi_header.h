#pragma once

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4200)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include <portmidi.h>

#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif