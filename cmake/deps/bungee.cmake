FetchContent_Declare(
    bungee
    GIT_REPOSITORY https://github.com/dmtrkio/bungee.git
    GIT_TAG main
    GIT_SUBMODULES_RECURSE TRUE
    EXCLUDE_FROM_ALL
)
set(BUNGEE_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(BUNGEE_INSTALL_FRAMEWORK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(bungee)

target_include_directories(bungee_library INTERFACE
    ${bungee_SOURCE_DIR}
)