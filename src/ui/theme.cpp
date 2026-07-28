#include "theme.h"

#include "imgui.h"

#include "ui_utils.h"

namespace ml::ui {
    void applyThemeHighContrast()
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

    void applyThemeNeon()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;

        const ImVec4 TokyoCyan  = hexToImVec4(0x7DCFFF);
        const ImVec4 TokyoPurp  = hexToImVec4(0xBB9AF7);
        const ImVec4 TokyoGreen = hexToImVec4(0x9ECE6A);
        const ImVec4 DarkBase   = hexToImVec4(0x1A1B26);
        const ImVec4 DarkFrame  = hexToImVec4(0x24283B);
        const ImVec4 DarkHead   = hexToImVec4(0x1F2335);
        const ImVec4 DarkHov    = hexToImVec4(0x292E42);
        const ImVec4 DarkAct    = hexToImVec4(0x3B4261);

        c[ImGuiCol_Text]                  = hexToImVec4(0xC0CAF5);
        c[ImGuiCol_TextDisabled]          = hexToImVec4(0x565F89);
        c[ImGuiCol_TextSelectedBg]        = hexToImVec4(0x7AA2F7, 0.25f);

        c[ImGuiCol_WindowBg]              = DarkBase;
        c[ImGuiCol_ChildBg]               = hexToImVec4(0x16161E);
        c[ImGuiCol_PopupBg]               = hexToImVec4(0x1F2335);
        c[ImGuiCol_MenuBarBg]             = hexToImVec4(0x16161E);

        c[ImGuiCol_Border]                = hexToImVec4(0x7AA2F7, 0.25f);
        c[ImGuiCol_BorderShadow]          = hexToImVec4(0x7AA2F7, 0.05f);

        c[ImGuiCol_FrameBg]               = DarkFrame;
        c[ImGuiCol_FrameBgHovered]        = hexToImVec4(0x292E42);
        c[ImGuiCol_FrameBgActive]         = hexToImVec4(0x3B4261);

        c[ImGuiCol_TitleBg]               = hexToImVec4(0x16161E);
        c[ImGuiCol_TitleBgActive]         = hexToImVec4(0x1F2335);
        c[ImGuiCol_TitleBgCollapsed]      = hexToImVec4(0x13131A);

        c[ImGuiCol_Button]                = hexToImVec4(0x2F3549);
        c[ImGuiCol_ButtonHovered]         = hexToImVec4(0x3B4261);
        c[ImGuiCol_ButtonActive]          = hexToImVec4(0x565F89);

        c[ImGuiCol_Header]                = DarkHead;
        c[ImGuiCol_HeaderHovered]         = DarkHov;
        c[ImGuiCol_HeaderActive]          = DarkAct;

        c[ImGuiCol_ScrollbarBg]           = hexToImVec4(0x13131A);
        c[ImGuiCol_ScrollbarGrab]         = hexToImVec4(0x3B4261);
        c[ImGuiCol_ScrollbarGrabHovered]  = hexToImVec4(0x565F89);
        c[ImGuiCol_ScrollbarGrabActive]   = TokyoCyan;

        c[ImGuiCol_CheckMark]             = TokyoCyan;
        c[ImGuiCol_SliderGrab]            = hexToImVec4(0x7AA2F7);
        c[ImGuiCol_SliderGrabActive]      = hexToImVec4(0x8DB0F9);

        c[ImGuiCol_Separator]             = hexToImVec4(0x3B4261);
        c[ImGuiCol_SeparatorHovered]      = hexToImVec4(0x565F89);
        c[ImGuiCol_SeparatorActive]       = TokyoCyan;

        c[ImGuiCol_ResizeGrip]            = hexToImVec4(0x3B4261);
        c[ImGuiCol_ResizeGripHovered]     = TokyoGreen;
        c[ImGuiCol_ResizeGripActive]      = TokyoCyan;

        c[ImGuiCol_Tab]                   = hexToImVec4(0x16161E);
        c[ImGuiCol_TabHovered]            = hexToImVec4(0x1F2335);
        c[ImGuiCol_TabActive]             = hexToImVec4(0x24283B);
        c[ImGuiCol_TabUnfocused]          = hexToImVec4(0x13131A);
        c[ImGuiCol_TabUnfocusedActive]    = hexToImVec4(0x1F2335);

        c[ImGuiCol_DockingPreview]        = hexToImVec4(0x7DCFFF, 0.25f);
        c[ImGuiCol_DockingEmptyBg]        = hexToImVec4(0x13131A);

        c[ImGuiCol_PlotLines]             = TokyoCyan;
        c[ImGuiCol_PlotLinesHovered]      = TokyoGreen;
        c[ImGuiCol_PlotHistogram]         = TokyoPurp;
        c[ImGuiCol_PlotHistogramHovered]  = hexToImVec4(0xC7A9F9);

        c[ImGuiCol_TableHeaderBg]         = hexToImVec4(0x1F2335);
        c[ImGuiCol_TableBorderStrong]     = hexToImVec4(0x3B4261);
        c[ImGuiCol_TableBorderLight]      = hexToImVec4(0x292E42);
        c[ImGuiCol_TableRowBg]            = hexToImVec4(0x000000, 0.00f);
        c[ImGuiCol_TableRowBgAlt]         = hexToImVec4(0x7DCFFF, 0.04f);

        c[ImGuiCol_DragDropTarget]        = TokyoGreen;
        c[ImGuiCol_NavHighlight]          = TokyoCyan;
        c[ImGuiCol_NavWindowingHighlight] = TokyoCyan;
        c[ImGuiCol_NavWindowingDimBg]     = hexToImVec4(0x000000, 0.60f);
        c[ImGuiCol_ModalWindowDimBg]      = hexToImVec4(0x000000, 0.70f);

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

    void applyThemeWarmNeutral()
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

    void applyImGuiTheme(ImGuiTheme theme)
    {
        switch (theme) {
            case ImGuiTheme::HighContrast:  applyThemeHighContrast();  break;
            case ImGuiTheme::Neon:          applyThemeNeon();        break;
            case ImGuiTheme::WarmNeutral:   applyThemeWarmNeutral(); break;
            default:                        applyThemeWarmNeutral(); break;
        }
    }
}
