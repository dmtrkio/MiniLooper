#include "session_manager_ui.h"

#include "imgui.h"

namespace ml::ui {
    SessionManagerUi::SessionManagerUi(SessionManager& sessionManager, looper::Looper& looper)
        : sessionManager_(sessionManager)
        , looper_(looper)
    {}

    const char* SessionManagerUi::getTitle() const { return "Session Manager"; }

    void SessionManagerUi::drawContent()
    {
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
    }
}