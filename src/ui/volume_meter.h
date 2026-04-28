#pragma once

#include <algorithm>

#include "imgui.h"
#include "ui_window_base.h"
#include "looper/looper.h"

namespace ui {
    inline void volumeMeter(const float leftDb, const float rightDb)
    {
        constexpr float kMinDb = -60.0f;
        constexpr float kMaxDb = 6.0f;

        constexpr int segments = 30;
        constexpr float width = 18.0f;
        constexpr float height = 140.0f;
        constexpr float spacing = 8.0f;
        constexpr float gap = 2.0f;

        auto normalize = [&](float db) {
            db = std::clamp(db, kMinDb, kMaxDb);
            return (db - kMinDb) / (kMaxDb - kMinDb);
        };

        const float l = normalize(leftDb);
        const float r = normalize(rightDb);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 pos = ImGui::GetCursorScreenPos();

        constexpr float meterWidth = width * 2 + spacing;
        ImGui::InvisibleButton("##meter", ImVec2(meterWidth + 30.0f, height));

        const float top = pos.y;
        const float bottom = pos.y + height;

        auto segmentColor = [&](const int i) {
            const auto t = static_cast<float>(i) / static_cast<float>(segments);
            if (t > 0.9f)   return IM_COL32(255, 60, 60, 255);
            if (t > 0.7f)   return IM_COL32(255, 210, 60, 255);
            return IM_COL32(60, 220, 90, 255);
        };

        auto drawMeter = [&](const float value, const float xOffset) {
            constexpr float segHeight = (height - gap * (segments - 1)) / segments;

            for (int i = 0; i < segments; i++) {
                const float threshold = static_cast<float>(i + 1) / static_cast<float>(segments);
                const bool active = (value >= threshold);

                const float y0 = pos.y + height - static_cast<float>(i + 1) * segHeight - static_cast<float>(i) * gap;
                const float y1 = y0 + segHeight;

                ImVec2 p0(pos.x + xOffset, y0);
                ImVec2 p1(pos.x + xOffset + width, y1);

                const ImU32 col = active ? segmentColor(i) : IM_COL32(40, 40, 40, 255);
                draw->AddRectFilled(p0, p1, col, 2.0f);
            }
        };

        drawMeter(l, 0);
        drawMeter(r, width + spacing);

        auto dbToY = [&](const float db) {
            const float t = normalize(db);
            return bottom - (bottom - top) * t;
        };

        constexpr float ticks[] = { -60, -48, -36, -24, -12, -6, 0, 6 };

        for (const float db : ticks) {
            constexpr float tickWidth = 6.0f;
            const float y = dbToY(db);

            const ImVec2 t0(pos.x + meterWidth + 4, y);
            const ImVec2 t1(pos.x + meterWidth + 4 + tickWidth, y);

            //draw->AddLine(t0, t1, IM_COL32(200, 200, 200, 255), 1.0f);

            char label[8];
            snprintf(label, sizeof(label), "%.0f", db);

            draw->AddText(
                ImVec2(t1.x + 4, y - ImGui::GetFontSize() * 0.5f),
                IM_COL32(200, 200, 200, 255),
                label
            );
        }
    }

    class VolumeMeterWindow : public WindowBase
    {
    public:
        explicit VolumeMeterWindow(looper::Looper &looper) : looper_(&looper) {}

        [[nodiscard]] const char* getTitle() const override { return "Volume Meter"; }

    protected:
        void drawContent() override
        {
            const auto [leftDb, rightDb] = looper_->getLooperState().level;
            volumeMeter(leftDb, rightDb);
        }

    private:
        looper::Looper *looper_;
    };
}