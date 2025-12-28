#include "pch.h"

#include "LoadoutUI.h"
#include "SuiteSpot.h"

LoadoutUI::LoadoutUI(SuiteSpot* plugin) : plugin_(plugin) {}

void LoadoutUI::RenderLoadoutControls() {
    ImGui::Spacing();

    auto* loadoutManager = plugin_->loadoutManager.get();
    if (loadoutManager) {
        if (!loadoutsInitialized) {
            loadoutNames = loadoutManager->GetLoadoutNames();
            currentLoadoutName = loadoutManager->GetCurrentLoadoutName();
            loadoutsInitialized = true;
        }

        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Current Loadout:");
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
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No loadouts found. Open Garage to create presets, then click Refresh.");
        } else {
            const char* comboLabel = (selectedLoadoutIndex >= 0 && selectedLoadoutIndex < (int)loadoutNames.size()) ?
                loadoutNames[selectedLoadoutIndex].c_str() : "<Select loadout>";
            ImGui::SetNextItemWidth(220);
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
                    bool success = loadoutManager->SwitchLoadout(selectedName);
                    if (success) {
                        currentLoadoutName = selectedName;
                        loadoutStatusText = "Applied \"" + selectedName + "\"";
                        loadoutStatusColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                    } else {
                        loadoutStatusText = "Failed to apply loadout";
                        loadoutStatusColor = ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
                    }
                    loadoutStatusTimer = 3.0f;
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
            currentLoadoutName = loadoutManager->GetCurrentLoadoutName();
            selectedLoadoutIndex = 0;
            loadoutStatusText = "Loadouts refreshed";
            loadoutStatusColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
            loadoutStatusTimer = 2.5f;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Refresh the list of available loadout presets");
        }

        ImGui::Spacing();
        ImGui::TextDisabled(("Available loadouts: " + std::to_string(loadoutNames.size())).c_str());

        if (loadoutStatusTimer > 0.0f && !loadoutStatusText.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(loadoutStatusColor, "%s", loadoutStatusText.c_str());
            loadoutStatusTimer -= ImGui::GetIO().DeltaTime;
            if (loadoutStatusTimer <= 0.0f) {
                loadoutStatusTimer = 0.0f;
                loadoutStatusText.clear();
            }
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "LoadoutManager not initialized");
    }
}
