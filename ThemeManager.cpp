#include "pch.h"
#include "ThemeManager.h"

namespace UI {
namespace Theme {

// Internal state for style editor toggle
static bool styleEditorVisible = false;

void ApplyModernTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // ===================================================================
    // COLORS: Full palette application
    // ===================================================================
    
    ImVec4* colors = style.Colors;
    
    // Window backgrounds
    colors[ImGuiCol_WindowBg]        = Colors::BG_WINDOW;
    colors[ImGuiCol_ChildBg]         = Colors::BG_CHILD;
    colors[ImGuiCol_PopupBg]         = Colors::BG_POPUP;
    
    // Frame backgrounds (buttons, inputs, sliders, etc.)
    colors[ImGuiCol_FrameBg]         = Colors::SURFACE_DEFAULT;
    colors[ImGuiCol_FrameBgHovered]  = Colors::SURFACE_HOVER;
    colors[ImGuiCol_FrameBgActive]   = Colors::SURFACE_ACTIVE;
    
    // Title bar
    colors[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);  // Slightly darker than window
    colors[ImGuiCol_TitleBgActive]   = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);  // Matches window bg
    colors[ImGuiCol_TitleBgCollapsed]= ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    
    // Menu bar
    colors[ImGuiCol_MenuBarBg]       = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    
    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]     = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]   = Colors::SCROLLBAR;
    colors[ImGuiCol_ScrollbarGrabHovered] = Colors::SCROLLBAR_HOVER;
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.40f, 0.40f, 0.44f, 1.00f);
    
    // Checkboxes
    colors[ImGuiCol_CheckMark]       = Colors::ACCENT_PRIMARY;
    
    // Slider grab
    colors[ImGuiCol_SliderGrab]      = Colors::ACCENT_PRIMARY;
    colors[ImGuiCol_SliderGrabActive]= ImVec4(0.36f, 0.66f, 1.00f, 1.00f);  // Brighter blue
    
    // Buttons
    colors[ImGuiCol_Button]          = Colors::SURFACE_DEFAULT;
    colors[ImGuiCol_ButtonHovered]   = Colors::SURFACE_HOVER;
    colors[ImGuiCol_ButtonActive]    = Colors::SURFACE_ACTIVE;
    
    // Headers (collapsing headers, tree nodes)
    colors[ImGuiCol_Header]          = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.26f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    
    // Separators
    colors[ImGuiCol_Separator]       = Colors::SEPARATOR;
    colors[ImGuiCol_SeparatorHovered]= ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = Colors::ACCENT_PRIMARY;
    
    // Resize grip
    colors[ImGuiCol_ResizeGrip]      = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.32f, 0.32f, 0.36f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]  = Colors::ACCENT_PRIMARY;
    
    // Tabs
    colors[ImGuiCol_Tab]             = Colors::TAB_INACTIVE;
    colors[ImGuiCol_TabHovered]      = ImVec4(0.26f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_TabActive]       = Colors::TAB_ACTIVE;
    colors[ImGuiCol_TabUnfocused]    = Colors::TAB_INACTIVE;
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    
    // Text
    colors[ImGuiCol_Text]            = Colors::TEXT_PRIMARY;
    colors[ImGuiCol_TextDisabled]    = Colors::TEXT_DISABLED;
    colors[ImGuiCol_TextSelectedBg]  = Colors::SELECTED_BG;
    
    // Borders
    colors[ImGuiCol_Border]          = Colors::BORDER_DEFAULT;
    colors[ImGuiCol_BorderShadow]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);  // No shadow
    
    // Misc
    colors[ImGuiCol_NavHighlight]    = Colors::ACCENT_PRIMARY;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    
    // Docking (if used in future)
    colors[ImGuiCol_DockingPreview]  = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_DockingEmptyBg]  = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    
    // Plot colors (for potential future graphs)
    colors[ImGuiCol_PlotLines]       = Colors::ACCENT_PRIMARY;
    colors[ImGuiCol_PlotLinesHovered]= ImVec4(0.36f, 0.66f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]   = Colors::ACCENT_SUCCESS;
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.50f, 0.83f, 0.52f, 1.00f);
    
    // Table colors
    colors[ImGuiCol_TableHeaderBg]   = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderLight]  = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    colors[ImGuiCol_TableRowBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]   = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    
    // ===================================================================
    // SPACING: Generous padding and margins
    // ===================================================================
    
    style.WindowPadding      = Spacing::WINDOW_PADDING;
    style.FramePadding       = Spacing::FRAME_PADDING;
    style.ItemSpacing        = Spacing::ITEM_SPACING;
    style.ItemInnerSpacing   = Spacing::ITEM_INNER_SPACING;
    style.IndentSpacing      = Spacing::INDENT_SPACING;
    style.ScrollbarSize      = Spacing::SCROLLBAR_SIZE;
    
    // Touch extra padding (for mobile/controller - not used but set for completeness)
    style.TouchExtraPadding  = ImVec2(0.0f, 0.0f);
    
    // ===================================================================
    // ROUNDING: Modern soft corners
    // ===================================================================
    
    style.WindowRounding     = Rounding::WINDOW;
    style.ChildRounding      = Rounding::CHILD;
    style.FrameRounding      = Rounding::FRAME;
    style.PopupRounding      = Rounding::POPUP;
    style.ScrollbarRounding  = Rounding::SCROLLBAR;
    style.GrabRounding       = Rounding::FRAME;      // Match frame rounding for sliders
    style.TabRounding        = Rounding::TAB;
    
    // ===================================================================
    // BORDERS: Subtle definition
    // ===================================================================
    
    style.WindowBorderSize   = Borders::WINDOW;
    style.ChildBorderSize    = 1.0f;
    style.PopupBorderSize    = 1.0f;
    style.FrameBorderSize    = Borders::FRAME;
    style.TabBorderSize      = 0.0f;  // Tabs don't need borders
    
    // ===================================================================
    // ALIGNMENT & MISC
    // ===================================================================
    
    // Button text alignment (centered by default)
    style.ButtonTextAlign    = ImVec2(0.5f, 0.5f);
    
    // Selectable text alignment
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);  // Left-aligned
    
    // Window title alignment
    style.WindowTitleAlign   = ImVec2(0.0f, 0.5f);  // Left-aligned, vertically centered
    
    // Window menu button position
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    
    // Color button position in color pickers
    style.ColorButtonPosition = ImGuiDir_Right;
    
    // Anti-aliasing
    style.AntiAliasedLines   = true;
    style.AntiAliasedFill    = true;
    
    // Alpha (global transparency - keep at 1.0 for full opacity)
    style.Alpha              = 1.0f;
    
    // Disabled alpha (how transparent disabled elements appear)
    style.DisabledAlpha      = 0.50f;
}

void ResetToDefaultTheme() {
    // Restore ImGui's default dark theme
    ImGui::StyleColorsDark();
}

bool ToggleStyleEditor() {
    styleEditorVisible = !styleEditorVisible;
    return styleEditorVisible;
}

void RenderStyleEditor() {
    if (styleEditorVisible) {
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("SuiteSpot Style Editor", &styleEditorVisible)) {
            ImGui::TextWrapped("Use this editor to tweak the theme in real-time. Changes are not saved.");
            ImGui::Separator();
            ImGui::Spacing();
            
            // Show the built-in ImGui style editor
            ImGui::ShowStyleEditor();
        }
        ImGui::End();
    }
}

bool IsStyleEditorVisible() {
    return styleEditorVisible;
}

} // namespace Theme
} // namespace UI
