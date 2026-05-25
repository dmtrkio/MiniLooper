#pragma once

#include <utility>
#include <vector>
#include <cstdlib>
#include <cmath>

#include <imgui.h>

#include "dsp/dsp.h"

namespace ui {
    bool audioThumbnail(
        const char* id,
        const dsp::MinMax* minMaxPairs,
        int pairCount,
        float playhead,
        bool drawPlayhead = true,
        const ImVec2& sizeArg = ImVec2(0, 0),
        ImU32 waveformColor   = IM_COL32(230, 220, 60, 255),
        ImU32 backgroundColor = IM_COL32(40, 40, 40, 255)
    );

    inline std::vector<dsp::MinMax> generateSyntheticWaveform(int bucketCount, unsigned int seed = 12345)
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

    inline void showAudioThumbnailDemo()
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
        ui::audioThumbnail("##demo1", waveform.data(), (int)waveform.size(), playhead);

        ImGui::Spacing();

        ImGui::Text("Empty");
        ui::audioThumbnail("##demo2", nullptr, (int)waveform.size(), playhead);

        // 4. Manual scrubbing
        ImGui::Spacing();
        ImGui::SliderFloat("Playhead", &playhead, 0.0f, 1.0f);

        ImGui::End();
    }
}