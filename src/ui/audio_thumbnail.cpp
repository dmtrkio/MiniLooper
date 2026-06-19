#include "audio_thumbnail.h"

#include <algorithm>

#include "ui/ui_utils.h"

namespace ml::ui {
    bool audioThumbnail(
        const char* id,
        const dsp::MinMax* minMaxPairs,
        int pairCount,
        float playhead,
        bool drawPlayhead,
        const ImVec2& sizeArg,
        ImU32 waveformColor,
        ImU32 backgroundColor
    )
    {
        ImVec2 size = sizeArg;
        if (size.x <= 0.0f)
            size.x = ImGui::CalcItemWidth();
        if (size.y <= 0.0f)
            size.y = ImGui::GetFrameHeight();

        const ImVec2 bbMin = ImGui::GetCursorScreenPos();
        const ImVec2 bbMax = ImVec2(bbMin.x + size.x, bbMin.y + size.y);

        ImGui::InvisibleButton(id, size);
        const bool hovered = ImGui::IsItemHovered();

        if (!ImGui::IsItemVisible() || size.x <= 0.0f || size.y <= 0.0f)
            return hovered;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();

        const float playheadClamped = pairCount > 0 ? std::clamp(playhead, 0.0f, 1.0f) : 0.0f;
        const float playheadX       = bbMin.x + size.x * playheadClamped;
        const float halfH           = size.y * 0.5f;
        const float centerY         = bbMin.y + halfH;

        const ImU32 playedBgColor = backgroundColor;
        const ImU32 playedWaveColor = waveformColor;
        
        constexpr float unplayedAlphaScale = 0.5f;
        const ImU32 unplayedBgColor = scaleRgb(backgroundColor, unplayedAlphaScale);
        const ImU32 unplayedWaveColor = scaleRgb(waveformColor, unplayedAlphaScale);

        // 1) Background rectangles — rounded only on their outer corners so the
        //    seam at playheadX stays perfectly vertical.
        drawList->AddRectFilled(
            bbMin,
            ImVec2(playheadX, bbMax.y),
            playedBgColor,
            style.FrameRounding,
            ImDrawFlags_RoundCornersLeft
        );
        drawList->AddRectFilled(
            ImVec2(playheadX, bbMin.y),
            bbMax,
            unplayedBgColor,
            style.FrameRounding,
            ImDrawFlags_RoundCornersRight
        );

        drawList->PushClipRect(bbMin, bbMax, true);

        auto getMinMaxPair = [&minMaxPairs] (std::size_t index) {
            const auto [minVal, maxVal] = minMaxPairs[index];
            return std::make_pair(
                std::max(minVal, -1.0f),
                std::min(maxVal, 1.0f)
            );
        };

        auto drawBin = [&](float x, float minVal, float maxVal) {
            const float yTop    = centerY - maxVal * halfH;
            const float yBottom = centerY - minVal * halfH;
            const ImU32 color   = (x < playheadX) ? playedWaveColor : unplayedWaveColor;

            drawList->AddLine(ImVec2(x, yTop), ImVec2(x, yBottom), color, 1.0f);
        };

        // 2) Waveform
        if (minMaxPairs) {
            if (pairCount == 1) {
                const float x = bbMin.x + size.x * 0.5f;
                const auto [minVal, maxVal] = getMinMaxPair(0);
                drawBin(x, minVal, maxVal);
            } else {
                const float step = size.x / static_cast<float>(pairCount - 1);
                const bool sparse = step > 2.0f;

                if (sparse) {
                    for (int i = 0; i < pairCount; ++i) {
                        const float x = bbMin.x + i * step;
                        const auto [minVal, maxVal] = getMinMaxPair(i);
                        drawBin(x, minVal, maxVal);
                    }
                } else {
                    // Sparse data — filled segments so the waveform is one continuous ribbon
                    auto drawSegment = [&](
                        float x0, float yTop0, float yBottom0,
                        float x1, float yTop1, float yBottom1,
                        ImU32 color
                    ) {
                        drawList->AddQuadFilled(
                            ImVec2(x0, yTop0),
                            ImVec2(x1, yTop1),
                            ImVec2(x1, yBottom1),
                            ImVec2(x0, yBottom0),
                            color
                        );
                    };

                    for (int i = 0; i < pairCount - 1; ++i) {
                        const float x0 = bbMin.x + i * step;
                        const float x1 = bbMin.x + (i + 1) * step;

                        const auto [minVal0, maxVal0] = getMinMaxPair(i);
                        const auto [minVal1, maxVal1] = getMinMaxPair(i + 1);

                        const float yTop0    = centerY - maxVal0 * halfH;
                        const float yBottom0 = centerY - minVal0 * halfH;
                        const float yTop1    = centerY - maxVal1 * halfH;
                        const float yBottom1 = centerY - minVal1 * halfH;

                        if (x1 <= playheadX) {
                            drawSegment(x0, yTop0, yBottom0, x1, yTop1, yBottom1, playedWaveColor);
                        } else if (x0 >= playheadX) {
                            drawSegment(x0, yTop0, yBottom0, x1, yTop1, yBottom1, unplayedWaveColor);
                        } else {
                            // Segment straddles the playhead — split it so the seam stays vertical
                            const float t = (playheadX - x0) / (x1 - x0);
                            const float yTopSplit    = yTop0    + t * (yTop1    - yTop0);
                            const float yBottomSplit = yBottom0 + t * (yBottom1 - yBottom0);

                            drawSegment(x0, yTop0, yBottom0, playheadX, yTopSplit, yBottomSplit, playedWaveColor);
                            drawSegment(playheadX, yTopSplit, yBottomSplit, x1, yTop1, yBottom1, unplayedWaveColor);
                        }
                    }
                }
            }
        }

        // 3) Playhead divider
        if (drawPlayhead) {
            drawList->AddLine(
                ImVec2(playheadX, bbMin.y),
                ImVec2(playheadX, bbMax.y),
                ImGui::GetColorU32(ImGuiCol_Border),
                1.0f
            );
        }

        // 4) Border — same color, thickness and rounding as ImGui frames/sliders
        if (style.FrameBorderSize > 0.0f) {
            drawList->AddRect(
                bbMin,
                bbMax,
                ImGui::GetColorU32(ImGuiCol_Border),
                style.FrameRounding,
                ImDrawFlags_RoundCornersAll,
                style.FrameBorderSize
            );
        }

        drawList->PopClipRect();

        return hovered;
    }
}