FetchContent_Declare(
    dr_wav
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
set(DR_LIBS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(dr_wav)

add_library(dr_wav INTERFACE)
target_include_directories(dr_wav INTERFACE 
    $<BUILD_INTERFACE:${dr_wav_SOURCE_DIR}>
)