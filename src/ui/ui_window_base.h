#pragma once

#include <string>

#include "imgui.h"

namespace ui {
    struct WindowBase
    {
        bool opened = false;
        const std::string title;

        virtual ~WindowBase() = default;
        virtual void draw() = 0;
    };
}