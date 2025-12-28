#include "pch.h"
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include "PrejumpUI.h"
#include "PrejumpPackManager.h"
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

PrejumpUI::PrejumpUI(SuiteSpot* plugin) : plugin_(plugin) {}

void PrejumpUI::RenderPrejumpTab() {
    ImGui::Spacing();
    const auto* manager = plugin_->prejumpMgr;
    static const std::vector<TrainingEntry> emptyPacks;
    static const std::string emptyString;
    const auto& packs = manager ? manager->GetPacks() : emptyPacks;
    const int packCount = manager ? manager->GetPackCount() : 0;
    const auto& lastUpdated = manager ? manager->GetLastUpdated() : emptyString;
    const bool scraping = manager && manager->IsScrapingInProgress();
    bool trainingShuffleEnabledValue = plugin_->IsTrainingShuffleEnabled();

    // ===== HEADER SECTION =====
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Prejump.com Training Pack Browser");
    ImGui::Spacing();

    // Status line: pack count and last updated
    if (packCount > 0) {
        ImGui::Text("Loaded: %d packs", packCount);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), " | Last updated: %s", lastUpdated.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No packs loaded - click 'Scrape Prejump' to download");
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
        if (ImGui::Button("Scrape Prejump")) {
            plugin_->ScrapeAndLoadPrejumpPacks();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Download latest training packs from prejump.com (~2-3 minutes)");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reload Cache")) {
        plugin_->LoadPrejumpPacksFromFile(plugin_->GetPrejumpPacksPath());
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reload packs from cached prejump_packs.json file");
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Early return if no packs loaded
    if (packs.empty()) {
        ImGui::TextWrapped("No packs available. Click 'Scrape Prejump' to download the training pack database from prejump.com.");
        return;
    }

    // ===== SHUFFLE BAG STATUS & CONTROLS =====
    if (ImGui::CollapsingHeader("Shuffle Bag Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
        int shufflePackCount = plugin_->prejumpMgr ? plugin_->prejumpMgr->GetShuffleBagCount() : 0;

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
                if (plugin_->prejumpMgr) {
                    auto shufflePacks = plugin_->prejumpMgr->GetShuffleBagPacks();
                    for (const auto& pack : shufflePacks) {
                        plugin_->prejumpMgr->ToggleShuffleBag(pack.code);
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
        ImGui::TextWrapped("No packs available. Click 'Scrape Prejump' to download the training pack database from prejump.com, or add your own custom packs above.");
        return;
    }

    // ===== FILTER & SEARCH CONTROLS =====
    ImGui::TextUnformatted("Search & Filters:");
    ImGui::Spacing();

    bool filtersChanged = (strcmp(prejumpSearchText, lastSearchText) != 0) ||
                          (prejumpDifficultyFilter != lastDifficultyFilter) ||
                          (prejumpTagFilter != lastTagFilter) ||
                          (prejumpMinShots != lastMinShots) ||
                          (prejumpSortColumn != lastSortColumn) ||
                          (prejumpSortAscending != lastSortAscending);

    // Search box
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputText("##search", prejumpSearchText, IM_ARRAYSIZE(prejumpSearchText))) {
        filtersChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Search by pack name, creator, or tag");
    }

    // Difficulty filter
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    const char* difficulties[] = {"All", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
    if (ImGui::BeginCombo("##difficulty", prejumpDifficultyFilter.c_str())) {
        for (int i = 0; i < IM_ARRAYSIZE(difficulties); i++) {
            bool selected = (prejumpDifficultyFilter == difficulties[i]);
            if (ImGui::Selectable(difficulties[i], selected)) {
                prejumpDifficultyFilter = difficulties[i];
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
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Min Shots", &prejumpMinShots, 0, 50)) {
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

    std::string displayTag = prejumpTagFilter.empty() ? "All Tags" : prejumpTagFilter;
    if (ImGui::BeginCombo("##tagfilter", displayTag.c_str())) {
        for (const auto& tag : availableTags) {
            bool selected = (tag == displayTag);
            if (ImGui::Selectable(tag.c_str(), selected)) {
                prejumpTagFilter = (tag == "All Tags") ? "" : tag;
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
        prejumpSearchText[0] = '\0';
        prejumpDifficultyFilter = "All";
        prejumpTagFilter = "";
        prejumpMinShots = 0;
        prejumpMaxShots = 100;
        filtersChanged = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ===== FILTERED & SORTED PACK LIST (cached) =====

    // Rebuild filtered list only when needed
    if (filtersChanged) {
        if (manager) {
            manager->FilterAndSortPacks(prejumpSearchText, prejumpDifficultyFilter, prejumpTagFilter,
                prejumpMinShots, prejumpSortColumn, prejumpSortAscending, filteredPacks);
        } else {
            filteredPacks.clear();
        }

        // Update cached filter state
        strncpy_s(lastSearchText, prejumpSearchText, sizeof(lastSearchText) - 1);
        lastDifficultyFilter = prejumpDifficultyFilter;
        lastTagFilter = prejumpTagFilter;
        lastMinShots = prejumpMinShots;
        lastSortColumn = prejumpSortColumn;
        lastSortAscending = prejumpSortAscending;
    }

    // Display filtered count
    ImGui::Text("Showing %d of %d packs", (int)filteredPacks.size(), packCount);
    ImGui::Spacing();

    // ===== TABLE WITH RESIZABLE COLUMNS (ImGui 1.75 Columns API) =====
    // No BeginChild needed - we're already inside the settings tab

    ImGui::Separator();

    // Setup columns (ImGui 1.75 compatible)
    ImGui::Columns(8, "PrejumpColumns", true);  // true = resizable borders

    // Don't set initial column widths - let ImGui auto-distribute across full tab width
    // Users can resize individual columns as needed

    // ===== HEADER ROW WITH SORT INDICATORS =====
    ImGui::Separator();

    // Name column header
    if (SortableColumnHeader("Name", 0, prejumpSortColumn, prejumpSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Creator column header
    if (SortableColumnHeader("Creator", 1, prejumpSortColumn, prejumpSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Difficulty column header
    if (SortableColumnHeader("Difficulty", 2, prejumpSortColumn, prejumpSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Shots column header
    if (SortableColumnHeader("Shots", 3, prejumpSortColumn, prejumpSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Tags column header (non-sortable)
    ImGui::TextUnformatted("Tags");
    ImGui::NextColumn();

    // Likes column header
    if (SortableColumnHeader("Likes", 5, prejumpSortColumn, prejumpSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Plays column header
    if (SortableColumnHeader("Plays", 6, prejumpSortColumn, prejumpSortAscending)) {
        filtersChanged = true;
    }
    ImGui::NextColumn();

    // Actions column header (non-sortable)
    ImGui::TextUnformatted("Actions");
    ImGui::NextColumn();

    ImGui::Separator();

    // ===== RENDER PACK ROWS =====
    for (size_t row = 0; row < filteredPacks.size(); row++) {
        const auto& pack = filteredPacks[row];

        // Name column
        ImGui::TextUnformatted(pack.name.c_str());
        if (ImGui::IsItemHovered() && !pack.staffComments.empty()) {
            ImGui::SetTooltip("%s", pack.staffComments.c_str());
        }
        ImGui::NextColumn();

        // Creator column
        ImGui::TextUnformatted(pack.creator.c_str());
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

        // Tags column (truncated)
        if (!pack.tags.empty()) {
            std::string tagsStr;
            for (size_t i = 0; i < std::min(size_t(2), pack.tags.size()); i++) {
                if (i > 0) tagsStr += ", ";
                tagsStr += pack.tags[i];
            }
            if (pack.tags.size() > 2) {
                tagsStr += "...";
            }
            ImGui::TextUnformatted(tagsStr.c_str());

            if (ImGui::IsItemHovered() && pack.tags.size() > 2) {
                std::string allTags;
                for (size_t i = 0; i < pack.tags.size(); i++) {
                    if (i > 0) allTags += ", ";
                    allTags += pack.tags[i];
                }
                ImGui::SetTooltip("%s", allTags.c_str());
            }
        }
        ImGui::NextColumn();

        // Likes column
        ImGui::Text("%d", pack.likes);
        ImGui::NextColumn();

        // Plays column
        ImGui::Text("%d", pack.plays);
        ImGui::NextColumn();

        // Actions column - NEW: Watch Video, Load Now and Add to Shuffle

        // Watch Video button
        if (!pack.videoUrl.empty()) {
            std::string watchLabel = "Watch##" + std::to_string(row);
            if (ImGui::SmallButton(watchLabel.c_str())) {
                // Open URL in default browser
                // ShellExecute is available via Windows headers in pch.h
                ShellExecuteA(NULL, "open", pack.videoUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Watch preview video");
            }
            ImGui::SameLine();
        }

        // Load Now button
        std::string loadLabel = "Load##" + std::to_string(row);
        if (ImGui::SmallButton(loadLabel.c_str())) {
            // Directly load the training pack
            SuiteSpot* plugin = plugin_;
            plugin_->gameWrapper->SetTimeout([plugin, pack](GameWrapper* gw) {
                std::string cmd = "load_training " + pack.code;
                plugin->cvarManager->executeCommand(cmd);
                LOG("SuiteSpot: Loading prejump pack: " + pack.name);
            }, 0.0f);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Load this pack now");
        }

        ImGui::SameLine();

        // Add to Shuffle button with visual feedback
        std::string shuffleLabel = "+Shuffle##" + std::to_string(row);

        // Check if pack already in shuffle bag (using inShuffleBag field)
        bool inShuffleBag = pack.inShuffleBag;

        if (inShuffleBag) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        }

        if (ImGui::SmallButton(shuffleLabel.c_str())) {
            // Toggle shuffle bag membership via PrejumpPackManager
            if (plugin_->prejumpMgr) {
                plugin_->prejumpMgr->ToggleShuffleBag(pack.code);
            }
        }

        if (inShuffleBag) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(inShuffleBag ? "Remove from shuffle bag" : "Add to shuffle bag");
        }

        // Delete button (only for custom packs, or with confirmation)
        if (pack.source == "custom") {
            ImGui::SameLine();
            std::string deleteLabel = "X##del" + std::to_string(row);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::SmallButton(deleteLabel.c_str())) {
                if (plugin_->prejumpMgr) {
                    plugin_->prejumpMgr->DeletePack(pack.code);
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

    // End columns
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Spacing();
}

bool PrejumpUI::ValidatePackCode(const char* code) const {
    // Validate format: XXXX-XXXX-XXXX-XXXX (alphanumeric, 19 chars total)
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

void PrejumpUI::ClearCustomPackForm() {
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

void PrejumpUI::RenderCustomPackForm() {
    if (ImGui::CollapsingHeader("Add Custom Pack")) {
        ImGui::Indent(10.0f);
        ImGui::Spacing();

        // Success/error messages
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

        // Code input (required)
        ImGui::TextUnformatted("Code *");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(XXXX-XXXX-XXXX-XXXX)");
        ImGui::SetNextItemWidth(220);
        if (ImGui::InputText("##customcode", customPackCode, IM_ARRAYSIZE(customPackCode))) {
            // Auto-format: insert dashes
            std::string raw;
            for (int i = 0; customPackCode[i]; i++) {
                char c = customPackCode[i];
                if (isalnum(static_cast<unsigned char>(c))) {
                    raw += static_cast<char>(toupper(static_cast<unsigned char>(c)));
                }
            }
            // Limit to 16 alphanumeric chars
            if (raw.length() > 16) raw = raw.substr(0, 16);

            // Reformat with dashes
            std::string formatted;
            for (size_t i = 0; i < raw.length(); i++) {
                if (i > 0 && i % 4 == 0) formatted += '-';
                formatted += raw[i];
            }
            strncpy_s(customPackCode, formatted.c_str(), sizeof(customPackCode) - 1);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Training pack code from Rocket League (auto-formatted)");
        }

        // Name input (required)
        ImGui::TextUnformatted("Name *");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##customname", customPackName, IM_ARRAYSIZE(customPackName));

        // Creator input (optional)
        ImGui::TextUnformatted("Creator");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##customcreator", customPackCreator, IM_ARRAYSIZE(customPackCreator));

        // Difficulty dropdown (optional)
        ImGui::TextUnformatted("Difficulty");
        ImGui::SetNextItemWidth(150);
        const char* difficulties[] = {"Unknown", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
        ImGui::Combo("##customdifficulty", &customPackDifficulty, difficulties, IM_ARRAYSIZE(difficulties));

        // Shot count slider
        ImGui::TextUnformatted("Shot Count");
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("##customshots", &customPackShotCount, 1, 50);

        // Tags input (comma-separated)
        ImGui::TextUnformatted("Tags");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(comma-separated)");
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##customtags", customPackTags, IM_ARRAYSIZE(customPackTags));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("e.g., Aerials, Wall Shots, Redirects");
        }

        // Notes input (multiline)
        ImGui::TextUnformatted("Notes");
        ImGui::InputTextMultiline("##customnotes", customPackNotes, IM_ARRAYSIZE(customPackNotes), ImVec2(400, 60));

        // Video URL (optional)
        ImGui::TextUnformatted("Video URL");
        ImGui::SetNextItemWidth(350);
        ImGui::InputText("##customvideo", customPackVideoUrl, IM_ARRAYSIZE(customPackVideoUrl));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("YouTube or other video link (optional)");
        }

        ImGui::Spacing();

        // Buttons
        if (ImGui::Button("Add Pack", ImVec2(100, 0))) {
            customPackError.clear();
            customPackSuccess = false;

            // Validate required fields
            if (strlen(customPackCode) == 0) {
                customPackError = "Pack code is required";
            } else if (!ValidatePackCode(customPackCode)) {
                customPackError = "Invalid code format. Expected: XXXX-XXXX-XXXX-XXXX";
            } else if (strlen(customPackName) == 0) {
                customPackError = "Pack name is required";
            } else {
                // Build TrainingEntry
                TrainingEntry pack;
                pack.code = customPackCode;
                pack.name = customPackName;
                pack.creator = strlen(customPackCreator) > 0 ? customPackCreator : "Unknown";

                const char* difficultyNames[] = {"Unknown", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
                pack.difficulty = difficultyNames[customPackDifficulty];

                pack.shotCount = customPackShotCount;

                // Parse tags
                if (strlen(customPackTags) > 0) {
                    std::string tagsStr = customPackTags;
                    size_t start = 0;
                    size_t end = tagsStr.find(',');
                    while (end != std::string::npos) {
                        std::string tag = tagsStr.substr(start, end - start);
                        // Trim whitespace
                        size_t first = tag.find_first_not_of(" \t");
                        size_t last = tag.find_last_not_of(" \t");
                        if (first != std::string::npos) {
                            pack.tags.push_back(tag.substr(first, last - first + 1));
                        }
                        start = end + 1;
                        end = tagsStr.find(',', start);
                    }
                    // Last tag
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

                // Try to add via manager
                if (plugin_->prejumpMgr) {
                    if (plugin_->prejumpMgr->AddCustomPack(pack)) {
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
