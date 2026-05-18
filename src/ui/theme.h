#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "imgui.h"

namespace ui {
    enum class ImGuiTheme
    {
        HighContrast,   // Stark black & white. Maximum readability.
        Neon,           // Cyberpunk palette on deep blue-black.
        WarmNeutral,    // Easy-on-the-eyes sepia/terracotta light theme.
        Count
    };

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

    inline void applyThemeHighContrast()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;

        // --- Text ---
        c[ImGuiCol_Text]                  = hexToImVec4(0xFFFFFF);
        c[ImGuiCol_TextDisabled]          = hexToImVec4(0x888888);
        c[ImGuiCol_TextSelectedBg]        = hexToImVec4(0xFFFFFF, 0.30f);

        // --- Backgrounds ---
        c[ImGuiCol_WindowBg]              = hexToImVec4(0x050505);
        c[ImGuiCol_ChildBg]               = hexToImVec4(0x080808);
        c[ImGuiCol_PopupBg]               = hexToImVec4(0x0A0A0A);
        c[ImGuiCol_MenuBarBg]             = hexToImVec4(0x111111);

        // --- Borders ---
        c[ImGuiCol_Border]                = hexToImVec4(0xFFFFFF, 0.60f);
        c[ImGuiCol_BorderShadow]          = hexToImVec4(0x000000, 0.00f);

        // --- Frames ---
        c[ImGuiCol_FrameBg]               = hexToImVec4(0x111111);
        c[ImGuiCol_FrameBgHovered]        = hexToImVec4(0x222222);
        c[ImGuiCol_FrameBgActive]         = hexToImVec4(0x333333);

        // --- Titles ---
        c[ImGuiCol_TitleBg]               = hexToImVec4(0x111111);
        c[ImGuiCol_TitleBgActive]         = hexToImVec4(0x222222);
        c[ImGuiCol_TitleBgCollapsed]      = hexToImVec4(0x111111);

        // --- Buttons ---
        c[ImGuiCol_Button]                = hexToImVec4(0x222222);
        c[ImGuiCol_ButtonHovered]         = hexToImVec4(0x444444);
        c[ImGuiCol_ButtonActive]          = hexToImVec4(0x666666);

        // --- Headers ---
        c[ImGuiCol_Header]                = hexToImVec4(0x333333);
        c[ImGuiCol_HeaderHovered]         = hexToImVec4(0x555555);
        c[ImGuiCol_HeaderActive]          = hexToImVec4(0x777777);

        // --- Scrollbar ---
        c[ImGuiCol_ScrollbarBg]           = hexToImVec4(0x0A0A0A);
        c[ImGuiCol_ScrollbarGrab]         = hexToImVec4(0x444444);
        c[ImGuiCol_ScrollbarGrabHovered]  = hexToImVec4(0x666666);
        c[ImGuiCol_ScrollbarGrabActive]   = hexToImVec4(0x888888);

        // --- Checkmarks & Sliders ---
        c[ImGuiCol_CheckMark]             = hexToImVec4(0xFFFFFF);
        c[ImGuiCol_SliderGrab]            = hexToImVec4(0xCCCCCC);
        c[ImGuiCol_SliderGrabActive]      = hexToImVec4(0xFFFFFF);

        // --- Separators ---
        c[ImGuiCol_Separator]             = hexToImVec4(0xFFFFFF, 0.40f);
        c[ImGuiCol_SeparatorHovered]      = hexToImVec4(0xFFFFFF, 0.60f);
        c[ImGuiCol_SeparatorActive]       = hexToImVec4(0xFFFFFF);

        // --- Resize Grip ---
        c[ImGuiCol_ResizeGrip]            = hexToImVec4(0x444444);
        c[ImGuiCol_ResizeGripHovered]     = hexToImVec4(0x666666);
        c[ImGuiCol_ResizeGripActive]      = hexToImVec4(0x888888);

        // --- Tabs ---
        c[ImGuiCol_Tab]                   = hexToImVec4(0x1A1A1A);
        c[ImGuiCol_TabHovered]            = hexToImVec4(0x333333);
        c[ImGuiCol_TabActive]             = hexToImVec4(0x444444);
        c[ImGuiCol_TabUnfocused]          = hexToImVec4(0x151515);
        c[ImGuiCol_TabUnfocusedActive]    = hexToImVec4(0x3A3A3A);

        // --- Docking ---
        c[ImGuiCol_DockingPreview]        = hexToImVec4(0xFFFFFF, 0.30f);
        c[ImGuiCol_DockingEmptyBg]        = hexToImVec4(0x080808);

        // --- Plots ---
        c[ImGuiCol_PlotLines]             = hexToImVec4(0xFFFFFF);
        c[ImGuiCol_PlotLinesHovered]      = hexToImVec4(0xCCCCCC);
        c[ImGuiCol_PlotHistogram]         = hexToImVec4(0xAAAAAA);
        c[ImGuiCol_PlotHistogramHovered]  = hexToImVec4(0xCCCCCC);

        // --- Tables ---
        c[ImGuiCol_TableHeaderBg]         = hexToImVec4(0x1A1A1A);
        c[ImGuiCol_TableBorderStrong]   = hexToImVec4(0xFFFFFF, 0.60f);
        c[ImGuiCol_TableBorderLight]    = hexToImVec4(0xFFFFFF, 0.30f);
        c[ImGuiCol_TableRowBg]          = hexToImVec4(0x000000, 0.00f);
        c[ImGuiCol_TableRowBgAlt]       = hexToImVec4(0xFFFFFF, 0.04f);

        // --- Drag & Drop / Navigation ---
        c[ImGuiCol_DragDropTarget]        = hexToImVec4(0xFFFFFF);
        c[ImGuiCol_NavHighlight]          = hexToImVec4(0xFFFFFF);
        c[ImGuiCol_NavWindowingHighlight]= hexToImVec4(0xFFFFFF, 0.60f);
        c[ImGuiCol_NavWindowingDimBg]    = hexToImVec4(0x000000, 0.60f);
        c[ImGuiCol_ModalWindowDimBg]      = hexToImVec4(0x000000, 0.70f);

        // --- Layout & Rounding (sharp, utilitarian) ---
        style.WindowRounding     = 0.0f;
        style.ChildRounding      = 0.0f;
        style.FrameRounding      = 0.0f;
        style.PopupRounding      = 0.0f;
        style.ScrollbarRounding  = 0.0f;
        style.GrabRounding       = 0.0f;
        style.TabRounding        = 0.0f;
        style.LogSliderDeadzone  = 0.0f;

        style.WindowBorderSize   = 1.0f;
        style.ChildBorderSize    = 1.0f;
        style.PopupBorderSize    = 1.0f;
        style.FrameBorderSize    = 1.0f;
        style.TabBorderSize      = 1.0f;

        style.WindowPadding      = ImVec2(10.0f, 10.0f);
        style.FramePadding       = ImVec2(6.0f, 4.0f);
        style.CellPadding        = ImVec2(6.0f, 4.0f);
        style.ItemSpacing        = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing   = ImVec2(6.0f, 4.0f);
        style.IndentSpacing      = 24.0f;
        style.ScrollbarSize      = 14.0f;
        style.GrabMinSize        = 10.0f;
        style.SeparatorTextBorderSize = 2.0f;
        style.SeparatorTextPadding    = ImVec2(8.0f, 4.0f);
        style.SeparatorTextAlign    = ImVec2(0.0f, 0.5f);
    }

    inline void applyThemeNeon()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;

        // Core palette
        const ImVec4 NeonCyan  = hexToImVec4(0x00F0FF); // headers, checkmarks, nav
        const ImVec4 NeonPink  = hexToImVec4(0xFF2070); // buttons, primary actions
        const ImVec4 NeonLime  = hexToImVec4(0xB3FF00); // sliders, plots, accents
        const ImVec4 DarkBase  = hexToImVec4(0x0C0C18); // window background
        const ImVec4 DarkFrame = hexToImVec4(0x161630); // frames / panels
        const ImVec4 DarkHead  = hexToImVec4(0x1E1E48); // header backgrounds
        const ImVec4 DarkHov   = hexToImVec4(0x282860); // hovered backgrounds
        const ImVec4 DarkAct   = hexToImVec4(0x323280); // active backgrounds

        // --- Text ---
        c[ImGuiCol_Text]                  = hexToImVec4(0xE2E2E2);
        c[ImGuiCol_TextDisabled]          = hexToImVec4(0x606090);
        c[ImGuiCol_TextSelectedBg]        = hexToImVec4(0x00F0FF, 0.25f);

        // --- Backgrounds ---
        c[ImGuiCol_WindowBg]              = DarkBase;
        c[ImGuiCol_ChildBg]               = hexToImVec4(0x0F0F1E);
        c[ImGuiCol_PopupBg]               = hexToImVec4(0x121228);
        c[ImGuiCol_MenuBarBg]             = hexToImVec4(0x141438);

        // --- Borders ---
        c[ImGuiCol_Border]                = hexToImVec4(0x00F0FF, 0.25f);
        c[ImGuiCol_BorderShadow]          = hexToImVec4(0x00F0FF, 0.05f);

        // --- Frames ---
        c[ImGuiCol_FrameBg]               = DarkFrame;
        c[ImGuiCol_FrameBgHovered]        = hexToImVec4(0x202050);
        c[ImGuiCol_FrameBgActive]         = hexToImVec4(0x2A2A70);

        // --- Titles ---
        c[ImGuiCol_TitleBg]               = hexToImVec4(0x121230);
        c[ImGuiCol_TitleBgActive]         = hexToImVec4(0x1A1A50);
        c[ImGuiCol_TitleBgCollapsed]      = hexToImVec4(0x0E0E28);

        // --- Buttons (Hot Pink) ---
        c[ImGuiCol_Button]                = NeonPink;
        c[ImGuiCol_ButtonHovered]         = hexToImVec4(0xFF4088);
        c[ImGuiCol_ButtonActive]          = hexToImVec4(0xFF60A0);

        // --- Headers (Dark blue-purple; neon accents on active) ---
        c[ImGuiCol_Header]                = DarkHead;
        c[ImGuiCol_HeaderHovered]         = DarkHov;
        c[ImGuiCol_HeaderActive]          = DarkAct;

        // --- Scrollbar ---
        c[ImGuiCol_ScrollbarBg]           = hexToImVec4(0x0A0A1A);
        c[ImGuiCol_ScrollbarGrab]         = hexToImVec4(0x2A2A60);
        c[ImGuiCol_ScrollbarGrabHovered]  = hexToImVec4(0x3A3A90);
        c[ImGuiCol_ScrollbarGrabActive]   = NeonCyan;

        // --- Checkmarks & Sliders (Cyan + Lime) ---
        c[ImGuiCol_CheckMark]             = NeonCyan;
        c[ImGuiCol_SliderGrab]            = NeonLime;
        c[ImGuiCol_SliderGrabActive]      = hexToImVec4(0xCCFF44);

        // --- Separators ---
        c[ImGuiCol_Separator]             = hexToImVec4(0x2A2A60);
        c[ImGuiCol_SeparatorHovered]      = hexToImVec4(0x3A3A90);
        c[ImGuiCol_SeparatorActive]       = NeonCyan;

        // --- Resize Grip ---
        c[ImGuiCol_ResizeGrip]            = hexToImVec4(0x2A2A60);
        c[ImGuiCol_ResizeGripHovered]     = NeonLime;
        c[ImGuiCol_ResizeGripActive]      = NeonCyan;

        // --- Tabs (darker cyan active so white text stays readable) ---
        c[ImGuiCol_Tab]                   = hexToImVec4(0x1A1A40);
        c[ImGuiCol_TabHovered]            = hexToImVec4(0x2A2A70);
        c[ImGuiCol_TabActive]             = hexToImVec4(0x00AABB);
        c[ImGuiCol_TabUnfocused]          = hexToImVec4(0x151535);
        c[ImGuiCol_TabUnfocusedActive]    = hexToImVec4(0x008899);

        // --- Docking ---
        c[ImGuiCol_DockingPreview]        = hexToImVec4(0x00F0FF, 0.25f);
        c[ImGuiCol_DockingEmptyBg]        = hexToImVec4(0x080818);

        // --- Plots ---
        c[ImGuiCol_PlotLines]             = NeonCyan;
        c[ImGuiCol_PlotLinesHovered]      = NeonLime;
        c[ImGuiCol_PlotHistogram]         = NeonPink;
        c[ImGuiCol_PlotHistogramHovered]  = hexToImVec4(0xFF60A0);

        // --- Tables ---
        c[ImGuiCol_TableHeaderBg]         = hexToImVec4(0x181840);
        c[ImGuiCol_TableBorderStrong]     = hexToImVec4(0x2A2A70);
        c[ImGuiCol_TableBorderLight]      = hexToImVec4(0x202050);
        c[ImGuiCol_TableRowBg]            = hexToImVec4(0x000000, 0.00f);
        c[ImGuiCol_TableRowBgAlt]         = hexToImVec4(0x00F0FF, 0.04f);

        // --- Drag & Drop / Navigation ---
        c[ImGuiCol_DragDropTarget]        = NeonLime;
        c[ImGuiCol_NavHighlight]          = NeonCyan;
        c[ImGuiCol_NavWindowingHighlight] = NeonCyan;
        c[ImGuiCol_NavWindowingDimBg]     = hexToImVec4(0x000000, 0.60f);
        c[ImGuiCol_ModalWindowDimBg]      = hexToImVec4(0x000000, 0.70f);

        // --- Layout & Rounding (slight tech edge) ---
        style.WindowRounding     = 2.0f;
        style.ChildRounding      = 2.0f;
        style.FrameRounding      = 3.0f;
        style.PopupRounding      = 3.0f;
        style.ScrollbarRounding  = 3.0f;
        style.GrabRounding       = 3.0f;
        style.TabRounding        = 3.0f;
        style.LogSliderDeadzone  = 2.0f;

        style.WindowBorderSize   = 1.0f;
        style.ChildBorderSize    = 1.0f;
        style.PopupBorderSize    = 1.0f;
        style.FrameBorderSize    = 1.0f;
        style.TabBorderSize      = 1.0f;

        style.WindowPadding      = ImVec2(12.0f, 12.0f);
        style.FramePadding       = ImVec2(8.0f, 5.0f);
        style.CellPadding        = ImVec2(8.0f, 5.0f);
        style.ItemSpacing        = ImVec2(10.0f, 8.0f);
        style.ItemInnerSpacing   = ImVec2(8.0f, 6.0f);
        style.IndentSpacing      = 24.0f;
        style.ScrollbarSize      = 16.0f;
        style.GrabMinSize        = 12.0f;
        style.SeparatorTextBorderSize = 1.0f;
        style.SeparatorTextPadding    = ImVec2(10.0f, 4.0f);
        style.SeparatorTextAlign    = ImVec2(0.0f, 0.5f);
    }

    inline void applyThemeWarmNeutral()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;

        // Core palette
        const ImVec4 WarmText  = hexToImVec4(0x3E3226); // dark coffee (not pure black)
        const ImVec4 WarmBase  = hexToImVec4(0xF2EBE3); // warm off-white
        const ImVec4 WarmFrame = hexToImVec4(0xE6DDD2); // frame background
        const ImVec4 WarmHov   = hexToImVec4(0xD9CFC3); // hovered
        const ImVec4 WarmAct   = hexToImVec4(0xCCC1B5); // active
        const ImVec4 WarmBtn   = hexToImVec4(0xD4C8B8); // button
        const ImVec4 WarmBrd   = hexToImVec4(0xC4B7A6); // borders
        const ImVec4 Accent    = hexToImVec4(0xC07756); // terracotta

        // --- Text ---
        c[ImGuiCol_Text]                  = WarmText;
        c[ImGuiCol_TextDisabled]          = hexToImVec4(0xA89B8C);
        c[ImGuiCol_TextSelectedBg]        = hexToImVec4(0xC07756, 0.30f);

        // --- Backgrounds ---
        c[ImGuiCol_WindowBg]              = WarmBase;
        c[ImGuiCol_ChildBg]               = hexToImVec4(0xEDE6DD);
        c[ImGuiCol_PopupBg]               = hexToImVec4(0xFAF5F0);
        c[ImGuiCol_MenuBarBg]             = hexToImVec4(0xE0D6CA);

        // --- Borders ---
        c[ImGuiCol_Border]                = WarmBrd;
        c[ImGuiCol_BorderShadow]          = hexToImVec4(0x000000, 0.00f);

        // --- Frames ---
        c[ImGuiCol_FrameBg]               = WarmFrame;
        c[ImGuiCol_FrameBgHovered]        = WarmHov;
        c[ImGuiCol_FrameBgActive]         = WarmAct;

        // --- Titles ---
        c[ImGuiCol_TitleBg]               = WarmFrame;
        c[ImGuiCol_TitleBgActive]         = WarmHov;
        c[ImGuiCol_TitleBgCollapsed]      = WarmFrame;

        // --- Buttons ---
        c[ImGuiCol_Button]                = WarmBtn;
        c[ImGuiCol_ButtonHovered]         = hexToImVec4(0xC4B7A6);
        c[ImGuiCol_ButtonActive]          = hexToImVec4(0xB4A696);

        // --- Headers ---
        c[ImGuiCol_Header]                = hexToImVec4(0xC4B7A6);
        c[ImGuiCol_HeaderHovered]         = hexToImVec4(0xB8AA99);
        c[ImGuiCol_HeaderActive]          = hexToImVec4(0xAA9988);

        // --- Scrollbar ---
        c[ImGuiCol_ScrollbarBg]           = hexToImVec4(0xE0D6CA);
        c[ImGuiCol_ScrollbarGrab]         = hexToImVec4(0xC4B7A6);
        c[ImGuiCol_ScrollbarGrabHovered]  = hexToImVec4(0xB8AA99);
        c[ImGuiCol_ScrollbarGrabActive]   = Accent;

        // --- Checkmarks & Sliders (Terracotta accent) ---
        c[ImGuiCol_CheckMark]             = Accent;
        c[ImGuiCol_SliderGrab]            = Accent;
        c[ImGuiCol_SliderGrabActive]      = hexToImVec4(0xA86548);

        // --- Separators ---
        c[ImGuiCol_Separator]             = WarmBrd;
        c[ImGuiCol_SeparatorHovered]      = hexToImVec4(0xB8AA99);
        c[ImGuiCol_SeparatorActive]       = Accent;

        // --- Resize Grip ---
        c[ImGuiCol_ResizeGrip]            = WarmBrd;
        c[ImGuiCol_ResizeGripHovered]     = Accent;
        c[ImGuiCol_ResizeGripActive]      = hexToImVec4(0xA86548);

        // --- Tabs ---
        c[ImGuiCol_Tab]                   = hexToImVec4(0xDDD5C8);
        c[ImGuiCol_TabHovered]            = hexToImVec4(0xC4B7A6);
        c[ImGuiCol_TabActive]             = hexToImVec4(0xB8AA99);
        c[ImGuiCol_TabUnfocused]          = hexToImVec4(0xE6DDD2);
        c[ImGuiCol_TabUnfocusedActive]    = hexToImVec4(0xCCC1B5);

        // --- Docking ---
        c[ImGuiCol_DockingPreview]        = hexToImVec4(0xC07756, 0.30f);
        c[ImGuiCol_DockingEmptyBg]        = hexToImVec4(0xEDE6DD);

        // --- Plots ---
        c[ImGuiCol_PlotLines]             = hexToImVec4(0x8B7355);
        c[ImGuiCol_PlotLinesHovered]      = Accent;
        c[ImGuiCol_PlotHistogram]         = Accent;
        c[ImGuiCol_PlotHistogramHovered]  = hexToImVec4(0xA86548);

        // --- Tables ---
        c[ImGuiCol_TableHeaderBg]         = hexToImVec4(0xE0D6CA);
        c[ImGuiCol_TableBorderStrong]     = hexToImVec4(0xB8AA99);
        c[ImGuiCol_TableBorderLight]      = WarmBrd;
        c[ImGuiCol_TableRowBg]            = hexToImVec4(0x000000, 0.00f);
        c[ImGuiCol_TableRowBgAlt]         = hexToImVec4(0xC07756, 0.06f);

        // --- Drag & Drop / Navigation ---
        c[ImGuiCol_DragDropTarget]        = Accent;
        c[ImGuiCol_NavHighlight]          = Accent;
        c[ImGuiCol_NavWindowingHighlight] = Accent;
        c[ImGuiCol_NavWindowingDimBg]     = hexToImVec4(0x3E3226, 0.30f);
        c[ImGuiCol_ModalWindowDimBg]      = hexToImVec4(0x3E3226, 0.40f);

        // --- Layout & Rounding (soft & friendly) ---
        style.WindowRounding     = 4.0f;
        style.ChildRounding      = 4.0f;
        style.FrameRounding      = 4.0f;
        style.PopupRounding      = 4.0f;
        style.ScrollbarRounding  = 4.0f;
        style.GrabRounding       = 4.0f;
        style.TabRounding        = 4.0f;
        style.LogSliderDeadzone  = 2.0f;

        style.WindowBorderSize   = 1.0f;
        style.ChildBorderSize    = 1.0f;
        style.PopupBorderSize    = 1.0f;
        style.FrameBorderSize    = 0.0f; // softer without frame borders
        style.TabBorderSize      = 1.0f;

        style.WindowPadding      = ImVec2(12.0f, 12.0f);
        style.FramePadding       = ImVec2(8.0f, 5.0f);
        style.CellPadding        = ImVec2(8.0f, 5.0f);
        style.ItemSpacing        = ImVec2(10.0f, 8.0f);
        style.ItemInnerSpacing   = ImVec2(8.0f, 6.0f);
        style.IndentSpacing      = 24.0f;
        style.ScrollbarSize      = 16.0f;
        style.GrabMinSize        = 12.0f;
        style.SeparatorTextBorderSize = 1.0f;
        style.SeparatorTextPadding    = ImVec2(10.0f, 4.0f);
        style.SeparatorTextAlign    = ImVec2(0.0f, 0.5f);
    }

    inline void applyImGuiTheme(ImGuiTheme theme) {
        switch (theme) {
            case ImGuiTheme::HighContrast:  applyThemeHighContrast();  break;
            case ImGuiTheme::Neon:          applyThemeNeon();        break;
            case ImGuiTheme::WarmNeutral:   applyThemeWarmNeutral(); break;
            default:                        applyThemeWarmNeutral(); break;
        }
    }
}