#include "controls_ui.h"

#include <string>

#include "imgui.h"


namespace ml::ui {
    ControlsHelpWindow::ControlsHelpWindow(std::size_t numTracks)
        : numTracks_(numTracks) {}

    const char* ControlsHelpWindow::getTitle() const { return "Controls Help"; }

    void ControlsHelpWindow::drawContent()
    {
        ImGui::TextUnformatted("Keyboard shortcuts for the looper.");
        ImGui::Spacing();

        if (ImGui::BeginTable("ShortcutsTable", 3,
                            ImGuiTableFlags_Borders |
                            ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Modifier", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            const std::string trackKey = "1 - " + std::to_string(numTracks_);
            const char* none = "";

            const auto row = [](const char* key, const char* mod, const char* action) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(key);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(mod);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(action);
            };

            row(trackKey.c_str(), none,          "Toggle recording");
            row(trackKey.c_str(), "Shift",       "Toggle playback");
            row(trackKey.c_str(), "Ctrl",        "Clear track");
            row(trackKey.c_str(), "Alt",         "Toggle footswitch assignment");
            row("C",              "Ctrl",        "Clear all tracks");
            row("S",              "Ctrl",        "Save Session");

            ImGui::EndTable();
        }
    }
}