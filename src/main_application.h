#pragma once

#include <memory>

#include "dsp/parameter/parameter_tree.h"
#include "looper/looper.h"
#include "midi/midi.h"

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
    bool showTracks_ = true;
    bool showVolumeMeter_ = false;
    bool showMixer_ = true;

    looper::Looper looper_;
    std::unique_ptr<midi::MidiEngine> midiEngine_;
};
