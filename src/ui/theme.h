#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "imgui.h"

namespace ml::ui {
    enum class ImGuiTheme
    {
        HighContrast,   // Stark black & white. Maximum readability.
        Neon,           // Cyberpunk palette on deep blue-black.
        WarmNeutral,    // Easy-on-the-eyes sepia/terracotta light theme.
        Count
    };

    void applyThemeHighContrast();
    void applyThemeNeon();
    void applyThemeWarmNeutral();
    void applyImGuiTheme(ImGuiTheme theme);

    inline constexpr const char* themeToString(ImGuiTheme theme)
    {
        switch (theme) {
            case ImGuiTheme::HighContrast: return "High Contrast";
            case ImGuiTheme::Neon: return "Neon";
            case ImGuiTheme::WarmNeutral: return "Warm Neutral";
            default: return "Unknown Theme";
        }
    }

    inline constexpr std::optional<ImGuiTheme> themeFromString(const std::string_view str)
    {
        if (str == "High Contrast") return ImGuiTheme::HighContrast;
        if (str == "Neon") return ImGuiTheme::Neon;
        if (str == "Warm Neutral") return ImGuiTheme::WarmNeutral;
        throw std::nullopt;
    }

    inline constexpr ImVec4 hexToImVec4(std::uint32_t rgb, float alpha = 1.0f)
    {
        return ImVec4(
            ((rgb >> 16) & 0xFF) / 255.0f,
            ((rgb >> 8)  & 0xFF) / 255.0f,
            ((rgb >> 0)  & 0xFF) / 255.0f,
            alpha
        );
    }
}