#pragma once

#include <vector>

#include "looper/looper.h"
#include "session_manager.h"
#include "audio/audio_engine.h"
#include "midi/midi.h"
#include "ui/ui_window_base.h"
#include "ui/theme.h"
#include "json.h"

namespace ml::ui {
    class MainUi
    {
    public:
        MainUi(
            SessionManager& sessionManager,
            audio::AudioEngine& audioEngine,
            midi::MidiEngine& midiEngine,
            looper::Looper& looper
        );

        json serializeToJson() const;
        void deserializeJson(const json& j);

        void runFrame();

    private:
        void drawTopBarMenu();

        ui::ImGuiTheme currentTheme_ = ui::ImGuiTheme::WarmNeutral;
        std::vector<std::unique_ptr<ui::WindowBase>> windowRegistry_;
        looper::Looper& looper_;
    };
}