#pragma once

#include <utility>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cmath>

#include <imgui.h>

#include "ui/ui_utils.h"

namespace ui {
    inline bool audioThumbnail(
        const char* id,
        const std::pair<float, float>* minMaxPairs,
        int pairCount,
        float playhead,
        const ImVec2& sizeArg = ImVec2(0, 0),
        ImU32 waveformColor   = IM_COL32(230, 220, 60, 255),
        ImU32 backgroundColor = IM_COL32(40, 40, 40, 255)
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

        // 2) Waveform
        if (pairCount == 1) {
            const float x       = bbMin.x + size.x * 0.5f;
            const float minVal  = minMaxPairs[0].first;
            const float maxVal  = minMaxPairs[0].second;
            const float yTop    = centerY - maxVal * halfH;
            const float yBottom = centerY - minVal * halfH;
            const ImU32 color   = (x < playheadX) ? playedWaveColor : unplayedWaveColor;

            drawList->AddLine(ImVec2(x, yTop), ImVec2(x, yBottom), color, 1.0f);
        } else {
            const float step = size.x / static_cast<float>(pairCount - 1);

            for (int i = 0; i < pairCount; ++i) {
                const float x       = bbMin.x + i * step;
                const float minVal  = minMaxPairs[i].first;
                const float maxVal  = minMaxPairs[i].second;
                const float yTop    = centerY - maxVal * halfH;
                const float yBottom = centerY - minVal * halfH;
                const ImU32 color   = (x < playheadX) ? playedWaveColor : unplayedWaveColor;

                drawList->AddLine(ImVec2(x, yTop), ImVec2(x, yBottom), color, 1.0f);
            }
        }

        // 3) Thin playhead divider
        if (pairCount > 0) {
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

        return hovered;
    }

    inline std::vector<std::pair<float, float>> generateSyntheticWaveform(int bucketCount, unsigned int seed = 12345)
    {
        std::vector<std::pair<float, float>> out;
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

    inline void showAudioThumbnailDemo()
    {
        ImGui::Begin("Audio Thumbnail Demo");

        // 1. Generate data once (or cache it somewhere static)
        static std::vector<std::pair<float, float>> waveform = ui::generateSyntheticWaveform(1000);

        // 2. Animated playhead so you can see the color inversion
        static float playhead = 0.0f;
        playhead += ImGui::GetIO().DeltaTime * 0.15f;
        if (playhead > 1.0f) playhead = 0.0f;

        // 3. Draw the widget
        ImGui::Text("Full");
        ui::audioThumbnail("##demo1", waveform.data(), (int)waveform.size(), playhead);

        ImGui::Spacing();

        ImGui::Text("Empty");
        ui::audioThumbnail("##demo2", nullptr, 0, playhead);

        // 4. Manual scrubbing
        ImGui::Spacing();
        ImGui::SliderFloat("Playhead", &playhead, 0.0f, 1.0f);

        ImGui::End();
    }
}