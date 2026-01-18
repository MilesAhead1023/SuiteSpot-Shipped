#pragma once
#include "IMGUI/imgui.h"
#include "StatusMessageUI.h"
#include <string>
#include <vector>

/*
 * ======================================================================================
 * SETTINGS UI: THE CONFIGURATION MENU
 * ======================================================================================
 * 
 * WHAT IS THIS?
 * This class builds the visual menu you see when you press F2 in Rocket League and
 * click on the "SuiteSpot" tab.
 * 
 * WHY IS IT HERE?
 * Users need a way to change settings (like delay times or which map to load).
 * This class translates those settings into buttons, sliders, and checkboxes.
 * 
 * HOW DOES IT WORK?
 * 1. `RenderMainSettingsWindow()`: This is the main loop. It draws the tabs ("Main Settings",
 *    "Auto-Queue", etc.).
 * 2. It reads the current values from `SuiteSpot` (which gets them from `SettingsSync`).
 * 3. When you change a value (like sliding a slider), it immediately tells `SettingsSync`
 *    to save that new value to the config file.
 * 4. It uses `UI::Helpers` to make the code cleaner (so we don't have to write 
 *    "Draw Slider" logic 50 times).
 */

class SuiteSpot;

class SettingsUI {
public:
    explicit SettingsUI(SuiteSpot* plugin);
    void RenderMainSettingsWindow();

private:
    SuiteSpot* plugin_;
    UI::StatusMessage statusMessage; // Shows "Success!" or error messages

    // Internal helpers to draw specific tabs
    // These break the big menu into smaller, manageable chunks
    void RenderGeneralTab(bool& enabledValue, int& mapTypeValue);
    void RenderMapSelectionTab(int mapTypeValue, bool bagRotationEnabledValue, int& currentIndexValue, int& currentTrainingIndexValue, int& currentWorkshopIndexValue, int& delayFreeplaySecValue, int& delayTrainingSecValue, int& delayWorkshopSecValue, int& delayQueueSecValue);
    void RenderFreeplayMode(int& currentIndexValue);
    void RenderTrainingMode(bool bagRotationEnabledValue, int& currentTrainingIndexValue);
    void RenderWorkshopMode(int& currentWorkshopIndexValue);

    // Workshop path configuration state
    char workshopPathBuf[512] = {0};
    bool workshopPathInit = false;
    std::string workshopPathCache = "";
};
