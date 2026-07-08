#pragma once

#include <functional>
#include <string>

namespace ml::ui {
    struct WindowBase
    {
        bool opened = false;

        virtual ~WindowBase() = default;

        [[nodiscard]] virtual const char* getTitle() const;

        void draw();
    protected:
        virtual void onFrame() {}
        virtual void drawContent() = 0;

        void showErrorPopup(std::string text, std::function<void()> onOk = nullptr);
    };
}