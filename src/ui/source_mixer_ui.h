#pragma once

#include "ui_window_base.h"
#include "looper/looper.h"

namespace ui {
    class SourceMixerUi final : public WindowBase
    {
    public:
        explicit SourceMixerUi(looper::Looper &looper);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        looper::Looper *looper_;
    };
}