#pragma once
#include "IMGUI/imgui.h"
#include "IMGUI/imgui_internal.h"
#include <string>
#include <functional>

namespace UI {
    namespace Modern {

        // Theme Constants
        namespace Theme {
            // Colors
            constexpr ImU32 Col_Background = IM_COL32(25, 25, 30, 255);
            constexpr ImU32 Col_CardBg = IM_COL32(35, 35, 40, 255);
            constexpr ImU32 Col_CardBgHover = IM_COL32(45, 45, 50, 255);
            constexpr ImU32 Col_Accent = IM_COL32(0, 180, 255, 255); // Cyan Neon
            constexpr ImU32 Col_AccentHover = IM_COL32(50, 200, 255, 255);
            constexpr ImU32 Col_Text = IM_COL32(240, 240, 240, 255);
            constexpr ImU32 Col_TextDisabled = IM_COL32(150, 150, 150, 255);
            constexpr ImU32 Col_Success = IM_COL32(50, 200, 100, 255);
            constexpr ImU32 Col_Warning = IM_COL32(255, 180, 0, 255);
            constexpr ImU32 Col_Error = IM_COL32(255, 80, 80, 255);

            // Metrics
            constexpr float Radius_Small = 4.0f;
            constexpr float Radius_Medium = 8.0f;
            constexpr float Radius_Large = 12.0f;
            constexpr float Padding = 10.0f;
        }

        // --- Vector Icons ---
        // Drawn procedurally with ImDrawList for perfect scaling
        enum class IconType {
            None,       // No icon
            Search,
            Settings,
            Play,
            Stop,
            Shuffle,
            Delete,
            Check,
            Cross,
            Refresh,
            Filter
        };

        // --- Widgets ---

        // A primary action button with an icon and text
        bool PrimaryButton(const char* id, const char* label, IconType icon, const ImVec2& size = ImVec2(0, 0));

        // A secondary/ghost button
        bool SecondaryButton(const char* id, const char* label, IconType icon, const ImVec2& size = ImVec2(0, 0));

        // A small icon-only button (good for toolbars)
        bool IconButton(const char* id, IconType icon, const ImVec2& size = ImVec2(32, 32), bool active = false);

        // A modern toggle switch (replaces checkbox)
        bool Toggle(const char* id, bool* v, const char* label = nullptr);

        // A high-contrast checkbox with visible border when unchecked
        bool Checkbox(const char* label, bool* v);

        // A toggle chip/pill for state indicators (e.g., "In Casual", "In Tournament")
        bool StateChip(const char* label, bool* active);

        // A styled card container
        void BeginCard(const char* id, const ImVec2& size = ImVec2(0, 0));
        void EndCard();

        // A colored status badge
        void StatusBadge(const char* text, ImU32 color);

        // A modern search bar
        bool SearchBar(const char* id, char* buffer, size_t bufferSize, const char* hint = "Search...");

        // Layout Helpers
        void PushFontLarge();
        void PopFont();

    } // namespace Modern
} // namespace UI
