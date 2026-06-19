#include "ui_window_base.h"

#include "imgui.h"

namespace ml::ui {
    const char* WindowBase::getTitle() const { return "No Title"; }

    void WindowBase::draw()
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
}