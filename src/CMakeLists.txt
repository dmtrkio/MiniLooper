add_library(${PROJECT_NAME}_core OBJECT
    audio/audio_engine.cpp
    audio/wav_writer.cpp
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