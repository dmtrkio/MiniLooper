cmake_minimum_required(VERSION 3.30)

# Project Settings
project(MiniLooper VERSION 0.0.1)
set(APP_ORGANIZATION "MiniLooper")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(cmake)
add_subdirectory(thirdparty)
add_subdirectory(src)

if (WIN32)
    add_executable(${PROJECT_NAME} WIN32 src/main_callbacks.cpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
    )
else()
    add_executable(${PROJECT_NAME} src/main_callbacks.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

include(CTest)
enable_testing()
add_subdirectory(tests)