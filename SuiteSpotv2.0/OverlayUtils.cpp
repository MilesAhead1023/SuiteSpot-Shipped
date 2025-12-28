#include "pch.h"
#include "OverlayUtils.h"
#include "IMGUI/imgui_internal.h"
#include <cmath>
#include <limits>

void OverlayUtils::ReplaceVars(std::string& str, std::map<std::string, std::string>& vars) {
    size_t start_pos = 0;
    while ((start_pos = str.find("{{", start_pos)) != std::string::npos) {
        size_t end_pos = str.find("}}", start_pos);
        if (end_pos == std::string::npos) break;

        std::string key = str.substr(start_pos + 2, end_pos - start_pos - 2);
        std::string val = "";
        
        if (vars.find(key) != vars.end()) {
            val = vars[key];
        }

        str.replace(start_pos, end_pos - start_pos + 2, val);
        start_pos += val.length();
    }
}

float OverlayUtils::EvaluateExpression(const std::string& str, float total_size) {
    // Simple parser: "50%" -> total_size * 0.5
    if (str.find('%') != std::string::npos) {
        try {
            float pct = std::stof(str);
            return total_size * (pct / 100.0f);
        } catch (...) {
            return 0.0f;
        }
    }
    try {
        return std::stof(str);
    } catch (...) {
        return 0.0f;
    }
}

void OverlayUtils::ImRotateStart(ImDrawList* drawlist, int& start_idx) {
    start_idx = drawlist->VtxBuffer.Size;
}

void OverlayUtils::ImRotateEnd(ImDrawList* drawlist, float rad, int start_idx) {
    if (rad == 0.0f) return;

    // Calculate center of rotation based on vertices
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);
    auto& buf = drawlist->VtxBuffer;
    for (int i = start_idx; i < buf.Size; ++i) {
        l = ImMin(l, buf[i].pos);
        u = ImMax(u, buf[i].pos);
    }
    ImVec2 center = ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);

    float s = sin(rad), c = cos(rad);
    
    // Rotate all new vertices around the center
    for (int i = start_idx; i < buf.Size; ++i) {
        buf[i].pos = center + ImRotate(buf[i].pos - center, c, s);
    }
}
