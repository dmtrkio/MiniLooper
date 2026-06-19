#pragma once

namespace ui {
    struct WindowBase
    {
        bool opened = false;

        virtual ~WindowBase() = default;

        [[nodiscard]] virtual const char* getTitle() const;

        void draw();

    protected:
        virtual void onFrame() {}
        virtual void drawContent() = 0;
    };
}