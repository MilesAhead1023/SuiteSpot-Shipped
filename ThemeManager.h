#pragma once

/*
 * ======================================================================================
 * THEME MANAGER: MODERN UI STYLING FOR SUITESPOT
 * ======================================================================================
 * 
 * WHAT IS THIS?
 * This module provides a centralized, professional theming system for all SuiteSpot
 * ImGui windows. It replaces the default ImGui dark theme with a carefully crafted
 * modern design featuring proper color hierarchy, spacing, and visual polish.
 * 
 * WHY IS IT HERE?
 * The default ImGui theme is functional but bland. Professional applications need
 * consistent, modern styling with proper visual hierarchy, readable text, and
 * pleasant interactive elements. This theme manager provides that polish.
 * 
 * HOW DOES IT WORK?
 * 1. Call ApplyModernTheme() once during plugin initialization (onLoad)
 * 2. Optionally call ToggleStyleEditor() to enable runtime theme tuning (F12)
 * 3. All ImGui windows automatically inherit the new theme
 * 
 * DESIGN PHILOSOPHY:
 * - Dark theme optimized for gaming environment (reduces eye strain)
 * - High contrast ratios for text readability (WCAG AA compliant)
 * - Subtle depth cues (borders, rounding) without overwhelming the user
 * - Generous spacing for modern, breathable UI
 * - Accent colors that complement Rocket League's blue/orange palette
 */

#include "IMGUI/imgui.h"

namespace UI {
namespace Theme {

// ===================================================================
// COLOR PALETTE: Modern Dark Theme
// ===================================================================
// These colors are carefully chosen for visual hierarchy and accessibility

namespace Colors {
    // Background Hierarchy (darkest → lightest)
    constexpr ImVec4 BG_WINDOW        = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);  // Main window background
    constexpr ImVec4 BG_CHILD         = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);  // Child windows, panels
    constexpr ImVec4 BG_POPUP         = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);  // Popups, modals
    
    // Surface Colors (interactive elements)
    constexpr ImVec4 SURFACE_DEFAULT  = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);  // Buttons, inputs (normal)
    constexpr ImVec4 SURFACE_HOVER    = ImVec4(0.24f, 0.24f, 0.27f, 1.00f);  // Hover state
    constexpr ImVec4 SURFACE_ACTIVE   = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);  // Active/pressed state
    
    // Accent Colors (semantic meaning)
    constexpr ImVec4 ACCENT_PRIMARY   = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);  // Blue (active, selected, links)
    constexpr ImVec4 ACCENT_SUCCESS   = ImVec4(0.40f, 0.73f, 0.42f, 1.00f);  // Green (success, confirm)
    constexpr ImVec4 ACCENT_WARNING   = ImVec4(0.98f, 0.77f, 0.26f, 1.00f);  // Yellow (warning, caution)
    constexpr ImVec4 ACCENT_ERROR     = ImVec4(0.90f, 0.27f, 0.27f, 1.00f);  // Red (error, destructive)
    
    // Text Hierarchy (brightest → dimmest)
    constexpr ImVec4 TEXT_PRIMARY     = ImVec4(0.95f, 0.95f, 0.96f, 1.00f);  // Main body text
    constexpr ImVec4 TEXT_SECONDARY   = ImVec4(0.70f, 0.70f, 0.72f, 1.00f);  // Subtext, labels
    constexpr ImVec4 TEXT_DISABLED    = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);  // Disabled state
    constexpr ImVec4 TEXT_HEADER      = ImVec4(0.82f, 0.88f, 0.95f, 1.00f);  // Section headers (blue tint)
    
    // Borders & Separators
    constexpr ImVec4 BORDER_DEFAULT   = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);  // Subtle outlines
    constexpr ImVec4 BORDER_ACTIVE    = ImVec4(0.42f, 0.42f, 0.47f, 1.00f);  // Active element border
    constexpr ImVec4 SEPARATOR        = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);  // Section dividers
    
    // Special States
    constexpr ImVec4 SELECTED_BG      = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);  // Selected item background (translucent)
    constexpr ImVec4 TAB_ACTIVE       = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);  // Active tab
    constexpr ImVec4 TAB_INACTIVE     = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);  // Inactive tab
    constexpr ImVec4 SCROLLBAR        = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);  // Scrollbar grab
    constexpr ImVec4 SCROLLBAR_HOVER  = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);  // Scrollbar hover
}

// ===================================================================
// SPACING TOKENS: Generous Padding for Modern Look
// ===================================================================

namespace Spacing {
    // Frame Padding (inside buttons, inputs)
    constexpr ImVec2 FRAME_PADDING         = ImVec2(12.0f, 6.0f);  // Increased from (4, 3)
    
    // Item Spacing (between UI elements)
    constexpr ImVec2 ITEM_SPACING          = ImVec2(12.0f, 6.0f);  // Increased from (8, 4)
    constexpr ImVec2 ITEM_INNER_SPACING    = ImVec2(8.0f, 4.0f);   // Inside composite widgets
    
    // Indentation
    constexpr float INDENT_SPACING         = 24.0f;                // Tree/list indent (was 21)
    
    // Scrollbar
    constexpr float SCROLLBAR_SIZE         = 16.0f;                // Scrollbar width
    
    // Window Padding
    constexpr ImVec2 WINDOW_PADDING        = ImVec2(12.0f, 12.0f);
}

// ===================================================================
// ROUNDING: Soft Corners for Modern Aesthetics
// ===================================================================

namespace Rounding {
    constexpr float WINDOW      = 8.0f;   // Window corners
    constexpr float FRAME       = 4.0f;   // Buttons, inputs, sliders
    constexpr float POPUP       = 6.0f;   // Popup windows
    constexpr float SCROLLBAR   = 8.0f;   // Scrollbar ends (pill shape)
    constexpr float TAB         = 4.0f;   // Tab corners
    constexpr float CHILD       = 4.0f;   // Child windows
}

// ===================================================================
// BORDERS: Subtle Definition
// ===================================================================

namespace Borders {
    constexpr float WINDOW      = 1.0f;   // Window border
    constexpr float FRAME       = 1.0f;   // Button/input border
}

// ===================================================================
// PUBLIC API
// ===================================================================

//
// ApplyModernTheme - Apply the complete SuiteSpot theme to ImGui
//
// Call this once during plugin initialization (SuiteSpot::onLoad).
// It sets all ImGui style values (colors, spacing, rounding) to create
// a cohesive, modern appearance across all windows.
//
// Example:
//   void SuiteSpot::onLoad() {
//       UI::Theme::ApplyModernTheme();
//       // ... rest of initialization
//   }
//
void ApplyModernTheme();

//
// ResetToDefaultTheme - Restore ImGui's default StyleColorsDark theme
//
// Useful for debugging or user preference. Reverts all custom styling.
//
void ResetToDefaultTheme();

//
// ToggleStyleEditor - Show/hide the ImGui style editor window
//
// The style editor is a powerful tool for tuning colors and spacing in real-time.
// Toggle it with F12 key for developer use. NOT meant for end users.
//
// Returns: true if the editor is now visible, false if hidden
//
bool ToggleStyleEditor();

//
// RenderStyleEditor - Draw the style editor window (if enabled)
//
// Call this in your Render() loop AFTER toggling is enabled.
// The editor window allows live tweaking of all theme values.
//
// Example in SuiteSpot::Render():
//   UI::Theme::RenderStyleEditor();
//
void RenderStyleEditor();

//
// IsStyleEditorVisible - Check if the style editor is currently shown
//
bool IsStyleEditorVisible();

} // namespace Theme
} // namespace UI
