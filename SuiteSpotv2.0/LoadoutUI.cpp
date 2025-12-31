#include "pch.h"

#include "LoadoutUI.h"
#include "SuiteSpot.h"
#include "UIConstants.h"
#include "UIHelpers.h"

LoadoutUI::LoadoutUI(SuiteSpot* plugin) : plugin_(plugin) {}

void LoadoutUI::RenderLoadoutControls() {
    ImGui::Spacing();

    auto* loadoutManager = plugin_->loadoutManager.get();
    if (loadoutManager) {
        if (!loadoutsInitialized) {
            loadoutNames = loadoutManager->GetLoadoutNames();
            loadoutManager->GetCurrentLoadoutName([this](std::string name) {
                currentLoadoutName = name;
            });
            loadoutsInitialized = true;
        }

        ImGui::TextColored(UI::LoadoutUI::SECTION_HEADER_COLOR, "Current Loadout:");
        ImGui::SameLine();
        if (currentLoadoutName.empty()) {
            ImGui::TextUnformatted("<Unknown>");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Loadout not detected yet. Refresh to check available presets.");
            }
        } else {
            ImGui::TextUnformatted(currentLoadoutName.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Your currently equipped loadout preset");
            }
        }

        ImGui::Spacing();

        if (loadoutNames.empty()) {
            ImGui::TextColored(UI::LoadoutUI::ERROR_WARNING_TEXT_COLOR, "No loadouts found. Open Garage to create presets, then click Refresh.");
        } else {
            const char* comboLabel = (selectedLoadoutIndex >= 0 && selectedLoadoutIndex < (int)loadoutNames.size()) ?
                loadoutNames[selectedLoadoutIndex].c_str() : "<Select loadout>";
            ImGui::SetNextItemWidth(UI::LoadoutUI::LOADOUT_SELECTOR_DROPDOWN_WIDTH);
            if (ImGui::BeginCombo("##loadout_combo", comboLabel)) {
                for (int i = 0; i < (int)loadoutNames.size(); ++i) {
                    bool isSelected = (i == selectedLoadoutIndex);
                    if (ImGui::Selectable(loadoutNames[i].c_str(), isSelected)) {
                        selectedLoadoutIndex = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Select a loadout preset to equip");
            }

            ImGui::SameLine();
            if (ImGui::Button("Apply Loadout")) {
                if (selectedLoadoutIndex >= 0 && selectedLoadoutIndex < (int)loadoutNames.size()) {
                    std::string selectedName = loadoutNames[selectedLoadoutIndex];
                    // Set immediate feedback or wait?
                    // Let's set a "Applying..." state if desired, or just wait for callback.
                    loadoutStatusText = "Applying...";
                    loadoutStatusColor = UI::LoadoutUI::APPLYING_STATUS_COLOR;
                    loadoutStatusTimer = UI::LoadoutUI::APPLYING_STATUS_DURATION; // Give it some time

                    loadoutManager->SwitchLoadout(selectedName, [this, selectedName](bool success) {
                        if (success) {
                            currentLoadoutName = selectedName;
                            loadoutStatusText = "Applied \"" + selectedName + "\"";
                            loadoutStatusColor = UI::LoadoutUI::SUCCESS_MESSAGE_COLOR;
                        } else {
                            loadoutStatusText = "Failed to apply loadout";
                            loadoutStatusColor = UI::LoadoutUI::ERROR_WARNING_TEXT_COLOR;
                        }
                        loadoutStatusTimer = UI::LoadoutUI::SUCCESS_MESSAGE_DURATION;
                    });
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Equip the selected loadout preset");
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh Loadouts")) {
            loadoutsInitialized = false;
            loadoutManager->RefreshLoadoutCache();
            loadoutNames = loadoutManager->GetLoadoutNames();
            loadoutManager->GetCurrentLoadoutName([this](std::string name) {
                currentLoadoutName = name;
            });
            selectedLoadoutIndex = 0;
            loadoutStatusText = "Loadouts refreshed";
            loadoutStatusColor = UI::LoadoutUI::REFRESH_MESSAGE_COLOR;
            loadoutStatusTimer = UI::LoadoutUI::REFRESH_MESSAGE_DURATION;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Refresh the list of available loadout presets");
        }

        ImGui::Spacing();
        ImGui::TextDisabled(("Available loadouts: " + std::to_string(loadoutNames.size())).c_str());

        UI::Helpers::ShowStatusMessage(loadoutStatusText, loadoutStatusColor,
            loadoutStatusTimer, ImGui::GetIO().DeltaTime);
    } else {
        ImGui::TextColored(UI::LoadoutUI::ERROR_WARNING_TEXT_COLOR, "LoadoutManager not initialized");
    }
}
