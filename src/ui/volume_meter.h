#pragma once

#include "ui_window_base.h"
#include "looper/looper.h"

namespace ui {
    void volumeMeter(const float leftDb, const float rightDb);

    class VolumeMeterWindow final : public WindowBase
    {
    public:
        explicit VolumeMeterWindow(looper::Looper &looper);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        looper::Looper *looper_;
    };
}