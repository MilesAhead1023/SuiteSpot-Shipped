#pragma once
#include "IMGUI/imgui.h"
#include <string>
#include <vector>

// Represents a single color state (Enabled + Value)
struct RSColor {
    bool enable = false;
    ImU32 color = IM_COL32_WHITE;
};

// Represents a graphical element (Text, Image, Shape)
struct RSElement {
    std::string type;       // "text", "image", "rectangle", "line"
    std::string value;      // Text content or Image path (e.g. "{{Score}}")
    std::string font;       // Font name (if custom)
    
    std::vector<ImVec2> positions; // Calculated screen positions
    ImVec2 size;            // Calculated size
    
    RSColor color;          // Text/Tint color
    RSColor fill;           // Background fill
    RSColor stroke;         // Border stroke
    
    float scale = 1.f;
    bool rotate_enable = false;
    float rotate = 0.f;     // Rotation in radians

    // Dynamic Lists
    std::string repeat_mode; // "players" or empty
    std::string team_filter; // "my_team", "opp_team", or empty
};

// Context for calculating positions (Screen size, Global Offset)
struct RSOptions {
    float x;                // Global X Offset
    float y;                // Global Y Offset
    float width;            // Screen Width
    float height;           // Screen Height
    float scale;            // Global Scale
    float opacity;          // Global Opacity
};
