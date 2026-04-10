#pragma once

#include "imgui.h"

#include "ui_window_base.h"
#include "session_manager.h"

namespace ui {
    class SessionManagerUi : public WindowBase
    {
    public:
        explicit SessionManagerUi(SessionManager& sessionManager, looper::Looper& looper)
            : sessionManager_(sessionManager)
            , looper_(looper) {}

        [[nodiscard]] const char* getTitle() const override { return "Session Manager"; }

        void draw() override
        {
            if (!opened) return;

            ImGui::PushID(this);
            ImGui::Begin(getTitle(), &opened);

            {
                const auto pathStr = sessionManager_.getSessionsPath().string();

                ImGui::Text("Sessions Path:");
                ImGui::InputText("##sessions_path", const_cast<char*>(pathStr.c_str()), ImGuiInputTextFlags_ReadOnly);

                if (ImGui::Button("Change Path")) {
                    sessionManager_.openSessionsPathDialog();
                }
            }

            ImGui::Separator();

            {
                if (ImGui::Button("Save Current Session")) {
                    sessionManager_.saveCurrentSessionToDisk(looper_);
                }
            }

            ImGui::End();
            ImGui::PopID();
        }

    private:
        SessionManager& sessionManager_;
        looper::Looper& looper_;
    };
}