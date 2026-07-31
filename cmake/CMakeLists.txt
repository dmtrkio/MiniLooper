include(${CMAKE_CURRENT_SOURCE_DIR}/options.cmake)

add_subdirectory(deps)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_BINARY_DIR}/generated/config.h
)