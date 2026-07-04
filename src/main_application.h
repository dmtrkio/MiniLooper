#pragma once

#include <memory>

#include "audio/audio_engine.h"
#include "session_manager.h"
#include "looper/looper.h"
#include "midi/midi.h"
#include "ui/main_ui.h"

namespace ml {
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
        void loadJsonSettings();
        void saveJsonSettings() const;
        void processInput();

        std::unique_ptr<audio::AudioEngine> audioEngine_;
        SessionManager sessionManager_;
        looper::Looper looper_;
        std::unique_ptr<midi::MidiEngine> midiEngine_;

        ui::MainUi ui_;
    };
}