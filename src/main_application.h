#pragma once

#include <memory>
#include <vector>

#include "audio/audio_engine.h"
#include "session_manager.h"
#include "looper/looper.h"
#include "midi/midi.h"
#include "ui/ui_window_base.h"
#include "ui/theme.h"

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
    void processInput();
    void drawTopBarMenu();

    std::unique_ptr<audio::AudioEngine> audioEngine_;
    SessionManager sessionManager_;
    looper::Looper looper_;
    std::unique_ptr<midi::MidiEngine> midiEngine_;

    ui::ImGuiTheme currentTheme_ = ui::ImGuiTheme::WarmNeutral;
    std::vector<std::unique_ptr<ui::WindowBase>> windowRegistry_;
};