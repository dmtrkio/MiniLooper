#pragma once

#include "ui_window_base.h"
#include "audio/audio_engine.h"

namespace ml::ui {
    class AudioSettingsUi final : public WindowBase
    {
    public:
        AudioSettingsUi(audio::AudioEngine& audioEngine);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        audio::AudioEngine& audioEngine_;
    };
}