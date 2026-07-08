#include "popup_manager.h"

#include "imgui.h"

namespace ml::ui {
    PopupManager& PopupManager::getInstance()
    {
        static PopupManager singleton;
        return singleton;
    }

    void PopupManager::errorPopup(std::string text, std::function<void()> onOk)
    {
        pending_.emplace(std::move(text), std::move(onOk));
    }

    void PopupManager::draw()
    {
        if (!current_ && !pending_.empty()) {
            current_ = pending_.front(); pending_.pop();
        }

        if (!current_) return;

        static const char* popupName = "Error";

        if (!ImGui::IsPopupOpen(popupName)) {
            const auto center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::OpenPopup(popupName);
        }

        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(current_->text.c_str());

            if (ImGui::Button("OK")) {
                if (current_->onOk) {
                    current_->onOk();
                }
                current_.reset();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}