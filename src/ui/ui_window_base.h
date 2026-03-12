#pragma once

namespace ui {
    struct WindowBase
    {
        bool opened = false;

        virtual ~WindowBase() = default;
        [[nodiscard]] virtual const char* getTitle() const { return "No Title"; }
        virtual void draw() = 0;
    };
}