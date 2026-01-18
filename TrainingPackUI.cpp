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

    // Bring window to front when first opened
    if (needsFocusOnNextRender_) {
        ImGui::SetNextWindowFocus();
        needsFocusOnNextRender_ = false;
    }

    // Only prevent bringing to front when bag manager is open
    // This allows normal window interaction (clicking to bring forward) when bag manager is closed
    // But keeps browser in back during drag-and-drop when bag manager is visible
    ImGuiWindowFlags browserFlags = ImGuiWindowFlags_None;
    if (showBagManagerModal) {
        browserFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    if (!ImGui::Begin(GetMenuTitle().c_str(), &isWindowOpen_, browserFlags)) {
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

    // ===== HEADER SECTION =====
    ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "Training Pack Browser");
    ImGui::Spacing();

    // Status line: pack count and last updated
    if (packCount > 0) {
        ImGui::Text("Loaded: %d packs", packCount);
        ImGui::SameLine();
        ImGui::TextColored(UI::TrainingPackUI::LAST_UPDATED_TEXT_COLOR, " | Last updated: %s", lastUpdated.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No packs loaded - click 'Update Pack List' to download");
    }

    // Control buttons
    ImGui::SameLine();
    float buttonX = ImGui::GetWindowWidth() - UI::TrainingPackUI::BUTTON_GROUP_OFFSET_FROM_RIGHT;
    if (buttonX > ImGui::GetCursorPosX()) {
        ImGui::SetCursorPosX(buttonX);
    }

    if (scraping) {
        ImGui::TextColored(UI::TrainingPackUI::SCRAPING_STATUS_TEXT_COLOR, "Updating...");
    } else {
        if (ImGui::Button("Update Pack List")) {
            plugin_->UpdateTrainingPackList();
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
        ImGui::End();
        return;
    }

    // ===== BAG MANAGER STATUS & CONTROLS =====
    if (ImGui::CollapsingHeader("Bag Manager", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& bags = manager ? manager->GetAvailableBags() : std::vector<TrainingBag>();

        // Count total packs across all enabled bags
        int totalPacksInBags = 0;
        int enabledBagCount = 0;
        for (const auto& bag : bags) {
            if (bag.enabled) {
                totalPacksInBags += manager ? manager->GetBagPackCount(bag.name) : 0;
                enabledBagCount++;
            }
        }

        if (totalPacksInBags > 0) {
            ImGui::TextColored(UI::TrainingPackUI::BAG_ROTATION_STATUS_COLOR,
                "Rotation: %d pack%s in %d bag%s",
                totalPacksInBags, totalPacksInBags == 1 ? "" : "s",
                enabledBagCount, enabledBagCount == 1 ? "" : "s");

            ImGui::SameLine();
            if (ImGui::Button("Start Bag Rotation")) {
                SuiteSpot* p = plugin_;
                p->gameWrapper->SetTimeout([p](GameWrapper* gw) {
                    // Enable plugin, set mode to Training, and enable bag rotation
                    p->cvarManager->getCvar("suitespot_enabled").setValue(1);
                    p->cvarManager->getCvar("suitespot_map_type").setValue(1);
                    p->cvarManager->getCvar("suitespot_bag_rotation").setValue(1);
                }, 0.0f);

                LOG("SuiteSpot: Bag rotation started with " + std::to_string(totalPacksInBags) + " packs");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable SuiteSpot, switch to Training mode, and enable bag rotation.");
            }
        } else {
            ImGui::TextDisabled("No packs in rotation bags");
            ImGui::TextWrapped("Add packs to bags using the 'Add to Bag' button below, or right-click a pack row.");
        }

        // Show bag summary in a compact row
        ImGui::Spacing();
        ImGui::TextUnformatted("Bags:");
        ImGui::SameLine();
        for (const auto& bag : bags) {
            int bagCount = manager ? manager->GetBagPackCount(bag.name) : 0;
            if (bagCount > 0) {
                ImVec4 badgeColor(bag.color[0], bag.color[1], bag.color[2], bag.color[3]);
                ImGui::TextColored(badgeColor, "%s %s(%d)", bag.icon.c_str(), bag.name.c_str(), bagCount);
                ImGui::SameLine();
            }
        }
        ImGui::NewLine();

        ImGui::SameLine();
        if (ImGui::Button("Manage Bags...")) {
            showBagManagerModal = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open bag manager to enable/disable bags and adjust rotation order");
        }

        ImGui::Spacing();
    }

    // Render bag manager modal (after the collapsing header)
    RenderBagManagerModal();

    ImGui::Separator();
    ImGui::Spacing();

    // ===== ADD CUSTOM PACK SECTION =====
    RenderCustomPackForm();

    ImGui::Separator();
    ImGui::Spacing();

    // Early return if no packs loaded
    if (packs.empty()) {
        ImGui::TextWrapped("No packs available. Click 'Scrape Packs' to download the training pack database, or add your own custom packs above.");
        ImGui::End();
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
    const char* difficulties[] = {"All", "Unranked", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
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
        bool hasSelection = !selectedPackCode.empty();
        
        // Find selected pack data
        const TrainingEntry* selectedPack = nullptr;
        if (hasSelection) {
            for (const auto& pack : filteredPacks) {
                if (pack.code == selectedPackCode) {
                    selectedPack = &pack;
                    break;
                }
            }
        }

        // Load Pack
        if (hasSelection) {
            if (ImGui::Button("Load Pack")) {
                 SuiteSpot* plugin = plugin_;
                 std::string code = selectedPack->code;
                 std::string name = selectedPack->name;
                 plugin_->gameWrapper->SetTimeout([plugin, code, name](GameWrapper* gw) {
                    std::string cmd = "load_training " + code;
                    plugin->cvarManager->executeCommand(cmd);
                    LOG("SuiteSpot: Loading training pack: " + name);
                }, 0.0f);
            }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Load Pack");
            ImGui::PopStyleVar();
        }

        ImGui::SameLine();

        // Add to Bag (opens popup to select bag)
        if (hasSelection) {
            if (ImGui::Button("Add to Bag...")) {
                ImGui::OpenPopup("BagPickerPopup");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Add selected pack to a training bag");
            }
        } else {
             ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
             ImGui::Button("Add to Bag...");
             ImGui::PopStyleVar();
        }

        // Bag Picker Popup
        if (ImGui::BeginPopup("BagPickerPopup")) {
            ImGui::TextUnformatted("Select Bag:");
            ImGui::Separator();

            const auto& bags = manager ? manager->GetAvailableBags() : std::vector<TrainingBag>();
            for (const auto& bag : bags) {
                ImVec4 badgeColor(bag.color[0], bag.color[1], bag.color[2], bag.color[3]);
                ImGui::PushStyleColor(ImGuiCol_Text, badgeColor);
                std::string label = bag.icon + " " + bag.name;
                if (ImGui::Selectable(label.c_str())) {
                    if (plugin_->trainingPackMgr) {
                        std::vector<std::string> codes;
                        codes.push_back(selectedPackCode);
                        plugin_->trainingPackMgr->AddPacksToBag(codes, bag.name);
                        browserStatus.ShowSuccess("Added pack to " + bag.name, 3.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                    }
                }
                ImGui::PopStyleColor();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        // Delete (Custom only)
        if (hasSelection) {
             if (ImGui::Button("Delete Custom Pack")) {
                 if (plugin_->trainingPackMgr) {
                     plugin_->trainingPackMgr->DeletePack(selectedPackCode);
                     browserStatus.ShowSuccess("Deleted custom pack", 3.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                     selectedPackCode = "";
                 }
             }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Delete Custom Pack");
            ImGui::PopStyleVar();
        }

        ImGui::SameLine();

        // Clear Selection
        if (hasSelection) {
            if (ImGui::Button("Clear Selection")) {
                selectedPackCode = "";
            }
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Clear Selection");
            ImGui::PopStyleVar();
        }
    }

    // ===== TABLE WITH RESIZABLE COLUMNS & FROZEN HEADER =====
    ImGui::Separator();

    // Only set initial column widths once, or when window width changes significantly
    // This allows users to manually resize columns by dragging
    float currentWindowWidth = ImGui::GetWindowContentRegionWidth();
    bool windowResized = std::abs(currentWindowWidth - lastWindowWidth) > 50.0f;

    if (!columnWidthsInitialized || windowResized) {
        CalculateOptimalColumnWidths();
        columnWidthsInitialized = true;
        lastWindowWidth = currentWindowWidth;
    }

    // ===== FROZEN HEADER ROW =====
    ImGui::Columns(UI::TrainingPackUI::TABLE_COLUMN_COUNT, "PackColumns_Header", true);

    for (int i = 0; i < 5 && i < (int)columnWidths.size(); i++) {
        ImGui::SetColumnWidth(i, columnWidths[i]);
    }

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

    ImGui::Columns(1);
    ImGui::Separator();

    // ===== SCROLLABLE PACK ROWS =====
    ImGui::BeginChild("PackTable", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::Columns(UI::TrainingPackUI::TABLE_COLUMN_COUNT, "PackColumns_Body", true);

    for (int i = 0; i < 5 && i < (int)columnWidths.size(); i++) {
        ImGui::SetColumnWidth(i, columnWidths[i]);
    }

    ImGuiListClipper clipper;
    clipper.Begin((int)filteredPacks.size());

    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            const auto& pack = filteredPacks[row];

            // Name column with Selection Logic
            bool isSelected = (selectedPackCode == pack.code);
            ImGui::PushID(pack.code.c_str());

            // Play Button (if video exists)
            if (!pack.videoUrl.empty()) {
                bool clicked = false;
                if (youtubeIcon && youtubeIcon->IsLoadedForImGui()) {
                    if (ImGui::ImageButton(youtubeIcon->GetImGuiTex(), ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
                        clicked = true;
                    }
                } else {
                    if (ImGui::ArrowButton("##play", ImGuiDir_Right)) {
                        clicked = true;
                    }
                }

                if (clicked) {
                    ShellExecuteA(NULL, "open", pack.videoUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Watch Preview");
                ImGui::SameLine();
            } else {
                // Indent to align with packs that have buttons (approximate width of ArrowButton + Spacing)
                ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0)); 
                ImGui::SameLine();
            }

            // SpanAllColumns allows clicking anywhere in the row
            // Simple toggle-on-click behavior
            if (ImGui::Selectable(pack.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                if (isSelected) {
                    selectedPackCode = "";
                } else {
                    selectedPackCode = pack.code;

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

            // === DRAG SOURCE for drag-and-drop to bag manager ===
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                // Payload: pack code as null-terminated string
                ImGui::SetDragDropPayload("TRAINING_PACK_CODE", pack.code.c_str(), pack.code.length() + 1);

                // Preview during drag
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Dragging: %s", pack.name.c_str());

                ImGui::EndDragDropSource();
            }

            // Right-click context menu for quick bag management
            if (ImGui::BeginPopupContextItem(("PackContext_" + pack.code).c_str())) {
                ImGui::TextColored(UI::TrainingPackUI::SECTION_HEADER_TEXT_COLOR, "%s", pack.name.c_str());
                ImGui::Separator();

                const auto& bags = manager ? manager->GetAvailableBags() : std::vector<TrainingBag>();
                for (const auto& bag : bags) {
                    bool inBag = pack.bagCategories.count(bag.name) > 0;
                    std::string label = (inBag ? "[X] " : "[ ] ") + bag.icon + " " + bag.name;

                    if (ImGui::Selectable(label.c_str())) {
                        if (plugin_->trainingPackMgr) {
                            if (inBag) {
                                plugin_->trainingPackMgr->RemovePackFromBag(pack.code, bag.name);
                                browserStatus.ShowSuccess("Removed from " + bag.name, 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                            } else {
                                plugin_->trainingPackMgr->AddPackToBag(pack.code, bag.name);
                                browserStatus.ShowSuccess("Added to " + bag.name, 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
                            }
                        }
                    }
                }
                ImGui::EndPopup();
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
                    ImGui::PushTextWrapPos(450.0f);
                    ImGui::TextUnformatted(tooltip.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
            }
            ImGui::NextColumn();

            // Difficulty column with color coding
            ImVec4 diffColor = UI::TrainingPackUI::DIFFICULTY_BADGE_UNRANKED_COLOR;
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
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::End();
}

void TrainingPackUI::RenderBagManagerModal() {
    if (!showBagManagerModal) return;

    // Use a separate floating window instead of a modal popup
    // This allows interaction with the browser window for drag-and-drop
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Bag Manager", &showBagManagerModal, ImGuiWindowFlags_None)) {
        ImGui::TextWrapped("Drag training packs from the browser into any bag below. Use up/down arrows to reorder packs within a bag.");
        ImGui::Separator();
        ImGui::Spacing();

        const auto* manager = plugin_->trainingPackMgr;
        const auto& bags = manager ? manager->GetAvailableBags() : std::vector<TrainingBag>();

        // Calculate child window dimensions
        ImGuiStyle& style = ImGui::GetStyle();
        float childWidth = (ImGui::GetContentRegionAvail().x - 2 * style.ItemSpacing.x) / 3.0f;
        float childHeight = 250.0f;

        // Render 6 bags in 3x2 grid
        for (int bagIdx = 0; bagIdx < 6 && bagIdx < (int)bags.size(); bagIdx++) {
            const auto& bag = bags[bagIdx];

            // Layout: 3 per row
            if (bagIdx % 3 != 0) ImGui::SameLine();

            ImGui::PushID(bagIdx);
            RenderBagChildWindow(bag, childWidth, childHeight);
            ImGui::PopID();
        }

        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            showBagManagerModal = false;
        }

        ImGui::End();
    }
}

void TrainingPackUI::RenderBagChildWindow(const TrainingBag& bag, float width, float height) {
    const auto* manager = plugin_->trainingPackMgr;

    // Get packs in this bag
    auto packsInBag = manager ? manager->GetPacksInBag(bag.name) : std::vector<TrainingEntry>();

    // Bag header with icon and color
    ImVec4 bagColor(bag.color[0], bag.color[1], bag.color[2], bag.color[3]);
    ImGui::PushStyleColor(ImGuiCol_Border, bagColor);

    ImGui::BeginChild(bag.name.c_str(), ImVec2(width, height), true);

    // === HEADER ===
    ImGui::TextColored(bagColor, "%s %s", bag.icon.c_str(), bag.name.c_str());
    ImGui::SameLine();

    // Active checkbox
    bool enabled = bag.enabled;
    if (ImGui::Checkbox("Active", &enabled)) {
        if (manager) {
            plugin_->trainingPackMgr->SetBagEnabled(bag.name, enabled);
        }
    }

    // Up/Down arrow buttons (positioned on same line)
    ImGui::SameLine();
    if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
        MoveSelectedPackUp(bag.name);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move selected pack up");

    ImGui::SameLine();
    if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
        MoveSelectedPackDown(bag.name);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move selected pack down");

    ImGui::SameLine();
    ImGui::TextDisabled("(%d)", (int)packsInBag.size());

    ImGui::Separator();

    // === PACK LIST (Scrollable) ===
    if (packsInBag.empty()) {
        ImGui::TextDisabled("Drop packs here");
    } else {
        // Render each pack in the bag
        for (int i = 0; i < (int)packsInBag.size(); i++) {
            const auto& pack = packsInBag[i];

            // Selectable pack row
            bool isSelected = selectedPackInBag[bag.name] == pack.code;
            if (ImGui::Selectable(pack.name.c_str(), isSelected)) {
                selectedPackInBag[bag.name] = pack.code;
            }

            // Context menu for quick remove
            if (ImGui::BeginPopupContextItem(("PackInBagCtx_" + pack.code).c_str())) {
                if (ImGui::MenuItem("Remove from bag")) {
                    if (manager) {
                        plugin_->trainingPackMgr->RemovePackFromBag(pack.code, bag.name);
                    }
                }
                ImGui::EndPopup();
            }
        }
    }

    // === DROP TARGET (Entire child region) ===
    if (ImGui::BeginDragDropTarget()) {
        // Accept packs from browser
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TRAINING_PACK_CODE")) {
            // Payload contains pack code string
            const char* packCode = (const char*)payload->Data;
            if (manager) {
                plugin_->trainingPackMgr->AddPackToBag(packCode, bag.name);
                browserStatus.ShowSuccess("Added to " + bag.name, 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void TrainingPackUI::MoveSelectedPackUp(const std::string& bagName) {
    auto it = selectedPackInBag.find(bagName);
    if (it == selectedPackInBag.end()) return;  // No selection

    std::string packCode = it->second;
    const auto* manager = plugin_->trainingPackMgr;
    if (!manager) return;

    auto packsInBag = manager->GetPacksInBag(bagName);

    // Find pack index
    int currentIdx = -1;
    for (int i = 0; i < (int)packsInBag.size(); i++) {
        if (packsInBag[i].code == packCode) {
            currentIdx = i;
            break;
        }
    }

    if (currentIdx <= 0) return;  // Already at top or not found

    // Swap with previous pack
    if (plugin_->trainingPackMgr) {
        plugin_->trainingPackMgr->SwapPacksInBag(bagName, currentIdx, currentIdx - 1);
    }
}

void TrainingPackUI::MoveSelectedPackDown(const std::string& bagName) {
    auto it = selectedPackInBag.find(bagName);
    if (it == selectedPackInBag.end()) return;  // No selection

    std::string packCode = it->second;
    const auto* manager = plugin_->trainingPackMgr;
    if (!manager) return;

    auto packsInBag = manager->GetPacksInBag(bagName);

    // Find pack index
    int currentIdx = -1;
    for (int i = 0; i < (int)packsInBag.size(); i++) {
        if (packsInBag[i].code == packCode) {
            currentIdx = i;
            break;
        }
    }

    if (currentIdx < 0 || currentIdx >= (int)packsInBag.size() - 1) return;  // At bottom or not found

    // Swap with next pack
    if (plugin_->trainingPackMgr) {
        plugin_->trainingPackMgr->SwapPacksInBag(bagName, currentIdx, currentIdx + 1);
    }
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
        const char* difficulties[] = {"Unranked", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
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
                const char* difficultyNames[] = {"Unranked", "Bronze", "Silver", "Gold", "Platinum", "Diamond", "Champion", "Grand Champion", "Supersonic Legend"};
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

    // Column proportions: Name (45%), Difficulty (25%), Shots (10%), Likes (10%), Plays (10%)
    columnWidths.resize(5);
    columnWidths[0] = availWidth * 0.45f;  // Name
    columnWidths[1] = availWidth * 0.25f;  // Difficulty
    columnWidths[2] = availWidth * 0.10f;  // Shots
    columnWidths[3] = availWidth * 0.10f;  // Likes
    columnWidths[4] = availWidth * 0.10f;  // Plays

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
    if (!isWindowOpen_) {
        return false;  // Window closed → no blocking
    }

    // ============================================================================
    // INPUT BLOCKING STRATEGY
    // ============================================================================
    //
    // The default PluginWindowBase::ShouldBlockInput() blocks ALL application
    // input whenever ImGui wants mouse or keyboard input. This is too aggressive
    // and prevents multi-window interactions like drag-and-drop.
    //
    // Our selective approach:
    // 1. Allow drag-and-drop between browser and bag manager (modal popups)
    // 2. Block only when user is actively typing in text fields
    // 3. Allow normal mouse interactions (clicking, hovering, scrolling)
    //
    // This enables the drag-and-drop UX while maintaining text input safety.
    // Based on: docs/Examples/ManagingMultipleWindows.md:163-180
    // ============================================================================

    ImGuiIO& io = ImGui::GetIO();

    // Block when actively typing in text fields (search box, custom pack form)
    // This prevents game commands from firing while typing
    if (io.WantTextInput && ImGui::IsAnyItemActive()) {
        return true;
    }

    // Allow normal mouse interaction without blocking game input
    // This includes drag-and-drop operations between browser and bag manager modal
    // Note: We intentionally DON'T block for popups/modals because the Bag Manager modal
    // needs to allow drag operations from the parent browser window (same ImGui context)
    return false;
}

bool TrainingPackUI::IsActiveOverlay() {
    return isWindowOpen_;
}

void TrainingPackUI::OnOpen() {
    isWindowOpen_ = true;
    needsFocusOnNextRender_ = true;  // Bring window to front on next render

    if (!youtubeIcon) {
        auto path = plugin_->GetDataRoot() / "Resources" / "Icons" / "icon_youtube.png";
        youtubeIcon = std::make_shared<ImageWrapper>(path.string(), true);
        youtubeIcon->LoadForImGui([this](bool success){
            if(!success){
                LOG("SuiteSpot: Failed to load YouTube icon from " + (plugin_->GetDataRoot() / "Resources" / "Icons" / "icon_youtube.png").string());
            }
        });
    }
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
