#pragma once
#include "OverlayTypes.h"
#include <functional>
#include <string>
#include <map>

class OverlayUtils {
public:
    // Replaces {{Key}} with Value from the map
    static void ReplaceVars(std::string& str, std::map<std::string, std::string>& vars);

    // Evaluates math expressions (e.g., "50% - 20") for responsive positioning
    // Simple implementation: supports % (percentage of screen) and raw pixels
    static float EvaluateExpression(const std::string& str, float total_size);

    // Rotation Helper (from RocketStats/imgui_rotate)
    static void ImRotateStart(ImDrawList* drawlist, int& start_idx);
    static void ImRotateEnd(ImDrawList* drawlist, float rad, int start_idx);
};
