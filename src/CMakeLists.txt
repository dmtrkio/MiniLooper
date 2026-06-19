add_library(${PROJECT_NAME}_core OBJECT
    audio/audio_device.cpp
    audio/portaudio_backend.cpp
    audio/audio_engine.cpp
    audio/wav_writer.cpp

    dsp/simple_pitch_shifter.cpp
    dsp/effects/effect_base.cpp
    dsp/effects/chorus.cpp
    dsp/effects/guitar_amp.cpp
    dsp/effects/pitch_shifter.cpp
    dsp/parameter/audio_parameter.cpp
    dsp/parameter/parameter_tree.cpp

    midi/midi_message.cpp
    midi/midi.cpp

    looper/source_mixer.cpp
    looper/looper_mixer.cpp
    looper/looper_processor.cpp
    looper/looper_commands.cpp
    looper/looper.cpp
    looper/looper_thumbnail_cache.cpp

    ui/ui_window_base.cpp
    ui/theme.cpp
    ui/parameter_ui.cpp
    ui/audio_settings_ui.cpp
    ui/midi_settings_ui.cpp
    ui/session_manager_ui.cpp
    ui/source_mixer_ui.cpp
    ui/mixer_ui.cpp
    ui/looper_ui.cpp
    ui/audio_thumbnail.cpp
    ui/volume_meter.cpp

    main_application.cpp
    session_manager.cpp
    json.cpp
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
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    #$<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/WX>

    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    #$<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Werror>
)