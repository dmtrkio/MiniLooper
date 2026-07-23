#include "volume_meter.h"

#include <cstdio>
#include <algorithm>

#include "imgui.h"

namespace ml::ui {
    void volumeMeter(const float leftDb, const float rightDb, float minDb, float maxDb)
    {
        constexpr int segments = 30;
        constexpr float width = 20.0f;
        constexpr float height = 180.0f;
        constexpr float spacing = 8.0f;
        constexpr float gap = 2.0f;

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 pos = ImGui::GetCursorScreenPos();

        constexpr float meterWidth = width * 2 + spacing;
        ImGui::InvisibleButton("##meter", ImVec2(meterWidth + 30.0f, height));

        auto drawMeter = [&](const float db, const float xOffset) {
            const auto clamped = std::clamp(db, minDb, maxDb);
            const auto value = (clamped - minDb) / (maxDb - minDb);

            char label[8];
            if (db > minDb) {
                std::snprintf(label, sizeof(label), "%.0f", clamped);
            } else {
                std::snprintf(label, sizeof(label), "-inf");
            }

            draw->AddText(
                ImVec2(pos.x + xOffset, pos.y - ImGui::GetFontSize() * 0.5f),
                ImGui::GetColorU32(ImGuiCol_Text),
                label
            );

            const float meterHeight = height - ImGui::GetFontSize();
            const float segHeight = (meterHeight - gap * (segments - 1)) / segments;
            const float meterY = pos.y + ImGui::GetFontSize();

            for (int i = 0; i < segments; i++) {
                const float threshold = static_cast<float>(i + 1) / static_cast<float>(segments);
                const bool active = (value >= threshold);

                const float y0 = meterY + meterHeight - static_cast<float>(i + 1) * segHeight - static_cast<float>(i) * gap;
                const float y1 = y0 + segHeight;

                const ImVec2 p0(pos.x + xOffset, y0);
                const ImVec2 p1(pos.x + xOffset + width, y1);

                const ImU32 col = [&]() {
                    constexpr ImU32 inactiveCol = IM_COL32(40, 40, 40, 255);
                    constexpr ImU32 activeCol = IM_COL32(60, 220, 90, 255);
                    constexpr ImU32 hotCol = IM_COL32(255, 210, 60, 255);
                    constexpr ImU32 clippingCol = IM_COL32(255, 60, 60, 255);

                    if (!active) return inactiveCol;
                    if (threshold > 0.9f) return clippingCol;
                    if (threshold > 0.7f) return hotCol;
                    return activeCol;
                }();

                draw->AddRectFilled(p0, p1, col, 2.0f);
            }
        };

        drawMeter(leftDb, 0);
        drawMeter(rightDb, width + spacing);
    }
}