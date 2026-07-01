#include "audio_thumbnail.h"

#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cmath>

#include "ui/ui_utils.h"

namespace ml::ui {
    bool audioThumbnail(
        const char* id,
        std::span<const dsp::MinMax> minMaxPairs,
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

        const auto pairCount = static_cast<int>(minMaxPairs.size());

        const float playheadClamped = pairCount > 0 ? std::clamp(playhead, 0.0f, 1.0f) : 0.0f;
        const float playheadX       = bbMin.x + size.x * playheadClamped;
        const float halfH           = size.y * 0.5f;
        const float centerY         = bbMin.y + halfH;

        const ImU32 playedBgColor = backgroundColor;
        const ImU32 playedWaveColor = waveformColor;
        
        constexpr float unplayedAlphaScale = 0.5f;
        const ImU32 unplayedBgColor = scaleRgb(backgroundColor, unplayedAlphaScale);
        const ImU32 unplayedWaveColor = scaleRgb(waveformColor, unplayedAlphaScale);

        // 1) Background
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

        // 2) Waveform
        if (pairCount == 1) {
            const float x = bbMin.x + size.x * 0.5f;
            const auto [minVal, maxVal] = getMinMaxPair(0);

            const float yTop    = centerY - maxVal * halfH;
            const float yBottom = centerY - minVal * halfH;
            const ImU32 color   = (x < playheadX) ? playedWaveColor : unplayedWaveColor;

            drawList->AddLine(ImVec2(x, yTop), ImVec2(x, yBottom), color, 1.0f);
        } else {
            const float step = size.x / static_cast<float>(pairCount - 1);
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
                    const float t = (playheadX - x0) / (x1 - x0);
                    const float yTopSplit    = yTop0    + t * (yTop1    - yTop0);
                    const float yBottomSplit = yBottom0 + t * (yBottom1 - yBottom0);

                    drawSegment(x0, yTop0, yBottom0, playheadX, yTopSplit, yBottomSplit, playedWaveColor);
                    drawSegment(playheadX, yTopSplit, yBottomSplit, x1, yTop1, yBottom1, unplayedWaveColor);
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

    static std::vector<dsp::MinMax> generateSyntheticWaveform(int bucketCount, unsigned int seed = 12345)
    {
        std::vector<dsp::MinMax> out;
        out.reserve(bucketCount);

        std::srand(seed);

        // Frequencies (in cycles across the whole buffer) + amplitudes
        constexpr float freqs[]   = { 3.0f, 11.0f, 23.5f, 47.0f };
        constexpr float amps[]    = { 0.45f, 0.25f, 0.15f, 0.08f };
        constexpr int   nOsc      = sizeof(freqs) / sizeof(freqs[0]);

        // Slow amplitude envelope (LFO) so the waveform "breathes"
        constexpr float lfoFreq   = 1.2f; // cycles across the buffer
        constexpr float lfoDepth  = 0.35f;

        for (int i = 0; i < bucketCount; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(bucketCount - 1);

            // Slow envelope
            const float envelope = 1.0f - lfoDepth * 0.5f * (1.0f - std::cosf(t * lfoFreq * 2.0f * 3.14159265f));

            // Sum sines
            float sample = 0.0f;
            for (int o = 0; o < nOsc; ++o)
                sample += amps[o] * std::sinf(t * freqs[o] * 2.0f * 3.14159265f);

            sample *= envelope;

            // Add a little high-frequency noise so the min/max don't perfectly overlap
            const float noise = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 0.08f;
            sample += noise;

            // Clamp
            if (sample > 1.0f)  sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;

            // Derive min/max for this bucket. For a single instantaneous sample we
            // fake a "thickness" so the line is visible even when the signal is near 0.
            const float thickness = 0.02f + 0.06f * std::abs(sample);
            const float maxVal = sample + thickness;
            const float minVal = sample - thickness;

            out.emplace_back(minVal, maxVal);
        }

        return out;
    }

    void showAudioThumbnailDemo()
    {
        ImGui::Begin("Audio Thumbnail Demo");

        // 1. Generate data once (or cache it somewhere static)
        static std::vector<dsp::MinMax> waveform = ui::generateSyntheticWaveform(200);

        // 2. Animated playhead so you can see the color inversion
        static float playhead = 0.0f;
        playhead += ImGui::GetIO().DeltaTime * 0.15f;
        if (playhead > 1.0f) playhead = 0.0f;

        // 3. Draw the widget
        ImGui::Text("Full");
        ui::audioThumbnail("##demo1", std::span{waveform.data(), waveform.size()}, playhead);

        ImGui::Spacing();

        // 4. Manual scrubbing
        ImGui::Spacing();
        ImGui::SliderFloat("Playhead", &playhead, 0.0f, 1.0f);

        ImGui::End();
    }
}