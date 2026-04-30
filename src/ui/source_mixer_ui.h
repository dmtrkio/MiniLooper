#pragma once

#include "ui_window_base.h"
#include "looper/looper.h"
#include "parameter_ui.h"

namespace ui {
    class SourceMixerUi final : public WindowBase
    {
    public:
        explicit SourceMixerUi(looper::Looper &looper) : looper_(&looper) {}

        [[nodiscard]] const char* getTitle() const override { return "Source Mixer"; }

    protected:
        void drawContent() override
        {
            const auto paramTree = looper_->getParameterTree()["SourceMixer"];
            // temporary view
            parameterTreeUi(paramTree);
        }

    private:
        looper::Looper *looper_;
    };
}