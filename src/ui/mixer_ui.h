#pragma once

#include "ui_window_base.h"
#include "looper/looper.h"
#include "looper/looper_thumbnail_cache.h"

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
        looper::ThumbnailCache thumbnailCache_;
        dsp::parameter::ParameterTree paramTree_;

        struct Bool
        {
            bool value{};
        };

        std::vector<Bool> eqOpened_;
    };
}