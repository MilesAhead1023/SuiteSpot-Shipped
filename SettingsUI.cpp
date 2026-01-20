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

    // Header with metadata
    ImGui::TextUnformatted("SuiteSpot");
    ImGui::TextColored(UI::SettingsUI::HEADER_TEXT_COLOR, "By: Flicks Creations");
    ImGui::TextColored(UI::SettingsUI::HEADER_TEXT_COLOR, "Version: %s", plugin_version);

    ImGui::Spacing();
    statusMessage.Render(ImGui::GetIO().DeltaTime);
    if (statusMessage.IsVisible()) ImGui::Spacing();

    bool enabledValue = plugin_->IsEnabled();
    int mapTypeValue = plugin_->GetMapType();
    bool autoQueueValue = plugin_->IsAutoQueueEnabled();
    bool bagRotationEnabledValue = plugin_->IsBagRotationEnabled();
    int delayQueueSecValue = plugin_->GetDelayQueueSec();
    int delayFreeplaySecValue = plugin_->GetDelayFreeplaySec();
    int delayTrainingSecValue = plugin_->GetDelayTrainingSec();
    int delayWorkshopSecValue = plugin_->GetDelayWorkshopSec();
    std::string currentFreeplayCode = plugin_->GetCurrentFreeplayCode();
    std::string currentTrainingCode = plugin_->GetCurrentTrainingCode();
    std::string currentWorkshopPath = plugin_->GetCurrentWorkshopPath();

    // Only show status if enabled
    if (enabledValue) {
        ImGui::SameLine(UI::SettingsUI::STATUS_TEXT_POSITION_X);

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

            if (bagRotationEnabledValue && plugin_->trainingPackMgr) {
                // Show current bag in rotation
                std::string currentBag = plugin_->settingsSync->GetCurrentBag();
                if (!currentBag.empty()) {
                    currentMap = "Bag Rotation: " + currentBag;
                } else if (!currentTrainingCode.empty()) {
                    // Find training pack by code
                    auto it = std::find_if(trainingPacks.begin(), trainingPacks.end(),
                        [&](const TrainingEntry& e) { return e.code == currentTrainingCode; });
                    if (it != trainingPacks.end()) {
                        currentMap = it->name;
                    }
                }
            } else if (!currentTrainingCode.empty()) {
                // Find training pack by code
                auto it = std::find_if(trainingPacks.begin(), trainingPacks.end(),
                    [&](const TrainingEntry& e) { return e.code == currentTrainingCode; });
                if (it != trainingPacks.end()) {
                    currentMap = it->name + " (Shots:" + std::to_string(it->shotCount) + ")";
                }
            }
        } else if (mapTypeValue == 2) {
            // Find workshop map by path
            auto it = std::find_if(RLWorkshop.begin(), RLWorkshop.end(),
                [&](const WorkshopEntry& e) { return e.filePath == currentWorkshopPath; });
            if (it != RLWorkshop.end()) {
                currentMap = it->name;
            }
            mapDelayStr = std::to_string(delayWorkshopSecValue) + "s";
        }

        // Map mode status (always green when enabled)
        const ImVec4 green = UI::SettingsUI::STATUS_ENABLED_TEXT_COLOR;
        const ImVec4 red   = UI::SettingsUI::STATUS_DISABLED_TEXT_COLOR;
        std::string modeText = "Mode: " + std::string(modeNames[mapTypeValue]);
        if (delayFreeplaySecValue > 0 || delayTrainingSecValue > 0 || delayWorkshopSecValue > 0) {
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
        ImGui::NewLine();
    }

    ImGui::Separator();

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
            ImGui::BeginGroup();
            
            // Auto-Queue Toggle
            UI::Helpers::CheckboxWithCVar("Auto-Queue", autoQueueValue, "suitespot_auto_queue",
                plugin_->cvarManager, plugin_->gameWrapper, "Automatically queue into the next match after the current match ends.");
            
            ImGui::SameLine(0, 20);

            // Queue Delay
            UI::Helpers::InputIntWithRange("Queue Delay", delayQueueSecValue,
                UI::SettingsUI::DELAY_QUEUE_MIN_SECONDS, UI::SettingsUI::DELAY_QUEUE_MAX_SECONDS,
                UI::SettingsUI::DELAY_QUEUE_INPUT_WIDTH, "suitespot_delay_queue_sec", plugin_->cvarManager,
                plugin_->gameWrapper, "Wait before auto-queuing.", "0-300s");

            ImGui::SameLine(0, 20);

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

            UI::Helpers::InputIntWithRange("Map Delay", *currentMapDelayValue,
                0, 300, UI::SettingsUI::DELAY_FREEPLAY_INPUT_WIDTH, currentMapDelayCVar, plugin_->cvarManager,
                plugin_->gameWrapper, mapDelayTooltip, "0-300s");

            ImGui::EndGroup();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 2) Map Selection Logic
            RenderMapSelectionTab(mapTypeValue, bagRotationEnabledValue, currentFreeplayCode,
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
    ImGui::BeginGroup();
    UI::Helpers::CheckboxWithCVar("Enable SuiteSpot", enabledValue, "suitespot_enabled",
        plugin_->cvarManager, plugin_->gameWrapper, "Enable/disable all SuiteSpot auto-loading and queuing features");
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::TextUnformatted("Map Mode:");
    ImGui::SameLine();
    ImGui::BeginGroup();
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
    ImGui::EndGroup();
}

void SettingsUI::RenderMapSelectionTab(int mapTypeValue,
    bool bagRotationEnabledValue,
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
        RenderTrainingMode(bagRotationEnabledValue, currentTrainingCode);
    } else if (mapTypeValue == 2) {
        RenderWorkshopMode(currentWorkshopPath);
    }
}

void SettingsUI::RenderFreeplayMode(std::string& currentFreeplayCode) {
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
    if (UI::Helpers::ComboWithTooltip("Freeplay Maps", freeplayLabel,
        "Select which stadium to load after matches", UI::SettingsUI::FREEPLAY_MAPS_DROPDOWN_WIDTH)) {
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

    ImGui::SameLine();
    if (ImGui::Button("Load Now##freeplay")) {
        if (!currentFreeplayCode.empty()) {
            std::string mapCode = currentFreeplayCode;
            SuiteSpot* p = plugin_;
            p->gameWrapper->SetTimeout([p, mapCode](GameWrapper* gw) {
                p->cvarManager->executeCommand("load_freeplay " + mapCode);
            }, 0.0f);
        }
    }
}

void SettingsUI::RenderTrainingMode(bool bagRotationEnabledValue, std::string& currentTrainingCode) {
    ImGui::TextUnformatted("Training Settings:");

    // Show bag rotation status
    if (bagRotationEnabledValue && plugin_->trainingPackMgr) {
        std::string currentBag = plugin_->settingsSync->GetCurrentBag();
        if (!currentBag.empty()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Bag Rotation Active - Current: %s", currentBag.c_str());
        } else {
            ImGui::TextDisabled("Bag Rotation: No packs in any bag");
        }
    }

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
    if (UI::Helpers::ComboWithTooltip("Workshop Maps", workshopLabel,
        "Select which workshop map to load after matches", UI::SettingsUI::WORKSHOP_MAPS_DROPDOWN_WIDTH)) {
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

    ImGui::SameLine();
    if (ImGui::Button("Refresh Workshop##maps")) {
        // After refresh, selection persists automatically since we use path-based lookup
        plugin_->LoadWorkshopMaps();
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Now##workshop")) {
        if (!currentWorkshopPath.empty()) {
            std::string filePath = currentWorkshopPath;
            SuiteSpot* p = plugin_;
            p->gameWrapper->SetTimeout([p, filePath](GameWrapper* gw) {
                p->cvarManager->executeCommand("load_workshop \"" + filePath + "\"");
            }, 0.0f);
        }
    }

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

        ImGui::TextWrapped("Workshop maps root folder:");
        ImGui::SetNextItemWidth(UI::SettingsUI::WORKSHOP_PATH_INPUT_WIDTH);
        ImGui::InputText("##workshop_root", workshopPathBuf, IM_ARRAYSIZE(workshopPathBuf));

        if (ImGui::Button("Save Workshop Source")) {
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
        ImGui::TreePop();
    }
}
