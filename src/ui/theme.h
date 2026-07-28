#pragma once

#include <optional>
#include <string_view>

namespace ml::ui {
    enum class ImGuiTheme
    {
        HighContrast,   // Black & white.
        Neon,           // Roughly tokyonight inspired.
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
}
