#include "pch.h"

#include "SettingsUI.h"
#include "LoadoutUI.h"
#include "TrainingPackUI.h"
#include "SuiteSpot.h"
#include "MapManager.h"
#include "TrainingPackManager.h"
#include "UIConstants.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>

SettingsUI::SettingsUI(SuiteSpot* plugin) : plugin_(plugin) {}

void SettingsUI::RenderMainSettingsWindow() {
    if (!plugin_) {
        return;
    }

    // Header with metadata
    ImGui::TextUnformatted("SuiteSpot");
    ImGui::TextColored(UI::SettingsUI::HEADER_TEXT_COLOR, "By: Flicks Creations");
    ImGui::TextColored(UI::SettingsUI::HEADER_TEXT_COLOR, "Version: %s", plugin_version);

    bool enabledValue = plugin_->IsEnabled();
    int mapTypeValue = plugin_->GetMapType();
    bool autoQueueValue = plugin_->IsAutoQueueEnabled();
    bool trainingShuffleEnabledValue = plugin_->IsTrainingShuffleEnabled();
    int delayQueueSecValue = plugin_->GetDelayQueueSec();
    int delayFreeplaySecValue = plugin_->GetDelayFreeplaySec();
    int delayTrainingSecValue = plugin_->GetDelayTrainingSec();
    int delayWorkshopSecValue = plugin_->GetDelayWorkshopSec();
    int currentIndexValue = plugin_->GetCurrentIndex();
    int currentTrainingIndexValue = plugin_->GetCurrentTrainingIndex();
    int currentWorkshopIndexValue = plugin_->GetCurrentWorkshopIndex();

    // Only show status if enabled
    if (enabledValue) {
        ImGui::SameLine(UI::SettingsUI::STATUS_TEXT_POSITION_X);

        const char* modeNames[] = {"Freeplay", "Training", "Workshop"};
        std::string currentMap = "<none>";
        std::string queueDelayStr = std::to_string(delayQueueSecValue) + "s";
        std::string mapDelayStr = "0s";
        std::string shotProgressStr = "";
        const ImVec4 white = UI::SettingsUI::STATUS_SEPARATOR_COLOR;

        // Get current selection and appropriate delay
        if (mapTypeValue == 0) {
            if (!RLMaps.empty() && currentIndexValue >= 0 && currentIndexValue < (int)RLMaps.size()) {
                currentMap = RLMaps[currentIndexValue].name;
            }
            mapDelayStr = std::to_string(delayFreeplaySecValue) + "s";
        } else if (mapTypeValue == 1) {
            if (!RLTraining.empty() && currentTrainingIndexValue >= 0 && currentTrainingIndexValue < (int)RLTraining.size()) {
                currentMap = RLTraining[currentTrainingIndexValue].name + " (Shots:" + std::to_string(RLTraining[currentTrainingIndexValue].shotCount) + ")";
            }
            mapDelayStr = std::to_string(delayTrainingSecValue) + "s";

            if (trainingShuffleEnabledValue && plugin_->trainingPackMgr) {
                int shuffleCount = plugin_->trainingPackMgr->GetShuffleBagCount();
                // Only show shuffle indicator when we have a non-empty selection/bag.
                if (shuffleCount > 0) {
                    if (shuffleCount == 1) {
                        // Single pack in shuffle – show its name/shots.
                        auto shuffleBag = plugin_->trainingPackMgr->GetShuffleBagPacks();
                        if (!shuffleBag.empty()) {
                            const auto& entry = shuffleBag.front();
                            currentMap = entry.name + " (Shots:" + std::to_string(entry.shotCount) + ")";
                        }
                    } else {
                        currentMap = "Shuffle (" + std::to_string(shuffleCount) + " packs)";
                    }
                } // else fall back to current dropdown selection
            }

            // Real-time shot progress requires APIs not exposed by BakkesMod SDK
            // This will be added in Phase 2b when we have better access to training state
        } else if (mapTypeValue == 2) {
            if (!RLWorkshop.empty() && currentWorkshopIndexValue >= 0 && currentWorkshopIndexValue < (int)RLWorkshop.size()) {
                currentMap = RLWorkshop[currentWorkshopIndexValue].name;
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

    // Main tab bar for organizing settings
    if (ImGui::BeginTabBar("SuiteSpotTabs", ImGuiTabBarFlags_None)) {

        // ===== MAIN SETTINGS TAB =====
        // Check CVar validity before opening tab
        CVarWrapper enableCvar = plugin_->cvarManager->getCvar("suitespot_enabled");
        if (!enableCvar) {
            // CVar not available, skip this tab
        } else if (ImGui::BeginTabItem("Main Settings")) {
            ImGui::Spacing();

            // 1) Enable/Disable + Standalone window entry

            RenderGeneralTab(enabledValue, mapTypeValue);
            RenderAutoQueueTab(autoQueueValue, delayQueueSecValue);
            RenderMapSelectionTab(mapTypeValue, trainingShuffleEnabledValue, currentIndexValue,
                currentTrainingIndexValue, currentWorkshopIndexValue, delayFreeplaySecValue,
                delayTrainingSecValue, delayWorkshopSecValue);

            // Close Main Settings tab
            ImGui::EndTabItem();
        } // End Main Settings tab

        // ===== LOADOUT MANAGEMENT TAB =====
        if (ImGui::BeginTabItem("Loadout Management")) {
            if (plugin_->loadoutUI) {
            plugin_->loadoutUI->RenderLoadoutControls();
        }
            ImGui::EndTabItem();
        } // End Loadout Management tab

        // ===== TRAINING PACKS TAB =====
        if (ImGui::BeginTabItem("Training Packs")) {
            if (plugin_->trainingPackUI) {
            plugin_->trainingPackUI->RenderTrainingPackTab();
        }
            ImGui::EndTabItem();
        } // End Training Packs tab

        // Close the tab bar
        ImGui::EndTabBar();
    } // End tab bar
}

void SettingsUI::RenderGeneralTab(bool& enabledValue, int& mapTypeValue) {
    ImGui::BeginGroup();
    UI::Helpers::CheckboxWithCVar("Enable SuiteSpot", enabledValue, "suitespot_enabled",
        plugin_->cvarManager, "Enable/disable all SuiteSpot auto-loading and queuing features");
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
            UI::Helpers::SetCVarSafely("suitespot_map_type", mapTypeValue, plugin_->cvarManager);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Choose which map type loads after matches:\nFreeplay = Official | Training = Custom Packs | Workshop = Modded Maps");
    }
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();
}

void SettingsUI::RenderAutoQueueTab(bool& autoQueueValue, int& delayQueueSecValue) {
    UI::Helpers::CheckboxWithCVar("Auto-Queue Next Match", autoQueueValue, "suitespot_auto_queue",
        plugin_->cvarManager, "Automatically queue into the next match after the current match ends.\nQueue delay starts at match end, independent of map load.");

    UI::Helpers::InputIntWithRange("Delay Queue (sec)", delayQueueSecValue,
        UI::SettingsUI::DELAY_QUEUE_MIN_SECONDS, UI::SettingsUI::DELAY_QUEUE_MAX_SECONDS,
        UI::SettingsUI::DELAY_QUEUE_INPUT_WIDTH, "suitespot_delay_queue_sec", plugin_->cvarManager,
        "Wait this many seconds before queuing (independent of map load). Range: 0-300s", "0-300s");

    ImGui::Spacing();
    ImGui::Separator();
}

void SettingsUI::RenderMapSelectionTab(int mapTypeValue,
    bool trainingShuffleEnabledValue,
    int& currentIndexValue,
    int& currentTrainingIndexValue,
    int& currentWorkshopIndexValue,
    int& delayFreeplaySecValue,
    int& delayTrainingSecValue,
    int& delayWorkshopSecValue) {
    ImGui::TextUnformatted("Map Selection:");
    ImGui::Spacing();

    if (mapTypeValue == 0) {
        RenderFreeplayMode(currentIndexValue, delayFreeplaySecValue);
    } else if (mapTypeValue == 1) {
        RenderTrainingMode(trainingShuffleEnabledValue, currentTrainingIndexValue, delayTrainingSecValue);
    } else if (mapTypeValue == 2) {
        RenderWorkshopMode(currentWorkshopIndexValue, delayWorkshopSecValue);
    }
}

void SettingsUI::RenderFreeplayMode(int& currentIndexValue, int& delayFreeplaySecValue) {
    if (!RLMaps.empty()) {
        currentIndexValue = std::clamp(currentIndexValue, 0, static_cast<int>(RLMaps.size() - 1));
    } else {
        currentIndexValue = 0;
    }

    const char* freeplayLabel = RLMaps.empty() ? "<none>" : RLMaps[currentIndexValue].name.c_str();
    if (UI::Helpers::ComboWithTooltip("Freeplay Maps", freeplayLabel,
        "Select which stadium to load after matches", UI::SettingsUI::FREEPLAY_MAPS_DROPDOWN_WIDTH)) {
        // Use ImGuiListClipper to only render visible items for better performance
        ImGuiListClipper clipper;
        clipper.Begin((int)RLMaps.size());

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                bool selected = (row == currentIndexValue);
                if (ImGui::Selectable(RLMaps[row].name.c_str(), selected)) {
                    currentIndexValue = row;
                    UI::Helpers::SetCVarSafely("suitespot_current_freeplay_index", currentIndexValue, plugin_->cvarManager);
                }
            }
        }
        ImGui::EndCombo();
    }

    float rightEdge = ImGui::GetWindowContentRegionMax().x;
    float loadBtnWidth = ImGui::CalcTextSize("Load Now").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), rightEdge - loadBtnWidth));
    if (ImGui::Button("Load Now##freeplay")) {
        if (!RLMaps.empty() && currentIndexValue >= 0 && currentIndexValue < (int)RLMaps.size()) {
            std::string mapCode = RLMaps[currentIndexValue].code;
            SuiteSpot* plugin = plugin_;
            plugin_->gameWrapper->SetTimeout([plugin, mapCode](GameWrapper* gw) {
                plugin->cvarManager->executeCommand("load_freeplay " + mapCode);
            }, 0.0f);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Load the selected freeplay map immediately");
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Freeplay Settings:");
    UI::Helpers::InputIntWithRange("Delay Freeplay (sec)", delayFreeplaySecValue,
        UI::SettingsUI::DELAY_FREEPLAY_MIN_SECONDS, UI::SettingsUI::DELAY_FREEPLAY_MAX_SECONDS,
        UI::SettingsUI::DELAY_FREEPLAY_INPUT_WIDTH, "suitespot_delay_freeplay_sec", plugin_->cvarManager,
        "Wait this many seconds after match ends before loading Freeplay. Range: 0-300s", "0-300s");
}

void SettingsUI::RenderTrainingMode(bool trainingShuffleEnabledValue, int& currentTrainingIndexValue, int& delayTrainingSecValue) {
    if (!RLTraining.empty()) {
        currentTrainingIndexValue = std::clamp(currentTrainingIndexValue, 0, static_cast<int>(RLTraining.size() - 1));
    } else {
        currentTrainingIndexValue = 0;
    }

    const char* trainingLabel = "<none>";
    if (!RLTraining.empty() && currentTrainingIndexValue >= 0 && currentTrainingIndexValue < (int)RLTraining.size()) {
        trainingLabelBuf = RLTraining[currentTrainingIndexValue].name + " (Shots:" + std::to_string(RLTraining[currentTrainingIndexValue].shotCount) + ")";
        trainingLabel = trainingLabelBuf.c_str();
    }

    if (UI::Helpers::ComboWithTooltip("Training Packs", trainingLabel,
        "Select which training pack to load after matches", UI::SettingsUI::TRAINING_PACKS_DROPDOWN_WIDTH)) {
        // Use ImGuiListClipper to only render visible items for better performance
        // This is especially important for training packs since we have 500+ items
        ImGuiListClipper clipper;
        clipper.Begin((int)RLTraining.size());

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                bool selected = (row == currentTrainingIndexValue);
                // String concatenation only happens for visible items (~10-15) instead of all 500+
                std::string itemLabel = RLTraining[row].name + " (Shots:" + std::to_string(RLTraining[row].shotCount) + ")";
                if (ImGui::Selectable(itemLabel.c_str(), selected)) {
                    currentTrainingIndexValue = row;
                    UI::Helpers::SetCVarSafely("suitespot_current_training_index", currentTrainingIndexValue, plugin_->cvarManager);
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh Training##maps")) {
        if (plugin_->trainingPackMgr) {
            plugin_->trainingPackMgr->LoadPacksFromFile(plugin_->GetTrainingPacksPath());
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reload training packs from the JSON file");
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Now##training")) {
        std::string packCode;

        if (trainingShuffleEnabledValue && plugin_->trainingPackMgr && plugin_->mapManager) {
            auto shuffleBag = plugin_->trainingPackMgr->GetShuffleBagPacks();
            if (!shuffleBag.empty()) {
                int randomIdx = plugin_->mapManager->GetRandomTrainingMapIndex(shuffleBag);
                if (randomIdx >= 0 && randomIdx < static_cast<int>(shuffleBag.size())) {
                    packCode = shuffleBag[randomIdx].code;
                }
            }
        }

        if (packCode.empty() && !RLTraining.empty() &&
            currentTrainingIndexValue >= 0 && currentTrainingIndexValue < static_cast<int>(RLTraining.size())) {
            packCode = RLTraining[currentTrainingIndexValue].code;
        }

        if (!packCode.empty()) {
            SuiteSpot* plugin = plugin_;
            plugin_->gameWrapper->SetTimeout([plugin, packCode](GameWrapper* gw) {
                plugin->cvarManager->executeCommand("load_training " + packCode);
            }, 0.0f);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Load a training pack (from shuffle bag if enabled, otherwise selected)");
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Pack##training_toggle")) {
        showAddTrainingForm = !showAddTrainingForm;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show or hide the custom training pack form");
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::TextUnformatted("Training Settings:");
    UI::Helpers::InputIntWithRange("Delay Training (sec)", delayTrainingSecValue,
        UI::SettingsUI::DELAY_TRAINING_MIN_SECONDS, UI::SettingsUI::DELAY_TRAINING_MAX_SECONDS,
        UI::SettingsUI::DELAY_TRAINING_INPUT_WIDTH, "suitespot_delay_training_sec", plugin_->cvarManager,
        "Wait this many seconds after match ends before loading Training. Range: 0-300s", "0-300s");

    if (showAddTrainingForm) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Add Custom Training Pack:");
        ImGui::Spacing();

        ImGui::InputText("Training Map Code##input", newMapCode, IM_ARRAYSIZE(newMapCode), 0);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enter the code (e.g., 555F-7503-BBB9-E1E3)");
        }

        ImGui::InputText("Training Map Name##input", newMapName, IM_ARRAYSIZE(newMapName), 0);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enter a custom name for this pack");
        }

        if (ImGui::Button("Add Training Map")) {
            if (strlen(newMapCode) > 0 && strlen(newMapName) > 0) {
                std::string codeStr(newMapCode);
                bool isValidCode = true;

                int dashCount = 0;
                for (char c : codeStr) {
                    if (c == '-') {
                        dashCount++;
                    } else if (!isxdigit(static_cast<unsigned char>(c))) {
                        isValidCode = false;
                        break;
                    }
                }
                if (dashCount != 3) {
                    isValidCode = false;
                }

                if (!isValidCode) {
                    // Invalid code format - could show error here in future
                } else {
                    TrainingEntry newPack;
                    newPack.code = codeStr;
                    newPack.name = std::string(newMapName);
                    newPack.source = "custom";

                    if (plugin_->trainingPackMgr) {
                        plugin_->trainingPackMgr->AddCustomPack(newPack);
                    }
                    addPackStatus.ShowSuccess("Pack added!",
                        UI::SettingsUI::CUSTOM_PACK_SUCCESS_MESSAGE_DURATION,
                        UI::StatusMessage::DisplayMode::TimerWithFade);
                    newMapCode[0] = 0;
                    newMapName[0] = 0;
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Add this training pack to your collection");
        }

        ImGui::SameLine();
        addPackStatus.Render(ImGui::GetIO().DeltaTime);
    }
}

void SettingsUI::RenderWorkshopMode(int& currentWorkshopIndexValue, int& delayWorkshopSecValue) {
    if (!RLWorkshop.empty()) {
        currentWorkshopIndexValue = std::clamp(currentWorkshopIndexValue, 0, static_cast<int>(RLWorkshop.size() - 1));
    } else {
        currentWorkshopIndexValue = 0;
    }

    const char* workshopLabel = RLWorkshop.empty() ? "<none>" : RLWorkshop[currentWorkshopIndexValue].name.c_str();
    if (UI::Helpers::ComboWithTooltip("Workshop Maps", workshopLabel,
        "Select which workshop map to load after matches", UI::SettingsUI::WORKSHOP_MAPS_DROPDOWN_WIDTH)) {
        // Use ImGuiListClipper to only render visible items for better performance
        ImGuiListClipper clipper;
        clipper.Begin((int)RLWorkshop.size());

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                bool selected = (row == currentWorkshopIndexValue);
                if (ImGui::Selectable(RLWorkshop[row].name.c_str(), selected)) {
                    currentWorkshopIndexValue = row;
                    UI::Helpers::SetCVarSafely("suitespot_current_workshop_index", currentWorkshopIndexValue, plugin_->cvarManager);
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh Workshop##maps")) {
        std::string previousPath;
        if (currentWorkshopIndexValue >= 0 && currentWorkshopIndexValue < (int)RLWorkshop.size()) {
            previousPath = RLWorkshop[currentWorkshopIndexValue].filePath;
        }
        plugin_->LoadWorkshopMaps();
        if (!previousPath.empty()) {
            auto it = std::find_if(RLWorkshop.begin(), RLWorkshop.end(),
                [&](const WorkshopEntry& entry){ return entry.filePath == previousPath; });
            if (it != RLWorkshop.end()) {
                currentWorkshopIndexValue = static_cast<int>(std::distance(RLWorkshop.begin(), it));
                UI::Helpers::SetCVarSafely("suitespot_current_workshop_index", currentWorkshopIndexValue, plugin_->cvarManager);
            }
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Refresh the workshop map list");
    }

    ImGui::SameLine();
    if (ImGui::Button("Load Now##workshop")) {
        if (!RLWorkshop.empty() && currentWorkshopIndexValue >= 0 && currentWorkshopIndexValue < (int)RLWorkshop.size()) {
            std::string filePath = RLWorkshop[currentWorkshopIndexValue].filePath;
            SuiteSpot* plugin = plugin_;
            plugin_->gameWrapper->SetTimeout([plugin, filePath](GameWrapper* gw) {
                plugin->cvarManager->executeCommand("load_workshop \"" + filePath + "\"");
            }, 0.0f);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Load the selected workshop map immediately");
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
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set the root folder to scan for workshop maps (contains subfolders with .upk files).");
        }

        if (ImGui::Button("Save Workshop Source")) {
            std::filesystem::path workshopPath(workshopPathBuf);
            std::error_code ec;
            if (workshopPath.empty()) {
                LOG("SuiteSpot: Workshop path cannot be empty");
                // Could show error message here in future
            } else if (!std::filesystem::exists(workshopPath, ec)) {
                LOG("SuiteSpot: Workshop path does not exist: {}", workshopPathBuf);
                // Could show error message here in future
            } else if (!std::filesystem::is_directory(workshopPath, ec)) {
                LOG("SuiteSpot: Workshop path is not a directory: {}", workshopPathBuf);
                // Could show error message here in future
            } else {
                std::filesystem::path cfgPath = plugin_->GetWorkshopLoaderConfigPath();
                ec.clear();
                std::filesystem::create_directories(cfgPath.parent_path(), ec);
                std::ofstream cfg(cfgPath.string(), std::ios::trunc);
                if (cfg.is_open()) {
                    cfg << "MapsFolderPath=" << workshopPathBuf << "\n";
                    cfg.close();
                    workshopPathCache = workshopPathBuf;
                    plugin_->LoadWorkshopMaps();
                    if (!RLWorkshop.empty()) {
                        currentWorkshopIndexValue = std::clamp(currentWorkshopIndexValue, 0, static_cast<int>(RLWorkshop.size() - 1));
                    } else {
                        currentWorkshopIndexValue = 0;
                    }
                    addPackStatus.ShowSuccess("Workshop path saved!",
                        UI::SettingsUI::WORKSHOP_PATH_ERROR_MESSAGE_DURATION,
                        UI::StatusMessage::DisplayMode::TimerWithFade);
                } else {
                    LOG("SuiteSpot: Failed to write workshopmaploader.cfg");
                    // Could show error message here in future
                }
            }
        }

        ImGui::TreePop();
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Workshop Settings:");
    UI::Helpers::InputIntWithRange("Delay Workshop (sec)", delayWorkshopSecValue,
        UI::SettingsUI::DELAY_WORKSHOP_MIN_SECONDS, UI::SettingsUI::DELAY_WORKSHOP_MAX_SECONDS,
        UI::SettingsUI::DELAY_WORKSHOP_INPUT_WIDTH, "suitespot_delay_workshop_sec", plugin_->cvarManager,
        "Wait this many seconds after match ends before loading Workshop. Range: 0-300s", "0-300s");
}