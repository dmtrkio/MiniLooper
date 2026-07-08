#include "ui_window_base.h"

#include "imgui.h"

#include "popup_manager.h"

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

    void WindowBase::showErrorPopup(std::string text, std::function<void()> onOk)
    {
        PopupManager::getInstance().errorPopup(std::move(text), std::move(onOk));
    }
}