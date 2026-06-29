#include "toggle_widget.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace ml::ui {
    bool toggle(const char* label, bool& v) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) {
            return false;
        }

        const ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiID id = window->GetID(label);

        const float lineHeight = ImGui::GetFrameHeight();
        const float trackHeight = lineHeight - style.FramePadding.y * 2.0f;
        const float trackWidth = trackHeight * 1.8f;
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        const float trackY = pos.y + style.FramePadding.y;

        const ImRect trackBoundingBox(ImVec2(pos.x, trackY), ImVec2(pos.x + trackWidth, trackY + trackHeight));

        ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
        const float totalWidth = trackWidth + (labelSize.x > 0.0f ? style.ItemInnerSpacing.x + labelSize.x : 0.0f);
        const float totalHeight = ImMax(lineHeight, labelSize.y);
        const ImRect totalBoundingBox(pos, ImVec2(pos.x + totalWidth, pos.y + totalHeight));

        ImGui::ItemSize(totalBoundingBox, style.FramePadding.y);
        if (!ImGui::ItemAdd(totalBoundingBox, id)) {
            return false;
        }

        bool hovered;
        bool pressed = ImGui::ButtonBehavior(totalBoundingBox, id, &hovered, nullptr);
        if (pressed) {
            v = !v;
        }

        const auto trackCol = [&] {
            if (v) {
                return ImGui::GetColorU32(ImGuiCol_ButtonActive);
            } else if (hovered) {
                return ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
            } else {
                return ImGui::GetColorU32(ImGuiCol_FrameBg);
            }
        }();

        ImDrawList* draw = ImGui::GetWindowDrawList();

        const float rounding = trackHeight * 0.5f;
        draw->AddRectFilled(trackBoundingBox.Min, trackBoundingBox.Max, trackCol, rounding);

        if (!v) {
            const auto borderCol = ImGui::GetColorU32(ImGuiCol_Border);
            draw->AddRect(trackBoundingBox.Min, trackBoundingBox.Max, borderCol, rounding, 0, 1.0f);
        }

        const float knobRadius = trackHeight * 0.4f;
        const float centerY = (trackBoundingBox.Min.y + trackBoundingBox.Max.y) * 0.5f;

        const auto knobPos = [=] {
            if (v) {
                return ImVec2(trackBoundingBox.Max.x - knobRadius, centerY);
            } else {
                return ImVec2(trackBoundingBox.Min.x + knobRadius, centerY);
            }
        }();

        const auto knobCol = ImGui::GetColorU32(ImGuiCol_Text);
        const auto knobBorderCol = ImGui::GetColorU32(ImGuiCol_Border);

        draw->AddCircleFilled(knobPos, knobRadius, knobCol, 16);
        draw->AddCircle(knobPos, knobRadius, knobBorderCol, 16, 1.0f);

        if (labelSize.x > 0.0f) {
            ImGui::RenderText(ImVec2(trackBoundingBox.Max.x + style.ItemInnerSpacing.x, pos.y + style.FramePadding.y), label);
        }

        return pressed;
    }
}