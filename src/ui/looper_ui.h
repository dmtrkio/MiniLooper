#pragma once

#include "ui_window_base.h"
#include "looper/looper.h"
#include "looper/looper_thumbnail_cache.h"
#include "session_manager.h"

namespace ml::ui {
    class LooperUi final : public WindowBase
    {
    public:
        explicit LooperUi(looper::Looper &looper, SessionManager& sessionManager);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        void drawTrack(const int trackIndex);

        looper::Looper *looper_;
        looper::ThumbnailCache thumbnailCache_;
        SessionManager *sessionManager_;
    };
}