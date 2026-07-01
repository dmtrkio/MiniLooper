#pragma once

#include <cstddef>

#include "ui_window_base.h"

namespace ml::ui {
    class ControlsHelpWindow : public WindowBase
    {
    public:
        explicit ControlsHelpWindow(std::size_t numTracks);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        std::size_t numTracks_;
    };
}