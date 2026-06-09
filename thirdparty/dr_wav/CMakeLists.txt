add_library(dr_wav INTERFACE)
target_include_directories(dr_wav INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)