add_library(${PROJECT_NAME}_core OBJECT
    audio/audio_engine.cpp
    audio/wav_writer.cpp
    dsp/effects/pitch_shifter.cpp
    dsp/parameter/parameter_tree.cpp
    midi/midi.cpp
    looper/looper_processor.cpp
    looper/looper_commands.cpp
    looper/looper.cpp
    ui/parameter_ui.cpp
    main_application.cpp
    session_manager.cpp
)

target_include_directories(${PROJECT_NAME}_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/res>
)

target_link_libraries(${PROJECT_NAME}_core PUBLIC
    MiniLooper::dependencies
)

target_compile_options(${PROJECT_NAME}_core PRIVATE
    $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/RTC1 /W3>>
    $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/O2 /W3>>
    $<$<CONFIG:Debug>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-g -Wall -Wextra>>
    $<$<CONFIG:Release>:$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O2 -Wall -Wextra>>
)

if (WIN32)
    add_executable(${PROJECT_NAME} WIN32 main_callbacks.cpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
    )
else()
    add_executable(${PROJECT_NAME} main_callbacks.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)