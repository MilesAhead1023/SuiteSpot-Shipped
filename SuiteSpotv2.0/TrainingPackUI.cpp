#include "pch.h"
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include "TrainingPackUI.h"
#include "TrainingPackManager.h"
#include "SuiteSpot.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

// Helper function for sortable column headers with visual indicators
namespace {
    bool SortableColumnHeader(const char* label, int columnIndex, int& currentSortColumn, bool& sortAscending) {
        // Display label with sort indicator if this column is active
        char buffer[256];
        if (currentSortColumn == columnIndex) {
            snprintf(buffer, sizeof(buffer), "%s %s", label, sortAscending ? "▲" : "▼");
        } else {
            snprintf(buffer, sizeof(buffer), "%s", label);
        }

        bool clicked = ImGui::Selectable(buffer, currentSortColumn == columnIndex, ImGuiSelectableFlags_DontClosePopups);
        if (clicked) {
            if (currentSortColumn == columnIndex) {
                sortAscending = !sortAscending;
            } else {
                currentSortColumn = columnIndex;
                sortAscending = true;
            }
        }
        return clicked;
    }
}

TrainingPackUI::TrainingPackUI(SuiteSpot* plugin) : plugin_(plugin) {}

void TrainingPackUI::RenderTrainingPackTab() {
    ImGui::Spacing();
    const auto* manager = plugin_->trainingPackMgr;
    static const std::vector<TrainingEntry> emptyPacks;
    static const std::string emptyString;
    const auto& packs = manager ? manager->GetPacks() : emptyPacks;
    const int packCount = manager ? manager->GetPackCount() : 0;
    const auto& lastUpdated = manager ? manager->GetLastUpdated() : emptyString;
    const bool scraping = manager && manager->IsScrapingInProgress();
    bool trainingShuffleEnabledValue = plugin_->IsTrainingShuffleEnabled();

    // ===== HEADER SECTION =====
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Training Pack Browser");
    ImGui::Spacing();

    // Status line: pack count and last updated
    if (packCount > 0) {
        ImGui::Text("Loaded: %d packs", packCount);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), " | Last updated: %s", lastUpdated.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No packs loaded - click 'Scrape Packs' to download");
    }

    // Control buttons
    ImGui::SameLine();
    float buttonX = ImGui::GetWindowWidth() - 280;
    if (buttonX > ImGui::GetCursorPosX()) {
        ImGui::SetCursorPosX(buttonX);
    }

    if (scraping) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Scraping...");
    } else {
        if (ImGui::Button("Scrape Packs")) {
            plugin_->ScrapeAndLoadTrainingPacks();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Download latest training packs from online source (~2-3 minutes)");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reload Cache")) {
        plugin_->LoadTrainingPacksFromFile(plugin_->GetTrainingPacksPath());
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reload packs from cached json file");
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Early return if no packs loaded
    if (packs.empty()) {
        ImGui::TextWrapped("No packs available. Click 'Scrape Packs' to download the training pack database, or add your own custom packs below.");
        return;
    }

    // ===== SHUFFLE BAG STATUS & CONTROLS =====
    if (ImGui::CollapsingHeader("Shuffle Bag Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
        int shufflePackCount = plugin_->trainingPackMgr ? plugin_->trainingPackMgr->GetShuffleBagCount() : 0;

        if (shufflePackCount > 0) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Current Bag: %d pack%s", shufflePackCount, shufflePackCount == 1 ? "" : "s");

            ImGui::SameLine();
            if (ImGui::Button("Start Shuffle Training")) {
                // Enable plugin, set mode to Training, and enable Shuffle
                plugin_->cvarManager->getCvar("suitespot_enabled").setValue(1);

                // Training
                plugin_->cvarManager->getCvar("suitespot_map_type").setValue(1);

                trainingShuffleEnabledValue = true;
                plugin_->cvarManager->getCvar("suitespot_training_shuffle").setValue(1);

                LOG("SuiteSpot: Shuffle training started with " + std::to_string(shufflePackCount) + " packs");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable SuiteSpot, switch to Training mode, and enable Shuffle using your current bag.");
            }

            ImGui::SameLine();
            if (ImGui::Button("Clear Bag")) {
                // Clear shuffle bag by toggling all packs that are in it
                if (plugin_->trainingPackMgr) {
                    auto shufflePacks = plugin_->trainingPackMgr->GetShuffleBagPacks();
                    for (const auto& pack : shufflePacks) {
                        plugin_->trainingPackMgr->ToggleShuffleBag(pack.code);
                    }
                }
                LOG("SuiteSpot: Shuffle bag cleared");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Remove all packs from the shuffle bag.");
            }

            // Show Shuffle toggle here as well for convenience
            ImGui::SameLine(400);
            if (ImGui::Checkbox("Shuffle Active", &trainingShuffleEnabledValue)) {
                plugin_->cvarManager->getCvar("suitespot_training_shuffle").setValue(trainingShuffleEnabledValue);
            }
        } else {
            ImGui::TextDisabled("Shuffle Bag: Empty");
            ImGui::TextWrapped("Add packs to your bag using the '+Shuffle' buttons in the table below to create a rotation.");
        }

        ImGui::Spacing();
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ===== ADD CUSTOM PACK SECTION =====
    RenderCustomPackForm();

    ImGui::Separator();
    ImGui::Spacing();

    // Early return if no packs loaded
    if (packs.empty()) {
        ImGui::TextWrapped("No packs available. Click 'Scrape Packs' to download the training pack database, or add your own custom packs above.");
        return;
    }

    // ===== FILTER & SEARCH CONTROLS =====
    ImGui::TextUnformatted("Search & Filters:");
    ImGui::Spacing();

    bool filtersChanged = (strcmp(packSearchText, lastSearchText) != 0) ||
                          (packDifficultyFilter != lastDifficultyFilter) ||
                          (packTagFilter != lastTagFilter) ||
                          (packMinShots != lastMinShots) ||
                          (packSortColumn != lastSortColumn) ||
                          (packSortAscending != lastSortAscending);

    // Dynamic width calculation for filters
    float availWidth = ImGui::GetContentRegionAvail().x;
    float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
    
    // Allocate widths: Search (40%), Difficulty (25%), Shots (35%)
    // Adjust logic to ensure they fit on one line if possible
    float searchW = (availWidth * 0.40f) - itemSpacing;
    float diffW = (availWidth * 0.25f) - itemSpacing;
    float shotsW = (availWidth * 0.35f) - itemSpacing;

    // Minimum constraints
    if (searchW < 150) searchW = 150;
    if (diffW < 120) diffW = 120;
    if (shotsW < 150) shotsW = 150;

    // Search box
    ImGui::SetNextItemWidth(searchW);
    if (ImGui::InputText("##search", packSearchText, IM_ARRAYSIZE(packSearchText))) {
        filtersChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Search by pack name, creator, or tag");
    }

    // Difficulty filter
    ImGui::SameLine();
    ImGui::SetNextItemWidth(diffW);
    const char* difficulties[] = {"All", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
    if (ImGui::BeginCombo("##difficulty", packDifficultyFilter.c_str())) {
        for (int i = 0; i < IM_ARRAYSIZE(difficulties); i++) {
            bool selected = (packDifficultyFilter == difficulties[i]);
            if (ImGui::Selectable(difficulties[i], selected)) {
                packDifficultyFilter = difficulties[i];
                filtersChanged = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Filter by difficulty level");
    }

    // Shot count range filter
    ImGui::SameLine();
    ImGui::SetNextItemWidth(shotsW);
    if (ImGui::SliderInt("Min Shots", &packMinShots, 0, 50)) {
        filtersChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Minimum number of shots in pack");
    }

    // Tag filter dropdown (second row)
    ImGui::SetNextItemWidth(200);

    if (!tagsInitialized || lastPackCount != packCount) {
        if (manager) {
            manager->BuildAvailableTags(availableTags);
        } else {
            availableTags.clear();
            availableTags.push_back("All Tags");
        }
        tagsInitialized = true;
        lastPackCount = packCount;
    }

    std::string displayTag = packTagFilter.empty() ? "All Tags" : packTagFilter;
    if (ImGui::BeginCombo("##tagfilter", displayTag.c_str())) {
        for (const auto& tag : availableTags) {
            bool selected = (tag == displayTag);
            if (ImGui::Selectable(tag.c_str(), selected)) {
                packTagFilter = (tag == "All Tags") ? "" : tag;
                filtersChanged = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Filter by tag");
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Filters")) {
        packSearchText[0] = '\0';
        packDifficultyFilter = "All";
        packTagFilter = "";
        packMinShots = 0;
        packMaxShots = 100;
        filtersChanged = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ===== FILTERED & SORTED PACK LIST (cached) =====

    // Rebuild filtered list only when needed
    if (filtersChanged) {
        if (manager) {
            manager->FilterAndSortPacks(packSearchText, packDifficultyFilter, packTagFilter,
                packMinShots, packSortColumn, packSortAscending, filteredPacks);
        } else {
            filteredPacks.clear();
        }

        // Update cached filter state
        strncpy_s(lastSearchText, packSearchText, sizeof(lastSearchText) - 1);
        lastDifficultyFilter = packDifficultyFilter;
        lastTagFilter = packTagFilter;
        lastMinShots = packMinShots;
        lastSortColumn = packSortColumn;
        lastSortAscending = packSortAscending;
        
        // Flag to recalculate column widths
        columnWidthsDirty = true;
    }

    // Display filtered count
    ImGui::Text("Showing %d of %d packs", (int)filteredPacks.size(), packCount);
    ImGui::Spacing();

    // ===== TABLE WITH RESIZABLE COLUMNS (ImGui 1.75 Columns API) =====
    ImGui::Separator();

    // Reduced columns to 6 (Removed Creator and Tags)
    ImGui::Columns(6, "PackColumns", true);

    // Apply optimal column widths if dirty
    if (columnWidthsDirty) {
        CalculateOptimalColumnWidths();
        for (int i = 0; i < 6 && i < (int)columnWidths.size(); i++) {
            ImGui::SetColumnWidth(i, columnWidths[i]);
        }
        columnWidthsDirty = false;
    }

    // ===== HEADER ROW WITH SORT INDICATORS =====
    ImGui::Separator();

    // Name column header (Sort ID 0)
    if (SortableColumnHeader("Name", 0, packSortColumn, packSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Difficulty column header (Sort ID 2)
    if (SortableColumnHeader("Difficulty", 2, packSortColumn, packSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Shots column header (Sort ID 3)
    if (SortableColumnHeader("Shots", 3, packSortColumn, packSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Likes column header (Sort ID 4)
    if (SortableColumnHeader("Likes", 4, packSortColumn, packSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Plays column header (Sort ID 5)
    if (SortableColumnHeader("Plays", 5, packSortColumn, packSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Actions column header (non-sortable)
    ImGui::TextUnformatted("Actions");
    ImGui::NextColumn();

    ImGui::Separator();

    // ===== RENDER PACK ROWS =====
    ImGuiListClipper clipper;
    clipper.Begin((int)filteredPacks.size());

    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            const auto& pack = filteredPacks[row];

            // Name column
            ImGui::TextUnformatted(pack.name.c_str());
            if (ImGui::IsItemHovered()) {
                std::string tooltip = "";
                if (!pack.staffComments.empty()) tooltip += pack.staffComments + "\n";
                if (!pack.creator.empty()) tooltip += "Creator: " + pack.creator + "\n"; // Moved creator to tooltip
                if (!pack.tags.empty()) {
                    tooltip += "Tags: ";
                    for (size_t i = 0; i < pack.tags.size(); i++) {
                        if (i > 0) tooltip += ", ";
                        tooltip += pack.tags[i];
                    }
                }
                if (!tooltip.empty()) ImGui::SetTooltip("%s", tooltip.c_str());
            }
            ImGui::NextColumn();

            // Difficulty column with color coding
            ImVec4 diffColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
            if (pack.difficulty == "Bronze") diffColor = ImVec4(0.8f, 0.5f, 0.2f, 1.0f);
            else if (pack.difficulty == "Silver") diffColor = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            else if (pack.difficulty == "Gold") diffColor = ImVec4(1.0f, 0.84f, 0.0f, 1.0f);
            else if (pack.difficulty == "Platinum") diffColor = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
            else if (pack.difficulty == "Diamond") diffColor = ImVec4(0.4f, 0.4f, 1.0f, 1.0f);
            else if (pack.difficulty == "Champion") diffColor = ImVec4(0.8f, 0.3f, 0.8f, 1.0f);
            else if (pack.difficulty == "Grand Champion") diffColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            else if (pack.difficulty == "Supersonic Legend") diffColor = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
            ImGui::TextColored(diffColor, "%s", pack.difficulty.c_str());
            ImGui::NextColumn();

            // Shots column
            ImGui::Text("%d", pack.shotCount);
            ImGui::NextColumn();

            // Likes column
            ImGui::Text("%d", pack.likes);
            ImGui::NextColumn();

            // Plays column
            ImGui::Text("%d", pack.plays);
            ImGui::NextColumn();

            // Actions column

            // Vid button
            if (!pack.videoUrl.empty()) {
                std::string watchLabel = "Vid##" + std::to_string(row);
                if (ImGui::SmallButton(watchLabel.c_str())) {
                    ShellExecuteA(NULL, "open", pack.videoUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Watch preview video");
                }
                ImGui::SameLine();
            }

            // Play button
            std::string loadLabel = "Play##" + std::to_string(row);
            if (ImGui::SmallButton(loadLabel.c_str())) {
                SuiteSpot* plugin = plugin_;
                plugin_->gameWrapper->SetTimeout([plugin, pack](GameWrapper* gw) {
                    std::string cmd = "load_training " + pack.code;
                    plugin->cvarManager->executeCommand(cmd);
                    LOG("SuiteSpot: Loading training pack: " + pack.name);
                }, 0.0f);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Load this pack now");
            }

            ImGui::SameLine();

            // +Bag button
            std::string shuffleLabel = "+Bag##" + std::to_string(row);
            bool inShuffleBag = pack.inShuffleBag;

            if (inShuffleBag) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            }

            if (ImGui::SmallButton(shuffleLabel.c_str())) {
                if (plugin_->trainingPackMgr) {
                    plugin_->trainingPackMgr->ToggleShuffleBag(pack.code);
                }
            }

            if (inShuffleBag) {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(inShuffleBag ? "Remove from shuffle bag" : "Add to shuffle bag");
            }

            // Delete button
            if (pack.source == "custom") {
                ImGui::SameLine();
                std::string deleteLabel = "X##del" + std::to_string(row);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
                if (ImGui::SmallButton(deleteLabel.c_str())) {
                    if (plugin_->trainingPackMgr) {
                        plugin_->trainingPackMgr->DeletePack(pack.code);
                        LOG("SuiteSpot: Deleted custom pack: " + pack.name);
                    }
                }
                ImGui::PopStyleColor(2);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Delete this custom pack");
                }
            }

            ImGui::NextColumn();
        }
    }

    // End columns
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Spacing();
}

bool TrainingPackUI::ValidatePackCode(const char* code) const {
    if (strlen(code) != 19) return false;
    for (int i = 0; i < 19; i++) {
        if (i == 4 || i == 9 || i == 14) {
            if (code[i] != '-') return false;
        } else {
            if (!isalnum(static_cast<unsigned char>(code[i]))) return false;
        }
    }
    return true;
}

void TrainingPackUI::ClearCustomPackForm() {
    customPackCode[0] = '\0';
    customPackName[0] = '\0';
    customPackCreator[0] = '\0';
    customPackDifficulty = 0;
    customPackShotCount = 10;
    customPackTags[0] = '\0';
    customPackNotes[0] = '\0';
    customPackVideoUrl[0] = '\0';
    customPackError.clear();
    customPackSuccess = false;
}

void TrainingPackUI::RenderCustomPackForm() {
    if (ImGui::CollapsingHeader("Add Custom Pack")) {
        ImGui::Indent(10.0f);
        ImGui::Spacing();

        if (customPackSuccess) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Pack added successfully!");
            ImGui::SameLine();
            if (ImGui::SmallButton("Dismiss")) {
                customPackSuccess = false;
            }
            ImGui::Spacing();
        }
        if (!customPackError.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", customPackError.c_str());
            ImGui::Spacing();
        }

        ImGui::TextUnformatted("Code *");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(XXXX-XXXX-XXXX-XXXX)");
        ImGui::SetNextItemWidth(220);
        if (ImGui::InputText("##customcode", customPackCode, IM_ARRAYSIZE(customPackCode))) {
            std::string raw;
            for (int i = 0; customPackCode[i]; i++) {
                char c = customPackCode[i];
                if (isalnum(static_cast<unsigned char>(c))) {
                    raw += static_cast<char>(toupper(static_cast<unsigned char>(c)));
                }
            }
            if (raw.length() > 16) raw = raw.substr(0, 16);
            std::string formatted;
            for (size_t i = 0; i < raw.length(); i++) {
                if (i > 0 && i % 4 == 0) formatted += '-';
                formatted += raw[i];
            }
            strncpy_s(customPackCode, formatted.c_str(), sizeof(customPackCode) - 1);
        }

        ImGui::TextUnformatted("Name *");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##customname", customPackName, IM_ARRAYSIZE(customPackName));

        ImGui::TextUnformatted("Creator");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##customcreator", customPackCreator, IM_ARRAYSIZE(customPackCreator));

        ImGui::TextUnformatted("Difficulty");
        ImGui::SetNextItemWidth(150);
        const char* difficulties[] = {"Unknown", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
        ImGui::Combo("##customdifficulty", &customPackDifficulty, difficulties, IM_ARRAYSIZE(difficulties));

        ImGui::TextUnformatted("Shot Count");
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("##customshots", &customPackShotCount, 1, 50);

        ImGui::TextUnformatted("Tags");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(comma-separated)");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##customtags", customPackTags, IM_ARRAYSIZE(customPackTags));

        ImGui::TextUnformatted("Notes");
        ImGui::InputTextMultiline("##customnotes", customPackNotes, IM_ARRAYSIZE(customPackNotes), ImVec2(400, 60));

        ImGui::TextUnformatted("Video URL");
        ImGui::SetNextItemWidth(350);
        ImGui::InputText("##customvideo", customPackVideoUrl, IM_ARRAYSIZE(customPackVideoUrl));

        ImGui::Spacing();

        if (ImGui::Button("Add Pack", ImVec2(100, 0))) {
            customPackError.clear();
            customPackSuccess = false;
            if (strlen(customPackCode) == 0) {
                customPackError = "Pack code is required";
            } else if (!ValidatePackCode(customPackCode)) {
                customPackError = "Invalid code format. Expected: XXXX-XXXX-XXXX-XXXX";
            } else if (strlen(customPackName) == 0) {
                customPackError = "Pack name is required";
            } else {
                TrainingEntry pack;
                pack.code = customPackCode;
                pack.name = customPackName;
                pack.creator = strlen(customPackCreator) > 0 ? customPackCreator : "Unknown";
                const char* difficultyNames[] = {"Unknown", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
                pack.difficulty = difficultyNames[customPackDifficulty];
                pack.shotCount = customPackShotCount;
                if (strlen(customPackTags) > 0) {
                    std::string tagsStr = customPackTags;
                    size_t start = 0;
                    size_t end = tagsStr.find(',');
                    while (end != std::string::npos) {
                        std::string tag = tagsStr.substr(start, end - start);
                        size_t first = tag.find_first_not_of(" \t");
                        size_t last = tag.find_last_not_of(" \t");
                        if (first != std::string::npos) {
                            pack.tags.push_back(tag.substr(first, last - first + 1));
                        }
                        start = end + 1;
                        end = tagsStr.find(',', start);
                    }
                    std::string tag = tagsStr.substr(start);
                    size_t first = tag.find_first_not_of(" \t");
                    size_t last = tag.find_last_not_of(" \t");
                    if (first != std::string::npos) {
                        pack.tags.push_back(tag.substr(first, last - first + 1));
                    }
                }
                pack.staffComments = customPackNotes;
                pack.videoUrl = customPackVideoUrl;
                pack.source = "custom";
                pack.inShuffleBag = false;
                pack.isModified = false;
                if (plugin_->trainingPackMgr) {
                    if (plugin_->trainingPackMgr->AddCustomPack(pack)) {
                        customPackSuccess = true;
                        ClearCustomPackForm();
                        LOG("SuiteSpot: Added custom pack: " + pack.name);
                    } else {
                        customPackError = "Pack with this code already exists";
                    }
                } else {
                    customPackError = "Pack manager not initialized";
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(80, 0))) {
            ClearCustomPackForm();
        }
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "* Required fields");
        ImGui::Unindent(10.0f);
        ImGui::Spacing();
    }
}

void TrainingPackUI::CalculateOptimalColumnWidths() {
    // Initialize widths with header widths + padding
    columnWidths.assign(6, 40.0f); // Minimum base width

    auto UpdateMax = [&](int col, const char* text) {
        float w = ImGui::CalcTextSize(text).x + 20.0f; // + Padding
        if (w > columnWidths[col]) columnWidths[col] = w;
    };

    // Headers
    UpdateMax(0, "Name");
    UpdateMax(1, "Difficulty");
    UpdateMax(2, "Shots");
    UpdateMax(3, "Likes");
    UpdateMax(4, "Plays");
    UpdateMax(5, "Actions");

    // Content
    for (const auto& pack : filteredPacks) {
        UpdateMax(0, pack.name.c_str());
        UpdateMax(1, pack.difficulty.c_str());
        UpdateMax(2, std::to_string(pack.shotCount).c_str());
        UpdateMax(3, std::to_string(pack.likes).c_str());
        UpdateMax(4, std::to_string(pack.plays).c_str());
        
        // Actions column approximation
        // Vid (30) + Spacing(8) + Play(30) + Spacing(8) + +Bag(30) + Spacing(8) + Padding(20)
        float actionsW = 150.0f; 
        if (actionsW > columnWidths[5]) columnWidths[5] = actionsW;
    }
    
    // Safety clamp for very long names
    if (columnWidths[0] > 400.0f) columnWidths[0] = 400.0f;
}