#pragma once

#include <vector>

#include "ui_window_base.h"
#include "looper/looper.h"
#include "audio/audio_engine.h"

namespace ml::ui {
    class SourceMixerUi final : public WindowBase
    {
    public:
        explicit SourceMixerUi(const audio::AudioEngine& audioEngine, looper::Looper& looper);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        const audio::AudioEngine* audioEngine_;
        looper::Looper* looper_;

        struct Bool { bool value{}; };
        std::vector<Bool> fxWindowOpened_;
        std::vector<Bool> inputSelectorWindowOpened_{};
    };
}