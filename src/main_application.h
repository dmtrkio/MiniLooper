#pragma once

#include <memory>

#include "looper/looper.h"
#include "midi/midi.h"
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
    static constexpr auto kNoDeviceString = "No device";

    void processInput();
    void audioEngineSettings();
    void midiEngineSettings();

    void looperUi();
    void trackUi(int trackIndex);
    void toggleRec(int trackIndex);
    void togglePlay(int trackIndex);

    bool showAudioSettings_ = false;
    bool showMidiSettings_ = false;
    bool showTracks_ = false;
    bool showVolumeMeter_ = true;

    ui::MixerUi mixerUi_;

    looper::Looper looper_;
    std::unique_ptr<midi::MidiEngine> midiEngine_;
};
