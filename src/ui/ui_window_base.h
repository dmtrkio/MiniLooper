#pragma once

#include <imgui.h>

namespace ui {
    struct WindowBase
    {
        bool opened = false;

        virtual ~WindowBase() = default;
        [[nodiscard]] virtual const char* getTitle() const { return "No Title"; }

        void draw()
        {
            onFrame();
            if (opened) {
                ImGui::PushID(this);
                ImGui::Begin(getTitle(), &opened);
                drawContent();
                ImGui::End();
                ImGui::PopID();
            }
        }

    protected:
        virtual void onFrame() {}
        virtual void drawContent() = 0;
    };
}