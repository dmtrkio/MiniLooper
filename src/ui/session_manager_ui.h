#pragma once

#include "ui_window_base.h"
#include "session_manager.h"

namespace ui {
    class SessionManagerUi final : public WindowBase
    {
    public:
        explicit SessionManagerUi(SessionManager& sessionManager, looper::Looper& looper);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        SessionManager& sessionManager_;
        looper::Looper& looper_;
    };
}