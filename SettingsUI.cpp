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
    // Workshop Map Search & Download
    ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "Workshop Maps");
    ImGui::Spacing();
    
    // Search section
    static char searchKeywords[128] = "";
    ImGui::Text("Search Maps:");
    ImGui::SetNextItemWidth(300.0f);
    if (ImGui::InputText("##workshop_search", searchKeywords, IM_ARRAYSIZE(searchKeywords), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (strlen(searchKeywords) > 0 && plugin_->workshopDownloader) {
            plugin_->workshopDownloader->SearchMaps(searchKeywords, 1);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Search") && plugin_->workshopDownloader) {
        if (strlen(searchKeywords) > 0) {
            plugin_->workshopDownloader->SearchMaps(searchKeywords, 1);
        }
    }
    
    // Show searching status
    if (plugin_->workshopDownloader && plugin_->workshopDownloader->IsSearching()) {
        ImGui::TextDisabled("Searching...");
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Search results
    if (plugin_->workshopDownloader && !plugin_->workshopDownloader->GetSearchResults().empty()) {
        ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "Search Results");
        
        const auto& results = plugin_->workshopDownloader->GetSearchResults();
        
        if (ImGui::BeginChild("SearchResults", ImVec2(0, 400), true)) {
            for (size_t i = 0; i < results.size(); i++) {
                const auto& result = results[i];
                
                ImGui::PushID((int)i);
                
                // Map name
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImGui::Text("%s", result.name.c_str());
                ImGui::PopStyleColor();
                
                // Author
                ImGui::SameLine();
                ImGui::TextDisabled("by %s", result.author.c_str());
                
                // Description
                if (!result.description.empty()) {
                    ImGui::Indent(20.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    ImGui::TextWrapped("%s", result.description.c_str());
                    ImGui::PopStyleColor();
                    ImGui::Unindent(20.0f);
                }
                
                // Action buttons
                ImGui::Spacing();
                
                // Download button
                if (ImGui::Button("Download")) {
                    auto workshopFolder = plugin_->workshopDownloader->GetWorkshopFolder();
                    plugin_->workshopDownloader->DownloadMap(result, workshopFolder, nullptr);
                }
                
                // Load Now button - check if map is already downloaded
                ImGui::SameLine();
                bool mapExists = false;
                std::string mapPath;
                for (const auto& workshop : RLWorkshop) {
                    if (workshop.name == result.name) {
                        mapExists = true;
                        mapPath = workshop.filePath;
                        break;
                    }
                }
                
                if (mapExists) {
                    if (ImGui::Button("Load Now")) {
                        plugin_->gameWrapper->Execute([mapPath](GameWrapper* gw) {
                            gw->ExecuteUnrealCommand("load_workshop \"" + mapPath + "\"");
                        });
                    }
                } else {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    ImGui::Button("Load Now");
                    ImGui::PopStyleVar();
                    ImGui::PopItemFlag();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("Download the map first");
                    }
                }
                
                ImGui::Separator();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    }
    
    // Download progress
    if (plugin_->workshopDownloader && plugin_->workshopDownloader->IsDownloading()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        const auto& progress = plugin_->workshopDownloader->GetDownloadProgress();
        
        ImGui::Text("Downloading: %s", progress.mapName.c_str());
        
        float progressFraction = progress.totalBytes > 0 ? 
            (float)progress.bytesDownloaded / (float)progress.totalBytes : 0.0f;
        
        char progressLabel[64];
        snprintf(progressLabel, sizeof(progressLabel), "%d%%", progress.percentComplete);
        ImGui::ProgressBar(progressFraction, ImVec2(-1, 0), progressLabel);
        
        if (progress.hasFailed) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Download failed: %s", progress.errorMessage.c_str());
        } else if (progress.isComplete) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Download complete!");
            // Refresh workshop maps after successful download
            plugin_->LoadWorkshopMaps();
        }
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
    
    auto& resultList = plugin_->workshopDownloader->RLMAPS_MapResultList;
    if (resultList.empty()) return;
    
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
