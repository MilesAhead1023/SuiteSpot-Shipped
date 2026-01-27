#include "pch.h"

#include "SettingsUI.h"
#include "LoadoutUI.h"
#include "TrainingPackUI.h"
#include "SuiteSpot.h"
#include "MapManager.h"
#include "TrainingPackManager.h"
#include "SettingsSync.h"
#include "ConstantsUI.h"
#include "HelpersUI.h"
#include "DefaultPacks.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>

SettingsUI::SettingsUI(SuiteSpot* plugin) : plugin_(plugin) {}

void SettingsUI::RenderMainSettingsWindow() {
    if (!plugin_) {
        return;
    }

    ImGui::SetWindowFontScale(UI::FONT_SCALE);

    // Header with metadata and Load Now button
    ImGui::BeginGroup();
    ImGui::TextColored(UI::SettingsUI::HEADER_TEXT_COLOR, "By: Flicks Creations");
    std::string ver = "Version: " + std::string(plugin_version);
    ImGui::TextColored(UI::SettingsUI::HEADER_TEXT_COLOR, "%s", ver.c_str());
    ImGui::EndGroup();

    ImGui::SameLine(ImGui::GetWindowWidth() - 150.0f);
    if (ImGui::Button("LOAD NOW", ImVec2(130, 26))) {
        int mapType = plugin_->GetMapType();
        SuiteSpot* p = plugin_;

        if (mapType == 0) { // Freeplay
            std::string code = p->GetCurrentFreeplayCode();
            if (!code.empty()) {
                p->gameWrapper->SetTimeout([p, code](GameWrapper* gw) {
                    p->cvarManager->executeCommand("load_freeplay " + code);
                }, 0.0f);
                statusMessage.ShowSuccess("Loading Freeplay", 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
            }
        } else if (mapType == 1) { // Training
            int trainingMode = p->settingsSync->GetTrainingMode();
            std::string code;
            
            // Bag rotation removed - always use single pack mode
            code = p->settingsSync->GetQuickPicksSelectedCode();
            if (code.empty()) code = p->GetCurrentTrainingCode();

            if (!code.empty()) {
                if (p->usageTracker) p->usageTracker->IncrementLoadCount(code);
                p->gameWrapper->SetTimeout([p, code](GameWrapper* gw) {
                    p->cvarManager->executeCommand("load_training " + code);
                }, 0.0f);
                statusMessage.ShowSuccess("Loading Training Pack", 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
            }
        } else if (mapType == 2) { // Workshop
            std::string path = p->GetCurrentWorkshopPath();
            if (!path.empty()) {
                p->gameWrapper->SetTimeout([p, path](GameWrapper* gw) {
                    p->cvarManager->executeCommand("load_workshop \"" + path + "\"");
                }, 0.0f);
                statusMessage.ShowSuccess("Loading Workshop Map", 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
            }
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Immediately load the currently selected map/pack");
    }

    ImGui::Spacing();
    statusMessage.Render(ImGui::GetIO().DeltaTime);
    if (statusMessage.IsVisible()) ImGui::Spacing();

    bool enabledValue = plugin_->IsEnabled();
    int mapTypeValue = plugin_->GetMapType();
    bool autoQueueValue = plugin_->IsAutoQueueEnabled();
    // Bag rotation removed
    int trainingModeValue = plugin_->settingsSync->GetTrainingMode();
    int delayQueueSecValue = plugin_->GetDelayQueueSec();
    int delayFreeplaySecValue = plugin_->GetDelayFreeplaySec();
    int delayTrainingSecValue = plugin_->GetDelayTrainingSec();
    int delayWorkshopSecValue = plugin_->GetDelayWorkshopSec();
    std::string currentFreeplayCode = plugin_->GetCurrentFreeplayCode();
    std::string currentTrainingCode = plugin_->GetCurrentTrainingCode();
    std::string quickPicksSelectedCode = plugin_->settingsSync->GetQuickPicksSelectedCode();
    std::string currentWorkshopPath = plugin_->GetCurrentWorkshopPath();

    // Only show status if enabled
    if (enabledValue) {
        ImGui::Separator();

        const char* modeNames[] = {"Freeplay", "Training", "Workshop"};
        std::string currentMap = "<none>";
        std::string queueDelayStr = std::to_string(delayQueueSecValue) + "s";
        std::string mapDelayStr = "0s";
        const ImVec4 white = UI::SettingsUI::STATUS_SEPARATOR_COLOR;

        // Get current selection and appropriate delay
        if (mapTypeValue == 0) {
            // Find freeplay map by code
            auto it = std::find_if(RLMaps.begin(), RLMaps.end(),
                [&](const MapEntry& e) { return e.code == currentFreeplayCode; });
            if (it != RLMaps.end()) {
                currentMap = it->name;
            }
            mapDelayStr = std::to_string(delayFreeplaySecValue) + "s";
        } else if (mapTypeValue == 1) {
            mapDelayStr = std::to_string(delayTrainingSecValue) + "s";

            // Get packs from manager for consistent lookup
            const auto& trainingPacks = plugin_->trainingPackMgr ?
                plugin_->trainingPackMgr->GetPacks() : RLTraining;

            // Bag rotation removed - show single pack mode
            std::string targetCode = quickPicksSelectedCode;
            if (targetCode.empty()) targetCode = currentTrainingCode;
            
            const TrainingEntry* targetPack = plugin_->trainingPackMgr->GetPackByCode(targetCode);
            if (targetPack) {
                currentMap = targetPack->name;
            } else if (!targetCode.empty()) {
                currentMap = targetCode + " (custom)";
            } else {
                currentMap = "<none selected>";
            }
            
            mapDelayStr = "with " + std::to_string(delayTrainingSecValue) + "s delay";
        } else if (mapTypeValue == 2) {
            // Find workshop map by path
            auto it = std::find_if(RLWorkshop.begin(), RLWorkshop.end(),
                [&](const WorkshopEntry& e) { return e.filePath == currentWorkshopPath; });
            if (it != RLWorkshop.end()) {
                currentMap = it->name;
            }
            mapDelayStr = std::to_string(delayWorkshopSecValue) + "s";
        }

        // Map mode status
        const ImVec4 green = UI::SettingsUI::STATUS_ENABLED_TEXT_COLOR;
        const ImVec4 red   = UI::SettingsUI::STATUS_DISABLED_TEXT_COLOR;
        std::string modeText = "Mode: " + std::string(modeNames[mapTypeValue]);
        
        int currentDelay = (mapTypeValue == 0) ? delayFreeplaySecValue : (mapTypeValue == 1 ? delayTrainingSecValue : delayWorkshopSecValue);
        if (currentDelay > 0) {
            modeText += " Delayed: " + mapDelayStr;
        }

        ImGui::TextColored(green, "%s", modeText.c_str());
        ImGui::SameLine();
        ImGui::TextColored(white, "|");
        ImGui::SameLine();
        ImGui::TextColored(green, "Map: %s", currentMap.c_str());
        ImGui::SameLine();
        ImGui::TextColored(white, "|");
        ImGui::SameLine();
        const ImVec4 queueColor = autoQueueValue ? green : red;
        if (delayQueueSecValue > 0) {
            ImGui::TextColored(queueColor, "Next Match Queue Delayed: %s", queueDelayStr.c_str());
        } else {
            ImGui::TextColored(queueColor, "Next Match Queue");
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 1) Global Controls
    // ...
    ImGui::Spacing();

    // 1) Global Controls (Enable/Disable + Map Mode) - above the tabs
    RenderGeneralTab(enabledValue, mapTypeValue);
    ImGui::Spacing();
    ImGui::Separator();

    // Main tab bar
    if (ImGui::BeginTabBar("SuiteSpotTabs", ImGuiTabBarFlags_None)) {

        // ===== MAP SELECT TAB =====
        if (ImGui::BeginTabItem("Map Select")) {
            ImGui::Spacing();

            // 1) Unified Header: Auto-Queue Toggle | Queue Delay | Map Delay
            ImGui::Columns(2, "MapSelectHeaderCols", false);
            ImGui::SetColumnWidth(0, 150.0f);
            
            // Auto-Queue
            UI::Helpers::CheckboxWithCVar("Auto-Queue", autoQueueValue, "suitespot_auto_queue",
                plugin_->cvarManager, plugin_->gameWrapper, "Automatically queue into the next match after the current match ends.");
            ImGui::NextColumn();
            ImGui::NextColumn(); // Skip right column

            // Queue Delay
            ImGui::Text("Queue Delay");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-1);
            UI::Helpers::InputIntWithRange("##QueueDelay", delayQueueSecValue,
                UI::SettingsUI::DELAY_QUEUE_MIN_SECONDS, UI::SettingsUI::DELAY_QUEUE_MAX_SECONDS,
                0.0f, "suitespot_delay_queue_sec", plugin_->cvarManager,
                plugin_->gameWrapper, "Wait before auto-queuing.", nullptr);
            ImGui::NextColumn();

            // Map Delay (Context-sensitive)
            int* currentMapDelayValue = &delayFreeplaySecValue;
            const char* currentMapDelayCVar = "suitespot_delay_freeplay_sec";
            const char* mapDelayTooltip = "Wait before loading Freeplay.";

            if (mapTypeValue == 1) { // Training
                currentMapDelayValue = &delayTrainingSecValue;
                currentMapDelayCVar = "suitespot_delay_training_sec";
                mapDelayTooltip = "Wait before loading Training.";
            } else if (mapTypeValue == 2) { // Workshop
                currentMapDelayValue = &delayWorkshopSecValue;
                currentMapDelayCVar = "suitespot_delay_workshop_sec";
                mapDelayTooltip = "Wait before loading Workshop.";
            }

            ImGui::Text("Map Delay");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-1);
            UI::Helpers::InputIntWithRange("##MapDelay", *currentMapDelayValue,
                0, 300, 0.0f, currentMapDelayCVar, plugin_->cvarManager,
                plugin_->gameWrapper, mapDelayTooltip, nullptr);
            ImGui::NextColumn();

            ImGui::Columns(1); // Reset
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 2) Map Selection Logic
            RenderMapSelectionTab(mapTypeValue, (trainingModeValue == 1), currentFreeplayCode,
                currentTrainingCode, currentWorkshopPath, delayFreeplaySecValue,
                delayTrainingSecValue, delayWorkshopSecValue, delayQueueSecValue);

            ImGui::EndTabItem();
        }

        // ===== LOADOUT MANAGEMENT TAB =====
        if (ImGui::BeginTabItem("Loadout Management")) {
            if (plugin_->loadoutUI) {
                plugin_->loadoutUI->RenderLoadoutControls();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void SettingsUI::RenderGeneralTab(bool& enabledValue, int& mapTypeValue) {
    ImGui::Columns(2, "GeneralTabCols", false); // Invisible columns
    
    // Col 1: Enable
    UI::Helpers::CheckboxWithCVar("Enable SuiteSpot", enabledValue, "suitespot_enabled",
        plugin_->cvarManager, plugin_->gameWrapper, "Enable/disable all SuiteSpot auto-loading and queuing features");
    
    ImGui::NextColumn();
    
    // Col 2: Map Mode
    ImGui::TextUnformatted("Map Mode:");
    ImGui::SameLine();
    
    const char* mapLabels[] = {"Freeplay", "Training", "Workshop"};
    for (int i = 0; i < 3; i++) {
        if (i > 0) ImGui::SameLine(0, UI::SettingsUI::MAP_TYPE_RADIO_BUTTON_SPACING);
        if (ImGui::RadioButton(mapLabels[i], mapTypeValue == i)) {
            mapTypeValue = i;
            UI::Helpers::SetCVarSafely("suitespot_map_type", mapTypeValue, plugin_->cvarManager, plugin_->gameWrapper);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Choose which map type loads after matches:\nFreeplay = Official | Training = Custom Packs | Workshop = Modded Maps");
    }
    
    ImGui::Columns(1); // Reset
}

void SettingsUI::RenderMapSelectionTab(int mapTypeValue,
    bool unused, // retired
    std::string& currentFreeplayCode,
    std::string& currentTrainingCode,
    std::string& currentWorkshopPath,
    int& delayFreeplaySecValue,
    int& delayTrainingSecValue,
    int& delayWorkshopSecValue,
    int& delayQueueSecValue) {
    ImGui::TextUnformatted("Map Selection:");
    ImGui::Spacing();

    if (mapTypeValue == 0) {
        RenderFreeplayMode(currentFreeplayCode);
    } else if (mapTypeValue == 1) {
        int trainingModeValue = plugin_->settingsSync->GetTrainingMode();
        RenderTrainingMode(trainingModeValue, currentTrainingCode);
    } else if (mapTypeValue == 2) {
        RenderWorkshopMode(currentWorkshopPath);
    }
}

void SettingsUI::RenderFreeplayMode(std::string& currentFreeplayCode) {
// Initialize to first map if empty and maps are available
if (currentFreeplayCode.empty() && !RLMaps.empty()) {
    currentFreeplayCode = RLMaps[0].code;
    plugin_->settingsSync->SetCurrentFreeplayCode(currentFreeplayCode);
    plugin_->cvarManager->getCvar("suitespot_current_freeplay_code").setValue(currentFreeplayCode);
}

// Find current selection index for display
int currentIndex = 0;
if (!currentFreeplayCode.empty()) {
    for (int i = 0; i < (int)RLMaps.size(); i++) {
        if (RLMaps[i].code == currentFreeplayCode) {
            currentIndex = i;
            break;
        }
    }
}

const char* freeplayLabel = RLMaps.empty() ? "<none>" : RLMaps[currentIndex].name.c_str();
    
    ImGui::Columns(2, "FreeplayCols", false);
    ImGui::SetColumnWidth(0, 150.0f);
    
    ImGui::Text("Freeplay Map");
    ImGui::NextColumn();
    
    ImGui::SetNextItemWidth(-1);
    if (UI::Helpers::ComboWithTooltip("##FreeplayMap", freeplayLabel,
        "Select which stadium to load after matches", -1.0f)) {
        ImGuiListClipper clipper;
        clipper.Begin((int)RLMaps.size());
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                bool selected = (RLMaps[row].code == currentFreeplayCode);
                if (ImGui::Selectable(RLMaps[row].name.c_str(), selected)) {
                    currentFreeplayCode = RLMaps[row].code;
                    plugin_->settingsSync->SetCurrentFreeplayCode(currentFreeplayCode);
                    plugin_->cvarManager->getCvar("suitespot_current_freeplay_code").setValue(currentFreeplayCode);
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Columns(1);
}

void SettingsUI::RenderTrainingMode(int trainingModeValue, std::string& currentTrainingCode) {
    ImGui::TextUnformatted("Training Mode:");
    ImGui::SameLine();
    
    if (ImGui::RadioButton("Single Pack", trainingModeValue == 0)) {
        UI::Helpers::SetCVarSafely("suitespot_training_mode", 0, plugin_->cvarManager, plugin_->gameWrapper);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Bag Rotation", trainingModeValue == 1)) {
        UI::Helpers::SetCVarSafely("suitespot_training_mode", 1, plugin_->cvarManager, plugin_->gameWrapper);
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (trainingModeValue == 0) {
        RenderSinglePackMode(currentTrainingCode);
    } else {
        RenderBagRotationMode();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Open Training Pack Browser", ImVec2(250, 30))) {
        SuiteSpot* p = plugin_;
        p->gameWrapper->SetTimeout([p](GameWrapper* gw) {
            p->cvarManager->executeCommand("togglemenu suitespot_browser");
        }, 0.0f);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open the full training pack browser to manage bags and packs");
    }
}

void SettingsUI::RenderWorkshopMode(std::string& currentWorkshopPath) {
// Initialize to first map if empty and maps are available
if (currentWorkshopPath.empty() && !RLWorkshop.empty()) {
    currentWorkshopPath = RLWorkshop[0].filePath;
    plugin_->settingsSync->SetCurrentWorkshopPath(currentWorkshopPath);
    plugin_->cvarManager->getCvar("suitespot_current_workshop_path").setValue(currentWorkshopPath);
}

// Find current selection index for display
int currentIndex = 0;
if (!currentWorkshopPath.empty()) {
    for (int i = 0; i < (int)RLWorkshop.size(); i++) {
        if (RLWorkshop[i].filePath == currentWorkshopPath) {
            currentIndex = i;
            break;
        }
    }
}

const char* workshopLabel = RLWorkshop.empty() ? "<none>" : RLWorkshop[currentIndex].name.c_str();
    
    ImGui::Columns(2, "WorkshopCols", false);
    ImGui::SetColumnWidth(0, 150.0f);

    ImGui::Text("Workshop Map");
    ImGui::NextColumn();

    ImGui::SetNextItemWidth(-1);
    if (UI::Helpers::ComboWithTooltip("##WorkshopMap", workshopLabel,
        "Select which workshop map to load after matches", -1.0f)) {
        ImGuiListClipper clipper;
        clipper.Begin((int)RLWorkshop.size());
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                bool selected = (RLWorkshop[row].filePath == currentWorkshopPath);
                std::string label = RLWorkshop[row].name + "##" + std::to_string(row);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    currentWorkshopPath = RLWorkshop[row].filePath;
                    plugin_->settingsSync->SetCurrentWorkshopPath(currentWorkshopPath);
                    plugin_->cvarManager->getCvar("suitespot_current_workshop_path").setValue(currentWorkshopPath);
                }
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Refresh List", ImVec2(-1, 0))) {
        // After refresh, selection persists automatically since we use path-based lookup
        plugin_->LoadWorkshopMaps();
    }
    ImGui::Columns(1);

    ImGui::Spacing();
    if (ImGui::TreeNodeEx("Workshop Source", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!workshopPathInit) {
            auto resolved = plugin_->ResolveConfiguredWorkshopRoot();
            if (resolved.empty()) {
                const char* pfx86 = std::getenv("ProgramFiles(x86)");
                if (pfx86) {
                    workshopPathCache = (std::filesystem::path(pfx86) / "Steam" / "steamapps" / "common" / "rocketleague" / "TAGame" / "CookedPCConsole" / "mods").string();
                } else {
                    workshopPathCache = "";
                }
            } else {
                workshopPathCache = resolved.string();
            }
            strncpy_s(workshopPathBuf, workshopPathCache.c_str(), sizeof(workshopPathBuf) - 1);
            workshopPathInit = true;
        }

        ImGui::Columns(2, "WorkshopSourceCols", false);
        ImGui::SetColumnWidth(0, 150.0f);

        ImGui::Text("Maps Root Folder");
        ImGui::NextColumn();

        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##workshop_root", workshopPathBuf, IM_ARRAYSIZE(workshopPathBuf));

        if (ImGui::Button("Save Path", ImVec2(-1, 0))) {
            std::filesystem::path workshopPath(workshopPathBuf);
            if (!workshopPath.empty() && std::filesystem::exists(workshopPath) && std::filesystem::is_directory(workshopPath)) {
                std::filesystem::path cfgPath = plugin_->GetWorkshopLoaderConfigPath();
                std::filesystem::create_directories(cfgPath.parent_path());
                std::ofstream cfg(cfgPath.string(), std::ios::trunc);
                if (cfg.is_open()) {
                    cfg << "MapsFolderPath=" << workshopPathBuf << "\n";
                    cfg.close();
                    workshopPathCache = workshopPathBuf;
                    plugin_->LoadWorkshopMaps();
                    statusMessage.ShowSuccess("Workshop path saved!", 3.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                }
            }
        }
        ImGui::Columns(1);
        ImGui::TreePop();
    }
}

void SettingsUI::RenderSinglePackMode(std::string& currentTrainingCode) {
// Toggle between Flicks Picks and Your Favorites
int listType = plugin_->settingsSync->GetQuickPicksListType();
    
ImGui::TextUnformatted("List Type:");
ImGui::SameLine();
    
if (ImGui::RadioButton("Flicks Picks", listType == 0)) {
    UI::Helpers::SetCVarSafely("suitespot_quickpicks_list_type", 0, plugin_->cvarManager, plugin_->gameWrapper);
}
if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Curated selection of 10 essential training packs");
}
ImGui::SameLine();
if (ImGui::RadioButton("Your Favorites", listType == 1)) {
    UI::Helpers::SetCVarSafely("suitespot_quickpicks_list_type", 1, plugin_->cvarManager, plugin_->gameWrapper);
}
if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Your most-used training packs based on load history");
}

ImGui::Spacing();
ImGui::Separator();
ImGui::Spacing();

// Display header based on list type
if (listType == 0) {
    ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "Flicks Picks");
} else {
    ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "Your Favorites");
}
ImGui::SameLine();
ImGui::TextDisabled("(Select post-match pack)");

std::vector<std::string> quickPicks = GetQuickPicksList();
std::string selectedCode = plugin_->settingsSync->GetQuickPicksSelectedCode();
    
    // If nothing selected, default to the first one in the list
    if (selectedCode.empty() && !quickPicks.empty()) {
        selectedCode = quickPicks[0];
        plugin_->settingsSync->SetQuickPicksSelected(selectedCode);
        plugin_->cvarManager->getCvar("suitespot_quickpicks_selected").setValue(selectedCode);
    }

    if (ImGui::BeginChild("QuickPicksList", ImVec2(0, UI::QuickPicksUI::TABLE_HEIGHT), true)) {
        for (const auto& code : quickPicks) {
            std::string name = "Unknown Pack";
            int shots = 0;
            std::string description = "";
            bool found = false;

            // 1. Try to find in loaded cache
            const auto& trainingPacks = plugin_->trainingPackMgr ? plugin_->trainingPackMgr->GetPacks() : RLTraining;
            auto it = std::find_if(trainingPacks.begin(), trainingPacks.end(), [&](const TrainingEntry& e) { return e.code == code; });
            
            if (it != trainingPacks.end()) {
                name = it->name;
                shots = it->shotCount;
                description = it->staffComments.empty() ? it->notes : it->staffComments;
                found = true;
            } else {
                // 2. Try to find in DefaultPacks (Hardcoded fallback for first run)
                for (const auto& defPack : DefaultPacks::FLICKS_PICKS) {
                    if (defPack.code == code) {
                        name = defPack.name;
                        shots = defPack.shotCount;
                        description = defPack.description;
                        found = true;
                        break;
                    }
                }
            }

            if (found) {
                bool isSelected = (code == selectedCode);
                ImGui::PushID(code.c_str());
                if (ImGui::RadioButton("##select", isSelected)) {
                    selectedCode = code;
                    plugin_->settingsSync->SetQuickPicksSelected(code);
                    plugin_->cvarManager->getCvar("suitespot_quickpicks_selected").setValue(code);
                }
                ImGui::SameLine();
                
                // Name and Shot Count
                float availWidth = ImGui::GetContentRegionAvail().x;
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine(availWidth - 80.0f); 
                ImGui::TextDisabled("| %d shots", shots);

                // Description (Indented and Wrapped)
                if (!description.empty()) {
                    ImGui::Indent(28.0f); 
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionWidth() - 10.0f);
                    ImGui::TextUnformatted(description.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::PopStyleColor();
                    ImGui::Unindent(28.0f);
                }
                
                ImGui::Separator();
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();
}

void SettingsUI::RenderBagRotationMode() {
    // Bag rotation feature removed
    ImGui::TextDisabled("Bag rotation feature has been removed");
}

std::vector<std::string> SettingsUI::GetQuickPicksList() {
    int listType = plugin_->settingsSync->GetQuickPicksListType();
    
    // Build Flicks Picks list
    std::vector<std::string> flicksPicks;
    for (const auto& p : DefaultPacks::FLICKS_PICKS) {
        flicksPicks.push_back(p.code);
    }

    // If Flicks Picks mode is selected, always return Flicks Picks
    if (listType == 0) {
        return flicksPicks;
    }

    // Your Favorites mode - use usage tracker
    if (!plugin_->usageTracker) return flicksPicks;

    // If first run or no data, fallback to Flicks Picks
    if (plugin_->usageTracker->IsFirstRun()) {
        return flicksPicks;
    }

    int count = plugin_->settingsSync->GetQuickPicksCount();
    auto topCodes = plugin_->usageTracker->GetTopUsedCodes(count);
    
    // If no favorites yet, fallback to Flicks Picks
    if (topCodes.empty()) return flicksPicks;
    
    return topCodes;
}
