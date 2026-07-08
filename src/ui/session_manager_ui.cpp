#include "session_manager_ui.h"

#include <iostream>

#include "imgui.h"

namespace ml::ui {
    SessionManagerUi::SessionManagerUi(SessionManager& sessionManager, looper::Looper& looper)
        : sessionManager_(sessionManager)
        , looper_(looper)
    {}

    const char* SessionManagerUi::getTitle() const { return "Session Manager"; }

    void SessionManagerUi::onFrame()
    {
        if (const auto r = sessionManager_.pollPendingSessionToLoad(looper_); !r) {
            std::cerr << r.error() << std::endl;
            showErrorPopup(r.error());
        }
    }

    void SessionManagerUi::drawContent()
    {
        {
            const auto pathStr = sessionManager_.getSessionsPath().string();

            ImGui::Text("Sessions Path:");
            ImGui::InputText("##sessions_path", const_cast<char*>(pathStr.c_str()), ImGuiInputTextFlags_ReadOnly);

            if (ImGui::Button("Change Path")) {
                sessionManager_.openSessionsPathDialog();
            }

            ImGui::SameLine();

            if (ImGui::Button("Open Session")) {
                sessionManager_.openLoadSessionDialog();
            }
        }

        ImGui::Separator();

        {
            if (ImGui::Button("Save Current Session")) {
                if (const auto r = sessionManager_.saveCurrentSession(looper_); !r) {
                    std::cerr << r.error() << std::endl;
                    showErrorPopup(r.error());
                }
            }
        }
    }
}