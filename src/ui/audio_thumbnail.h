#pragma once

#include <span>
#include <imgui.h>

#include "dsp/dsp.h"

namespace ml::ui {
    bool audioThumbnail(
        const char* id,
        std::span<const dsp::MinMax> minMaxPairs,
        float playhead,
        bool drawPlayhead = true,
        const ImVec2& sizeArg = ImVec2(0, 0),
        ImU32 waveformColor   = IM_COL32(230, 220, 60, 255),
        ImU32 backgroundColor = IM_COL32(40, 40, 40, 255)
    );

    void showAudioThumbnailDemo();
}