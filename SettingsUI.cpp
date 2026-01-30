#include "pch.h"

#include "SettingsUI.h"
#include "LoadoutUI.h"
#include "TrainingPackUI.h"
#include "SuiteSpot.h"
#include "MapManager.h"
#include "TrainingPackManager.h"
#include "WorkshopDownloader.h"
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
            // Bag rotation removed - always use single pack mode
            std::string code = p->settingsSync->GetQuickPicksSelectedCode();
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
    // Bag rotation removed - trainingModeValue removed
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
            RenderMapSelectionTab(mapTypeValue, false, currentFreeplayCode,
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
        
        // ===== WORKSHOP BROWSER TAB =====
        if (ImGui::BeginTabItem("Workshop Browser")) {
            RenderWorkshopBrowserTab();
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
        // Bag rotation removed - always single pack mode
        RenderTrainingMode(0, currentTrainingCode);
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
    // Bag rotation removed - always use single pack mode
    RenderSinglePackMode(currentTrainingCode);

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
    // Header with Refresh button
    ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "Local Workshop Maps");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70.0f);
    if (ImGui::Button("Refresh", ImVec2(70, 0))) {
        plugin_->LoadWorkshopMaps();
        selectedWorkshopIndex = -1;  // Reset selection
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Rescan workshop folders for maps");
    }
    ImGui::Spacing();

    // Check if we have any maps
    if (RLWorkshop.empty()) {
        ImGui::TextColored(UI::WorkshopBrowserUI::NO_MAPS_COLOR,
            "No workshop maps found.");
        ImGui::TextDisabled("Maps are discovered from:");
        ImGui::BulletText("WorkshopMapLoader configured path");
        ImGui::BulletText("Epic Games install mods folder");
        ImGui::BulletText("Steam install mods folder");
        ImGui::Spacing();
        ImGui::TextDisabled("Download maps from the Workshop Browser tab above.");
        return;
    }

    // Initialize selection from current CVar if needed
    if (selectedWorkshopIndex < 0 && !currentWorkshopPath.empty()) {
        for (int i = 0; i < (int)RLWorkshop.size(); i++) {
            if (RLWorkshop[i].filePath == currentWorkshopPath) {
                selectedWorkshopIndex = i;
                break;
            }
        }
    }

    // Clamp selection to valid range
    if (selectedWorkshopIndex >= (int)RLWorkshop.size()) {
        selectedWorkshopIndex = (int)RLWorkshop.size() - 1;
    }

    // Calculate panel widths
    float availWidth = ImGui::GetContentRegionAvail().x;
    float leftWidth = std::max(UI::WorkshopBrowserUI::LEFT_PANEL_MIN_WIDTH,
                               availWidth * UI::WorkshopBrowserUI::LEFT_PANEL_WIDTH_PERCENT);
    float rightWidth = availWidth - leftWidth - ImGui::GetStyle().ItemSpacing.x;

    // Two-panel layout
    ImGui::BeginGroup();

    // === LEFT PANEL: Map List ===
    if (ImGui::BeginChild("WorkshopMapList", ImVec2(leftWidth, UI::WorkshopBrowserUI::BROWSER_HEIGHT), true)) {
        ImGui::TextDisabled("%d maps", (int)RLWorkshop.size());
        ImGui::Separator();

        for (int i = 0; i < (int)RLWorkshop.size(); i++) {
            const auto& entry = RLWorkshop[i];
            bool isSelected = (i == selectedWorkshopIndex);
            bool isCurrentAutoLoad = (entry.filePath == currentWorkshopPath);

            ImGui::PushID(i);

            // Show marker if this is the current auto-load selection
            if (isCurrentAutoLoad) {
                ImGui::PushStyleColor(ImGuiCol_Text, UI::WorkshopBrowserUI::SELECTED_BADGE_COLOR);
                ImGui::Text(">");
                ImGui::PopStyleColor();
                ImGui::SameLine();
            }

            if (ImGui::Selectable(entry.name.c_str(), isSelected, ImGuiSelectableFlags_None)) {
                selectedWorkshopIndex = i;
            }

            // Double-click to load immediately
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                SuiteSpot* p = plugin_;
                std::string path = entry.filePath;
                p->gameWrapper->SetTimeout([p, path](GameWrapper* gw) {
                    p->cvarManager->executeCommand("load_workshop \"" + path + "\"");
                }, 0.0f);
                statusMessage.ShowSuccess("Loading Workshop Map", 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // === RIGHT PANEL: Details Pane ===
    if (ImGui::BeginChild("WorkshopMapDetails", ImVec2(rightWidth, UI::WorkshopBrowserUI::BROWSER_HEIGHT), true)) {
        if (selectedWorkshopIndex >= 0 && selectedWorkshopIndex < (int)RLWorkshop.size()) {
            auto& selectedMap = RLWorkshop[selectedWorkshopIndex];

            // Load preview image if needed (lazy loading)
            if (!selectedMap.previewPath.empty() && !selectedMap.isImageLoaded && !selectedMap.previewImage) {
                selectedMap.previewImage = std::make_shared<ImageWrapper>(selectedMap.previewPath.string(), false, true);
                selectedMap.isImageLoaded = true;
            }

            // Preview image
            if (selectedMap.previewImage && selectedMap.previewImage->GetImGuiTex()) {
                ImGui::Image(selectedMap.previewImage->GetImGuiTex(),
                             ImVec2(UI::WorkshopBrowserUI::PREVIEW_IMAGE_WIDTH,
                                    UI::WorkshopBrowserUI::PREVIEW_IMAGE_HEIGHT));
            } else {
                // Placeholder box
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(p,
                    ImVec2(p.x + UI::WorkshopBrowserUI::PREVIEW_IMAGE_WIDTH,
                           p.y + UI::WorkshopBrowserUI::PREVIEW_IMAGE_HEIGHT),
                    ImColor(40, 40, 45, 255), 4.0f);
                drawList->AddText(
                    ImVec2(p.x + UI::WorkshopBrowserUI::PREVIEW_IMAGE_WIDTH / 2 - 40,
                           p.y + UI::WorkshopBrowserUI::PREVIEW_IMAGE_HEIGHT / 2 - 8),
                    ImColor(100, 100, 100, 255), "No Preview");
                ImGui::Dummy(ImVec2(UI::WorkshopBrowserUI::PREVIEW_IMAGE_WIDTH,
                                    UI::WorkshopBrowserUI::PREVIEW_IMAGE_HEIGHT));
            }

            ImGui::Spacing();

            // Map name (large)
            ImGui::PushStyleColor(ImGuiCol_Text, UI::WorkshopBrowserUI::MAP_NAME_COLOR);
            ImGui::TextWrapped("%s", selectedMap.name.c_str());
            ImGui::PopStyleColor();

            // Author
            if (!selectedMap.author.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, UI::WorkshopBrowserUI::AUTHOR_COLOR);
                ImGui::Text("By: %s", selectedMap.author.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            // Description
            if (!selectedMap.description.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, UI::WorkshopBrowserUI::DESCRIPTION_COLOR);
                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                ImGui::TextWrapped("%s", selectedMap.description.c_str());
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Current selection indicator
            bool isCurrentAutoLoad = (selectedMap.filePath == currentWorkshopPath);
            if (isCurrentAutoLoad) {
                ImGui::TextColored(UI::WorkshopBrowserUI::SELECTED_BADGE_COLOR, "Selected for Post-Match Auto-Load");
                ImGui::Spacing();
            }

            // Action buttons
            if (!isCurrentAutoLoad) {
                if (ImGui::Button("Select for Post-Match", ImVec2(180, 26))) {
                    plugin_->settingsSync->SetCurrentWorkshopPath(selectedMap.filePath);
                    plugin_->cvarManager->getCvar("suitespot_current_workshop_path").setValue(selectedMap.filePath);
                    currentWorkshopPath = selectedMap.filePath;
                    statusMessage.ShowSuccess("Workshop map selected", 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Set this map to load after matches end");
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Load Now", ImVec2(100, 26))) {
                SuiteSpot* p = plugin_;
                std::string path = selectedMap.filePath;
                p->gameWrapper->SetTimeout([p, path](GameWrapper* gw) {
                    p->cvarManager->executeCommand("load_workshop \"" + path + "\"");
                }, 0.0f);
                statusMessage.ShowSuccess("Loading Workshop Map", 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Load this workshop map immediately");
            }

        } else {
            // No selection
            ImGui::TextDisabled("Select a map from the list");
        }
    }
    ImGui::EndChild();

    ImGui::EndGroup();
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

    if (ImGui::BeginChild("QuickPicksList", ImVec2(UI::QuickPicksUI::TABLE_WIDTH, UI::QuickPicksUI::TABLE_HEIGHT), true)) {
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
                
                // Add top padding
                ImGui::Dummy(ImVec2(0, 4.0f));
                
                ImGui::PushID(code.c_str());
                if (ImGui::RadioButton("##select", isSelected)) {
                    selectedCode = code;
                    plugin_->settingsSync->SetQuickPicksSelected(code);
                    plugin_->cvarManager->getCvar("suitespot_quickpicks_selected").setValue(code);
                }
                ImGui::SameLine();
                
                // Name and Shot Count
                float availWidth = ImGui::GetContentRegionAvail().x;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White for name
                ImGui::Text("%s", name.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine(availWidth - 80.0f); 
                ImGui::TextDisabled("| %d shots", shots);

                // Description (Indented and Wrapped)
                if (!description.empty()) {
                    ImGui::Indent(28.0f); 
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Darker gray for description
                    ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionWidth() - 10.0f);
                    ImGui::TextUnformatted(description.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::PopStyleColor();
                    ImGui::Unindent(28.0f);
                }
                
                // Add bottom padding
                ImGui::Dummy(ImVec2(0, 4.0f));
                
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
void SettingsUI::RenderWorkshopBrowserTab() {
    if (!plugin_->workshopDownloader) {
        ImGui::TextDisabled("Workshop downloader not initialized");
        return;
    }

    ImGui::Spacing();
    
    static bool pathInit = false;
    if (!pathInit) {
        std::string defaultPath = plugin_->gameWrapper->GetDataFolder().string() + "\\SuiteSpot\\Workshop";
        strncpy_s(workshopDownloadPathBuf, defaultPath.c_str(), sizeof(workshopDownloadPathBuf) - 1);
        pathInit = true;
    }

    // Download destination path
    ImGui::Text("Download to:");
    ImGui::SetNextItemWidth(400.0f);
    ImGui::InputText("##WorkshopPath", workshopDownloadPathBuf, IM_ARRAYSIZE(workshopDownloadPathBuf));
    
    ImGui::Spacing();
    
    // Search bar
    ImGui::Text("Search Maps:");
    ImGui::SetNextItemWidth(400.0f);
    bool enterPressed = ImGui::InputText("##WorkshopSearch", workshopSearchBuf, IM_ARRAYSIZE(workshopSearchBuf), 
                                         ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    
    if ((ImGui::Button("Search") || enterPressed) && strlen(workshopSearchBuf) > 0) {
        plugin_->workshopDownloader->GetResults(workshopSearchBuf, 1);
    }
    
    ImGui::SameLine();
    if (plugin_->workshopDownloader->RLMAPS_Searching) {
        ImGui::TextDisabled("Searching...");
    } else if (plugin_->workshopDownloader->RLMAPS_MapResultList.size() > 0) {
        ImGui::Text("%d maps found", plugin_->workshopDownloader->RLMAPS_NumberOfMapsFound);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Search results
    RLMAPS_RenderSearchWorkshopResults(workshopDownloadPathBuf);
    
    // Popups
    RenderAcceptDownload();
    RenderInfoPopup("Downloading?", "A download is already running!\\nYou cannot download 2 workshops at the same time.");
    RenderInfoPopup("Exists?", "This directory is not valid!");
    
    if (plugin_->workshopDownloader->FolderErrorBool) {
        RenderInfoPopup("FolderError", plugin_->workshopDownloader->FolderErrorText.c_str());
    }
}

void SettingsUI::RLMAPS_RenderSearchWorkshopResults(const char* mapspath) {
    if (!plugin_->workshopDownloader) return;
    
    std::lock_guard<std::mutex> lock(plugin_->workshopDownloader->resultsMutex);

    auto& resultList = plugin_->workshopDownloader->RLMAPS_MapResultList;
    if (resultList.empty()) return;

    // Safe Lazy Loading of Preview Images on Render Thread
    for (auto& map : resultList) {
        if (map.Image == nullptr && !map.IsDownloadingPreview && !map.ImagePath.empty()) {
            // Check if file exists before trying to load
            if (std::filesystem::exists(map.ImagePath)) {
                map.Image = std::make_shared<ImageWrapper>(map.ImagePath.string(), false, true);
                if (map.Image) {
                    map.isImageLoaded = true;
                }
            }
        }
    }
    
    if (ImGui::BeginChild("##SearchResults", ImVec2(0, 500), true)) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        int columns = 4;
        float cardWidth = 190.0f;
        float cardHeight = 260.0f;
        
        for (int i = 0; i < resultList.size(); i++) {
            if (i % columns != 0) ImGui::SameLine();
            
            RLMAPS_RenderAResult(i, drawList, mapspath);
        }
    }
    ImGui::EndChild();
}

void SettingsUI::RLMAPS_RenderAResult(int i, ImDrawList* drawList, const char* mapspath) {
    if (!plugin_->workshopDownloader) return;
    
    auto& resultList = plugin_->workshopDownloader->RLMAPS_MapResultList;
    if (i >= resultList.size()) return;
    
    ImGui::PushID(i);
    
    RLMAPS_MapResult& mapResult = resultList[i];
    std::string mapName = mapResult.Name;
    std::string mapDescription = mapResult.Description;
    std::string mapAuthor = mapResult.Author;
    
    ImGui::BeginChild("##RlmapsResult", ImVec2(190.0f, 260.0f));
    {
        ImGui::BeginGroup();
        {
            ImVec2 TopCornerLeft = ImGui::GetCursorScreenPos();
            ImVec2 RectFilled_p_max = ImVec2(TopCornerLeft.x + 190.0f, TopCornerLeft.y + 260.0f);
            ImVec2 ImageP_Min = ImVec2(TopCornerLeft.x + 6.0f, TopCornerLeft.y + 6.0f);
            ImVec2 ImageP_Max = ImVec2(TopCornerLeft.x + 184.0f, TopCornerLeft.y + 179.0f);
            
            drawList->AddRectFilled(TopCornerLeft, RectFilled_p_max, ImColor(44, 75, 113, 255), 5.0f, 15);
            drawList->AddRect(ImageP_Min, ImageP_Max, ImColor(255, 255, 255, 255), 0, 15, 2.0f);
            
            if (mapResult.isImageLoaded && mapResult.Image) {
                try {
                    if (mapResult.Image->GetImGuiTex()) {
                        drawList->AddImage(mapResult.Image->GetImGuiTex(), ImageP_Min, ImageP_Max);
                    }
                } catch (...) {}
            }
            
            std::string GoodMapName = mapName;
            if (ImGui::CalcTextSize(GoodMapName.c_str()).x > 180.0f) {
                GoodMapName = LimitTextSize(GoodMapName, 170.0f) + "...";
            }
            drawList->AddText(ImVec2(TopCornerLeft.x + 4.0f, TopCornerLeft.y + 185.0f), 
                             ImColor(255, 255, 255, 255), GoodMapName.c_str());
            drawList->AddText(ImVec2(TopCornerLeft.x + 4.0f, TopCornerLeft.y + 215.0f), 
                             ImColor(255, 255, 255, 255), ("By " + mapAuthor).c_str());
            
            ImGui::SetCursorScreenPos(ImVec2(TopCornerLeft.x + 4.0f, TopCornerLeft.y + 235.0f));
            if (ImGui::Button("Download", ImVec2(182, 20))) {
                if (!plugin_->workshopDownloader->RLMAPS_IsDownloadingWorkshop && 
                    fs::exists(mapspath)) {
                    ImGui::OpenPopup("Releases");
                } else if (!fs::exists(mapspath)) {
                    ImGui::OpenPopup("Exists?");
                } else {
                    ImGui::OpenPopup("Downloading?");
                }
            }
            
            RenderReleases(mapResult, mapspath);
            
            ImGui::EndGroup();
            
            if (ImGui::IsItemHovered()) {
                std::string GoodDescription = mapDescription;
                if (mapDescription.length() > 150) {
                    GoodDescription = mapDescription.substr(0, 150) + "...";
                }
                
                ImGui::BeginTooltip();
                ImGui::Text("Title: %s", mapName.c_str());
                ImGui::Text("By: %s", mapAuthor.c_str());
                ImGui::Text("Description:\\n%s", GoodDescription.c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::EndChild();
    }
    
    ImGui::PopID();
}

void SettingsUI::RenderReleases(RLMAPS_MapResult mapResult, const char* mapspath) {
    if (ImGui::BeginPopupModal("Releases", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        for (int releasesIndex = 0; releasesIndex < mapResult.releases.size(); releasesIndex++) {
            RLMAPS_Release release = mapResult.releases[releasesIndex];
            
            if (ImGui::Button(release.tag_name.c_str(), ImVec2(182, 20))) {
                if (!plugin_->workshopDownloader->RLMAPS_IsDownloadingWorkshop && 
                    fs::exists(mapspath)) {
                    std::thread t2(&WorkshopDownloader::RLMAPS_DownloadWorkshop, 
                                   plugin_->workshopDownloader.get(), 
                                   std::string(mapspath), mapResult, release);
                    t2.detach();
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        if (ImGui::Button("Cancel", ImVec2(182, 20))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void SettingsUI::RenderAcceptDownload() {
    if (!plugin_->workshopDownloader) return;
    
    RenderYesNoPopup("Download?", 
                     "Do you really want to download?\\nYou'll not be able to cancel if you start it.",
                     [this]() {
                         plugin_->workshopDownloader->AcceptTheDownload = true;
                         plugin_->workshopDownloader->UserIsChoosingYESorNO = false;
                         ImGui::CloseCurrentPopup();
                     },
                     [this]() {
                         plugin_->workshopDownloader->AcceptTheDownload = false;
                         plugin_->workshopDownloader->UserIsChoosingYESorNO = false;
                         ImGui::CloseCurrentPopup();
                     });
}

void SettingsUI::RenderYesNoPopup(const char* popupName, const char* label, 
                                   std::function<void()> yesFunc, std::function<void()> noFunc) {
    if (ImGui::BeginPopupModal(popupName, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", label);
        ImGui::NewLine();
        
        CenterNextItem(208.0f);
        ImGui::BeginGroup();
        {
            if (ImGui::Button("YES", ImVec2(100.0f, 25.0f))) {
                try {
                    yesFunc();
                } catch (const std::exception& ex) {
                    LOG("Popup error: {}", ex.what());
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("NO", ImVec2(100.0f, 25.0f))) {
                noFunc();
            }
            ImGui::EndGroup();
        }
        
        ImGui::EndPopup();
    }
}

void SettingsUI::RenderInfoPopup(const char* popupName, const char* label) {
    if (ImGui::BeginPopupModal(popupName, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", label);
        ImGui::NewLine();
        CenterNextItem(100.0f);
        if (ImGui::Button("OK", ImVec2(100.0f, 25.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SettingsUI::CenterNextItem(float itemWidth) {
    auto windowWidth = ImGui::GetWindowSize().x;
    ImGui::SetCursorPosX((windowWidth - itemWidth) * 0.5f);
}

std::string SettingsUI::LimitTextSize(std::string str, float maxTextSize) {
    while (ImGui::CalcTextSize(str.c_str()).x > maxTextSize) {
        if (str.empty()) break;
        str = str.substr(0, str.size() - 1);
    }
    return str;
}
