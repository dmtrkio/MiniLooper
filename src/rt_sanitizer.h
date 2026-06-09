#pragma once

#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    #define RT_SAN [[clang::nonblocking]]
#else
    #define RT_SAN
#endif