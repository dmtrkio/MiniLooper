#pragma once

#include <imgui.h>
#include <cstdint>

namespace ml::ui {
    inline constexpr ImU32 fadeAlpha(ImU32 color, float t) noexcept
    {
        int a = (color >> 24) & 0xFF;
        a = (int)(a * t);
        return (color & 0x00FFFFFF) | (a << 24);
    }

    inline constexpr ImU32 scaleRgb(ImU32 color, float t) noexcept
    {
        int r =  color        & 0xFF;
        int g = (color >> 8)  & 0xFF;
        int b = (color >> 16) & 0xFF;
        int a = (color >> 24) & 0xFF;

        r = (int)(r * t);
        g = (int)(g * t);
        b = (int)(b * t);

        if (r > 255) r = 255; else if (r < 0) r = 0;
        if (g > 255) g = 255; else if (g < 0) g = 0;
        if (b > 255) b = 255; else if (b < 0) b = 0;

        return (r | (g << 8) | (b << 16) | (a << 24));
    }

    inline constexpr ImVec4 hexToImVec4(std::uint32_t rgb, float alpha = 1.0f) noexcept
    {
        return ImVec4(
            ((rgb >> 16) & 0xFF) / 255.0f,
            ((rgb >> 8)  & 0xFF) / 255.0f,
            ((rgb >> 0)  & 0xFF) / 255.0f,
            alpha
        );
    }
}