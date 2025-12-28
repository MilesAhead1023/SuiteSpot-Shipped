#pragma once
#include "IMGUI/imgui.h"
#include <string>
#include <vector>

class SuiteSpot;

class LoadoutUI {
public:
    explicit LoadoutUI(SuiteSpot* plugin);
    void RenderLoadoutControls();

private:
    SuiteSpot* plugin_;

    std::string loadoutStatusText;
    ImVec4 loadoutStatusColor = ImVec4(0, 0, 0, 0);
    float loadoutStatusTimer = 0.0f;
    std::string currentLoadoutName;
    std::vector<std::string> loadoutNames;
    int selectedLoadoutIndex = 0;
    bool loadoutsInitialized = false;
};
