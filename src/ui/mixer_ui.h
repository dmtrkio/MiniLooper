#pragma once

#include "ui_window_base.h"
#include "looper/looper.h"

namespace ml::ui {
    class MixerUi final : public WindowBase
    {
    public:
        explicit MixerUi(looper::Looper &looper);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        void drawTrack(const int trackIndex);

        looper::Looper *looper_;

        struct BoolWrapper { bool value{}; };
        std::vector<BoolWrapper> fxWindowOpened_;
    };
}