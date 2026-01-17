#include "pch.h"
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include "TrainingPackUI.h"
#include "TrainingPackManager.h"
#include "SuiteSpot.h"
#include "ConstantsUI.h"
#include "HelpersUI.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

// Helper function for sortable column headers with visual indicators
namespace {
    bool SortableColumnHeader(const char* label, int columnIndex, int& currentSortColumn, bool& sortAscending) {
        // Display label with sort indicator if this column is active
        // Use ASCII arrows (^ v) since Unicode triangles may not be in the font
        char buffer[256];
        if (currentSortColumn == columnIndex) {
            snprintf(buffer, sizeof(buffer), "%s %s", label, sortAscending ? "(asc)" : "(desc)");
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

void TrainingPackUI::Render() {
    if (!isWindowOpen_) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(GetMenuTitle().c_str(), &isWindowOpen_, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    const auto* manager = plugin_->trainingPackMgr;
    static const std::vector<TrainingEntry> emptyPacks;
    static const std::string emptyString;
    const auto& packs = manager ? manager->GetPacks() : emptyPacks;
    const int packCount = manager ? manager->GetPackCount() : 0;
    const auto& lastUpdated = manager ? manager->GetLastUpdated() : emptyString;
    const bool scraping = manager && manager->IsScrapingInProgress();
    
    // Use cached value to avoid CVar access on render thread
    bool trainingShuffleEnabledValue = cachedShuffleEnabled;

    // ===== HEADER SECTION =====
    ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "Training Pack Browser");
    ImGui::Spacing();

    // Status line: pack count and last updated
    if (packCount > 0) {
        ImGui::Text("Loaded: %d packs", packCount);
        ImGui::SameLine();
        ImGui::TextColored(UI::TrainingPackUI::LAST_UPDATED_TEXT_COLOR, " | Last updated: %s", lastUpdated.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No packs loaded - click 'Scrape Packs' to download");
    }

    // Control buttons
    ImGui::SameLine();
    float buttonX = ImGui::GetWindowWidth() - UI::TrainingPackUI::BUTTON_GROUP_OFFSET_FROM_RIGHT;
    if (buttonX > ImGui::GetCursorPosX()) {
        ImGui::SetCursorPosX(buttonX);
    }

    if (scraping) {
        ImGui::TextColored(UI::TrainingPackUI::SCRAPING_STATUS_TEXT_COLOR, "Scraping...");
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
            ImGui::TextColored(UI::TrainingPackUI::SHUFFLE_ACTIVE_STATUS_COLOR, "Current Bag: %d pack%s", shufflePackCount, shufflePackCount == 1 ? "" : "s");

            ImGui::SameLine();
            if (ImGui::Button("Start Shuffle Training")) {
                SuiteSpot* p = plugin_;
                p->gameWrapper->SetTimeout([p](GameWrapper* gw) {
                    // Enable plugin, set mode to Training, and enable Shuffle
                    p->cvarManager->getCvar("suitespot_enabled").setValue(1);
                    // Training
                    p->cvarManager->getCvar("suitespot_map_type").setValue(1);
                    p->cvarManager->getCvar("suitespot_training_shuffle").setValue(1);
                }, 0.0f);

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
            ImGui::SameLine(UI::TrainingPackUI::SHUFFLE_CHECKBOX_POSITION_X);
            if (ImGui::Checkbox("Shuffle Active", &trainingShuffleEnabledValue)) {
                cachedShuffleEnabled = trainingShuffleEnabledValue;
                SuiteSpot* p = plugin_;
                bool v = trainingShuffleEnabledValue;
                p->gameWrapper->SetTimeout([p, v](GameWrapper* gw) {
                    p->cvarManager->getCvar("suitespot_training_shuffle").setValue(v);
                }, 0.0f);
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

    // Fixed widths for filter controls
    // Search box
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputText("##search", packSearchText, IM_ARRAYSIZE(packSearchText))) {
        filtersChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Search by pack name, creator, or tag");
    }

    // Difficulty filter
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
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
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::SliderInt("Min Shots", &packMinShots, UI::TrainingPackUI::FILTER_MIN_SHOTS_MIN, UI::TrainingPackUI::FILTER_MIN_SHOTS_MAX)) {
        filtersChanged = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Minimum number of shots in pack");
    }

    // Tag filter dropdown (second row)
    ImGui::SetNextItemWidth(UI::TrainingPackUI::TAG_FILTER_DROPDOWN_WIDTH);

    bool packsSourceChanged = (lastPackCount != packCount);

    if (!tagsInitialized || packsSourceChanged) {
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
    if (filtersChanged || packsSourceChanged || !packListInitialized) {
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
        packListInitialized = true;
    }

    // Display filtered count
    ImGui::Text("Showing %d of %d packs", (int)filteredPacks.size(), packCount);
    ImGui::Spacing();

    // ===== ACTION BAR =====
    ImGui::Separator();

    // Render browser status message
    browserStatus.Render(ImGui::GetIO().DeltaTime);
    if (browserStatus.IsVisible()) ImGui::Spacing();

    {
        int numSelected = (int)selectedPackCodes.size();
        
        // Watch Video
        bool canWatch = false;
        std::string videoUrl = "";
        bool singleSelection = numSelected == 1;
        if (singleSelection) {
             for (const auto& pack : filteredPacks) {
                 if (selectedPackCodes.count(pack.code)) {
                     videoUrl = pack.videoUrl;
                     break;
                 }
             }
             canWatch = !videoUrl.empty();
        }

        std::string watchLabel = "Watch Video";
        if (numSelected > 1) {
            watchLabel = "No Video";
        } else if (singleSelection && !canWatch) {
            watchLabel = "No Video";
        }
        
        if (singleSelection && canWatch) {
            if (ImGui::Button(watchLabel.c_str())) {
                ShellExecuteA(NULL, "open", videoUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button(watchLabel.c_str());
            ImGui::PopStyleVar();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open preview video for selected pack");

        ImGui::SameLine();

        // Load Pack
        if (numSelected == 1) {
            if (ImGui::Button("Load Pack")) {
                for (const auto& pack : filteredPacks) {
                    if (selectedPackCodes.count(pack.code)) {
                         SuiteSpot* plugin = plugin_;
                         std::string code = pack.code;
                         std::string name = pack.name;
                         plugin_->gameWrapper->SetTimeout([plugin, code, name](GameWrapper* gw) {
                            std::string cmd = "load_training " + code;
                            plugin->cvarManager->executeCommand(cmd);
                            LOG("SuiteSpot: Loading training pack: " + name);
                        }, 0.0f);
                        break;
                    }
                }
            }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Load Pack");
            ImGui::PopStyleVar();
        }

        ImGui::SameLine();

        // Add to Shuffle Bag
        if (numSelected > 0) {
            if (ImGui::Button("Add to Shuffle Bag")) {
                if (plugin_->trainingPackMgr) {
                    int addedCount = 0;
                    for (const auto& pack : filteredPacks) {
                        if (selectedPackCodes.count(pack.code)) {
                            plugin_->trainingPackMgr->AddToShuffleBag(pack.code);
                            addedCount++;
                        }
                    }
                    browserStatus.ShowSuccess("Added " + std::to_string(addedCount) + " pack(s) to shuffle bag", 3.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                }
            }
        } else {
             ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
             ImGui::Button("Add to Shuffle Bag");
             ImGui::PopStyleVar();
        }

        ImGui::SameLine();

        // Delete (Custom only)
        if (numSelected > 0) {
             if (ImGui::Button("Delete Custom Pack")) {
                 if (plugin_->trainingPackMgr) {
                     int deletedCount = (int)selectedPackCodes.size();
                     std::vector<std::string> toDelete;
                     for(const auto& code : selectedPackCodes) toDelete.push_back(code);
                     
                     for(const auto& code : toDelete) {
                         plugin_->trainingPackMgr->DeletePack(code);
                     }
                     browserStatus.ShowSuccess("Deleted " + std::to_string(deletedCount) + " custom pack(s)", 3.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                     selectedPackCodes.clear();
                 }
             }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Delete Custom Pack");
            ImGui::PopStyleVar();
        }

        ImGui::SameLine();

        // Clear Selection
        if (numSelected > 0) {
            if (ImGui::Button("Clear Selection")) {
                selectedPackCodes.clear();
            }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Clear Selection");
            ImGui::PopStyleVar();
        }
    }

    // ===== TABLE WITH RESIZABLE COLUMNS (ImGui 1.75 Columns API) =====
    ImGui::Separator();

    // Reduced columns to 5 (Removed Creator, Tags, Actions)
    ImGui::Columns(UI::TrainingPackUI::TABLE_COLUMN_COUNT, "PackColumns", true);

    // Only set initial column widths once, or when window width changes significantly
    // This allows users to manually resize columns by dragging
    float currentWindowWidth = ImGui::GetWindowContentRegionWidth();
    bool windowResized = std::abs(currentWindowWidth - lastWindowWidth) > 50.0f;

    if (!columnWidthsInitialized || windowResized) {
        CalculateOptimalColumnWidths();
        for (int i = 0; i < 5 && i < (int)columnWidths.size(); i++) {
            ImGui::SetColumnWidth(i, columnWidths[i]);
        }
        columnWidthsInitialized = true;
        lastWindowWidth = currentWindowWidth;
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

    ImGui::Separator();

    // ===== RENDER PACK ROWS =====
    ImGuiListClipper clipper;
    clipper.Begin((int)filteredPacks.size());

    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            const auto& pack = filteredPacks[row];

                        // Name column with Selection Logic
                        bool isSelected = selectedPackCodes.count(pack.code) > 0;
                        ImGui::PushID(pack.code.c_str());
            
                        // SpanAllColumns allows clicking anywhere in the row
                        // Simple toggle-on-click behavior (Checklist style)
                        if (ImGui::Selectable(pack.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                            if (isSelected) {
                                selectedPackCodes.erase(pack.code);
                            } else {
                                selectedPackCodes.insert(pack.code);
                                
                                // Sync with AutoLoadFeature by updating the global index
                                // This ensures match end logic respects the last selected map
                                for (int i = 0; i < (int)RLTraining.size(); ++i) {
                                    if (RLTraining[i].code == pack.code) {
                                        SuiteSpot* p = plugin_;
                                        p->gameWrapper->SetTimeout([p, i](GameWrapper* gw) {
                                            p->cvarManager->getCvar("suitespot_current_training_index").setValue(i);
                                        }, 0.0f);
                                        break;
                                    }
                                }
                            }
                            lastSelectedRowIndex = row;
                        }
                        ImGui::PopID();
            if (ImGui::IsItemHovered()) {
                std::string tooltip = "";
                if (!pack.staffComments.empty()) tooltip += pack.staffComments + "\n";
                if (!pack.creator.empty()) tooltip += "Creator: " + pack.creator + "\n";
                if (!pack.tags.empty()) {
                    tooltip += "Tags: ";
                    for (size_t i = 0; i < pack.tags.size(); i++) {
                        if (i > 0) tooltip += ", ";
                        tooltip += pack.tags[i];
                    }
                }
                if (!tooltip.empty()) {
                    ImVec2 mPos = ImGui::GetMousePos();
                    ImGui::SetNextWindowPos(ImVec2(mPos.x + 20, mPos.y + 20));
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", tooltip.c_str());
                    ImGui::EndTooltip();
                }
            }
            ImGui::NextColumn();

            // Difficulty column with color coding
            ImVec4 diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_DEFAULT_COLOR;
            if (pack.difficulty == "Bronze") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_BRONZE_COLOR;
            else if (pack.difficulty == "Silver") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_SILVER_COLOR;
            else if (pack.difficulty == "Gold") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_GOLD_COLOR;
            else if (pack.difficulty == "Platinum") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_PLATINUM_COLOR;
            else if (pack.difficulty == "Diamond") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_DIAMOND_COLOR;
            else if (pack.difficulty == "Champion") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_CHAMPION_COLOR;
            else if (pack.difficulty == "Grand Champion") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_GRAND_CHAMPION_COLOR;
            else if (pack.difficulty == "Supersonic Legend") diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_SUPERSONIC_LEGEND_COLOR;
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
        }
    }

    // End columns
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Spacing();
    ImGui::End();
}

bool TrainingPackUI::ValidatePackCode(const char* code) const {
    if (strlen(code) != UI::TrainingPackUI::PACK_CODE_EXPECTED_LENGTH) return false;
    for (int i = 0; i < UI::TrainingPackUI::PACK_CODE_EXPECTED_LENGTH; i++) {
        if (i == UI::TrainingPackUI::PACK_CODE_DASH_POSITION_1 || i == UI::TrainingPackUI::PACK_CODE_DASH_POSITION_2 || i == UI::TrainingPackUI::PACK_CODE_DASH_POSITION_3) {
            if (code[i] != '-') return false;
        }
        else {
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
    customPackStatus.Clear();
}

void TrainingPackUI::RenderCustomPackForm() {
    if (ImGui::CollapsingHeader("Add Custom Pack")) {
        ImGui::Indent(UI::TrainingPackUI::CUSTOM_PACK_FORM_INDENT);
        ImGui::Spacing();

        customPackStatus.Render(ImGui::GetIO().DeltaTime);
        if (customPackStatus.IsVisible()) {
            ImGui::Spacing();
        }

        ImGui::TextUnformatted("Code *");
        ImGui::SameLine();
        ImGui::TextColored(UI::TrainingPackUI::DISABLED_INFO_TEXT_COLOR, "(XXXX-XXXX-XXXX-XXXX)");
        ImGui::SetNextItemWidth(UI::TrainingPackUI::CUSTOM_PACK_CODE_INPUT_WIDTH);
        if (ImGui::InputText("##customcode", customPackCode, IM_ARRAYSIZE(customPackCode))) {
            std::string raw;
            for (int i = 0; customPackCode[i]; i++) {
                char c = customPackCode[i];
                if (isalnum(static_cast<unsigned char>(c))) {
                    raw += static_cast<char>(toupper(static_cast<unsigned char>(c)));
                }
            }
            if (raw.length() > UI::TrainingPackUI::PACK_CODE_RAW_MAX_LENGTH) raw = raw.substr(0, UI::TrainingPackUI::PACK_CODE_RAW_MAX_LENGTH);
            std::string formatted;
            for (size_t i = 0; i < raw.length(); i++) {
                if (i > 0 && i % 4 == 0) formatted += '-';
                formatted += raw[i];
            }
            strncpy_s(customPackCode, formatted.c_str(), sizeof(customPackCode) - 1);
        }

        ImGui::TextUnformatted("Name *");
        ImGui::SetNextItemWidth(UI::TrainingPackUI::CUSTOM_PACK_NAME_INPUT_WIDTH);
        ImGui::InputText("##customname", customPackName, IM_ARRAYSIZE(customPackName));

        ImGui::TextUnformatted("Creator");
        ImGui::SetNextItemWidth(UI::TrainingPackUI::CUSTOM_PACK_CREATOR_INPUT_WIDTH);
        ImGui::InputText("##customcreator", customPackCreator, IM_ARRAYSIZE(customPackCreator));

        ImGui::TextUnformatted("Difficulty");
        ImGui::SetNextItemWidth(UI::TrainingPackUI::CUSTOM_PACK_DIFFICULTY_DROPDOWN_WIDTH);
        const char* difficulties[] = {"Unknown", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
        ImGui::Combo("##customdifficulty", &customPackDifficulty, difficulties, IM_ARRAYSIZE(difficulties));

        ImGui::TextUnformatted("Shot Count");
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("##customshots", &customPackShotCount, UI::TrainingPackUI::CUSTOM_PACK_SHOTS_MIN, UI::TrainingPackUI::CUSTOM_PACK_SHOTS_MAX);

        ImGui::TextUnformatted("Tags");
        ImGui::SameLine();
        ImGui::TextColored(UI::TrainingPackUI::DISABLED_INFO_TEXT_COLOR, "(comma-separated)");
        ImGui::SetNextItemWidth(UI::TrainingPackUI::CUSTOM_PACK_TAGS_INPUT_WIDTH);
        ImGui::InputText("##customtags", customPackTags, IM_ARRAYSIZE(customPackTags));

        ImGui::TextUnformatted("Notes");
        ImGui::InputTextMultiline("##customnotes", customPackNotes, IM_ARRAYSIZE(customPackNotes), ImVec2(UI::TrainingPackUI::CUSTOM_PACK_NOTES_INPUT_WIDTH, UI::TrainingPackUI::CUSTOM_PACK_NOTES_INPUT_HEIGHT));

        ImGui::TextUnformatted("Video URL");
        ImGui::SetNextItemWidth(UI::TrainingPackUI::CUSTOM_PACK_VIDEO_URL_INPUT_WIDTH);
        ImGui::InputText("##customvideo", customPackVideoUrl, IM_ARRAYSIZE(customPackVideoUrl));

        ImGui::Spacing();

        if (ImGui::Button("Add Pack", ImVec2(UI::TrainingPackUI::CUSTOM_PACK_ADD_BUTTON_WIDTH, UI::TrainingPackUI::CUSTOM_PACK_ADD_BUTTON_HEIGHT))) {
            customPackStatus.Clear();
            if (strlen(customPackCode) == 0) {
                customPackStatus.ShowError("Pack code is required");
            }
            else if (!ValidatePackCode(customPackCode)) {
                customPackStatus.ShowError("Invalid code format. Expected: XXXX-XXXX-XXXX-XXXX");
            }
            else if (strlen(customPackName) == 0) {
                customPackStatus.ShowError("Pack name is required");
            }
            else {
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
                        customPackStatus.ShowSuccess("Pack added successfully!");
                        ClearCustomPackForm();
                        LOG("SuiteSpot: Added custom pack: " + pack.name);
                    } else {
                        customPackStatus.ShowError("Pack with this code already exists");
                    }
                } else {
                    customPackStatus.ShowError("Pack manager not initialized");
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(UI::TrainingPackUI::CUSTOM_PACK_CLEAR_BUTTON_WIDTH, UI::TrainingPackUI::CUSTOM_PACK_CLEAR_BUTTON_HEIGHT))) {
            ClearCustomPackForm();
        }
        ImGui::Spacing();
        ImGui::TextColored(UI::TrainingPackUI::DISABLED_INFO_TEXT_COLOR, "* Required fields");
        ImGui::Unindent(UI::TrainingPackUI::CUSTOM_PACK_FORM_INDENT);
        ImGui::Spacing();
    }
}

void TrainingPackUI::CalculateOptimalColumnWidths() {
    // Dynamic proportional widths based on full window content width
    // Use GetWindowContentRegionWidth() instead of GetContentRegionAvail().x
    // to get the full width regardless of cursor position
    float availWidth = ImGui::GetWindowContentRegionWidth();

    // Column proportions: Name (40%), Difficulty (20%), Shots (15%), Likes (10%), Plays (15%)
    columnWidths.resize(5);
    columnWidths[0] = availWidth * 0.40f;  // Name
    columnWidths[1] = availWidth * 0.20f;  // Difficulty
    columnWidths[2] = availWidth * 0.15f;  // Shots
    columnWidths[3] = availWidth * 0.10f;  // Likes
    columnWidths[4] = availWidth * 0.15f;  // Plays

    // Apply minimum widths to ensure readability
    if (columnWidths[0] < 150.0f) columnWidths[0] = 150.0f;
    if (columnWidths[1] < 100.0f) columnWidths[1] = 100.0f;
    if (columnWidths[2] < 60.0f) columnWidths[2] = 60.0f;
    if (columnWidths[3] < 60.0f) columnWidths[3] = 60.0f;
    if (columnWidths[4] < 60.0f) columnWidths[4] = 60.0f;
}

std::string TrainingPackUI::GetMenuName() {
    return "suitespot_browser";
}

std::string TrainingPackUI::GetMenuTitle() {
    return "SuiteSpot Training Browser";
}

void TrainingPackUI::SetImGuiContext(uintptr_t ctx) {
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool TrainingPackUI::ShouldBlockInput() {
    return isWindowOpen_;
}

bool TrainingPackUI::IsActiveOverlay() {
    return isWindowOpen_;
}

void TrainingPackUI::OnOpen() {
    isWindowOpen_ = true;
}

void TrainingPackUI::OnClose() {
    isWindowOpen_ = false;
}

bool TrainingPackUI::IsOpen() {
    return isWindowOpen_;
}

void TrainingPackUI::SetOpen(bool open) {
    isWindowOpen_ = open;
}