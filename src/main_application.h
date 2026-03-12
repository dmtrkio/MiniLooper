#pragma once

#include <memory>

#include "looper/looper.h"
#include "midi/midi.h"
#include "ui/audio_settings_ui.h"
#include "ui/looper_ui.h"
#include "ui/midi_settings_ui.h"
#include "ui/mixer_ui.h"

class MainApplication
{
public:
    MainApplication(int argc, const char * const *argv);
    ~MainApplication();

    MainApplication(const MainApplication &) = delete;
    MainApplication(const MainApplication &&) = delete;

    MainApplication &operator=(const MainApplication &) = delete;
    MainApplication &operator=(const MainApplication &&) = delete;

    void onFrame();

private:
    looper::Looper looper_;
    std::unique_ptr<midi::MidiEngine> midiEngine_;

    ui::AudioSettingsUi audioSettingsUi_;
    ui::MidiSettingsUi midiSettingsUi_;
    ui::LooperUi looperUi_;
    ui::MixerUi mixerUi_;

    bool showVolumeMeter_ = true;
};