# Training Browser Two-Panel Layout Plan

## Overview
Redesign the "SuiteSpot Training Browser" window to use a Windows Explorer-style layout:
- **LEFT**: Pack list table with filters above
- **RIGHT**: Persistent details panel showing selected pack info and actions

## Evidence & Verification

### Source Files Located
| Item | Location |
|------|----------|
| Window Title | `"SuiteSpot Training Browser"` at `TrainingPackUI.cpp:800` |
| Render Function | `TrainingPackUI::Render()` at `TrainingPackUI.cpp:56-613` |
| Call Chain | `SuiteSpot::RenderSettings()` → `trainingPackUI->Render()` |
| Selection State | `std::string selectedPackCode` already exists at `TrainingPackUI.h:101` |
| Existing Pattern | Workshop two-panel layout at `SettingsUI.cpp:418-571` |

### Verified ImGui APIs (from `IMGUI/imgui.h` v1.75)
- `ImGui::BeginChild(str_id, size, border, flags)` - line 276
- `ImGui::EndChild()` - line 278
- `ImGui::GetContentRegionAvail()` - line 313
- `ImGui::PushTextWrapPos(wrap_pos_x)` - line 352
- `ImGui::PopTextWrapPos()` - paired with above
- `ImGui::SmallButton(label)` - line 422
- `ImGui::SetClipboardText(text)` - line 720
- `ImGui::TextWrapped(fmt, ...)` - line 411
- `ImGui::PushStyleColor(idx, col)` / `PopStyleColor()` - style management
- `ImGui::BeginGroup()` / `EndGroup()` - grouping panels
- `ImGui::SameLine()` - horizontal layout

### Verified Codebase Patterns
- `UI::StatusMessage::ShowSuccess()` - from `StatusMessageUI.h:117`
- `UI::Helpers::SetCVarSafely()` - from `HelpersUI.h:39`
- `browserStatus` member variable - already exists in `TrainingPackUI.h:115`
- Difficulty color mapping - exists in `TrainingPackUI.cpp:574-588`

## Files to Modify

1. **`TrainingPackUI.cpp`** - Main render logic (primary changes)
2. **`TrainingPackUI.h`** - Add details panel state (description expand toggle)
3. **`ConstantsUI.h`** - Add new constants for two-panel layout

## Implementation Steps

### Step 1: Add Constants to `ConstantsUI.h`

Add to `namespace UI::TrainingPackUI` (after line 306, before closing brace):
```cpp
// === Two-Panel Layout (following WorkshopBrowserUI pattern) ===
constexpr float LEFT_PANEL_WIDTH_PERCENT = 0.60f;    // 60% for pack list
constexpr float LEFT_PANEL_MIN_WIDTH = 400.0f;       // Minimum list panel width
constexpr float RIGHT_PANEL_MIN_WIDTH = 280.0f;      // Minimum details panel width
constexpr float BROWSER_PANEL_HEIGHT = 400.0f;       // Height of two-panel area
constexpr int DESCRIPTION_MAX_CHARS = 200;           // Chars before "More" toggle

// === Details Panel Colors (high contrast for screenshots) ===
inline const ImVec4 DETAILS_HEADER_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);           // White
inline const ImVec4 DETAILS_LABEL_COLOR = ImVec4(0.7f, 0.85f, 1.0f, 1.0f);           // Light blue
inline const ImVec4 DETAILS_VALUE_COLOR = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);         // Near-white
inline const ImVec4 DETAILS_TAGS_COLOR = ImVec4(0.6f, 0.9f, 0.6f, 1.0f);             // Light green
inline const ImVec4 DETAILS_CODE_COLOR = ImVec4(0.9f, 0.9f, 0.6f, 1.0f);             // Light yellow
inline const ImVec4 DETAILS_DESCRIPTION_COLOR = ImVec4(0.88f, 0.88f, 0.88f, 1.0f);   // Light gray
inline const ImVec4 DETAILS_NO_SELECTION_COLOR = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);     // Dim gray
```

### Step 2: Add State to `TrainingPackUI.h`

Add member variable (after line 102, near `lastSelectedRowIndex`):
```cpp
bool descriptionExpanded_ = false;  // For More/Less toggle in details panel
```

Add private method declaration (after line 122, near other helper methods):
```cpp
void RenderDetailsPanel();
ImVec4 GetDifficultyColor(const std::string& difficulty) const;
```

### Step 3: Refactor `TrainingPackUI::Render()` Layout

**Current structure:**
```
[Header + Status + Buttons]
[Add Custom Pack (collapsible)]
[Filters]
[Action Bar]
[Full-width Table]
```

**New structure:**
```
[Header + Status + Buttons]
[Add Custom Pack (collapsible)]
[Filters]
[Filtered count + status]
[Two-Panel: Left=Table | Right=Details]
```

#### Layout Changes in Render() (replace lines ~320-613)

After the filters section (around line 317 "Showing %d of %d packs"), replace the action bar and table with:

```cpp
// Display filtered count
ImGui::Text("Showing %d of %d packs", (int)filteredPacks.size(), packCount);
ImGui::Spacing();

// Render browser status message
browserStatus.Render(ImGui::GetIO().DeltaTime);
if (browserStatus.IsVisible()) ImGui::Spacing();

ImGui::Separator();
ImGui::Spacing();

// === TWO-PANEL LAYOUT (following WorkshopBrowserUI pattern) ===

// Calculate panel widths
float availWidth = ImGui::GetContentRegionAvail().x;
float leftWidth = std::max(UI::TrainingPackUI::LEFT_PANEL_MIN_WIDTH,
                           availWidth * UI::TrainingPackUI::LEFT_PANEL_WIDTH_PERCENT);
float rightWidth = availWidth - leftWidth - ImGui::GetStyle().ItemSpacing.x;

// Enforce minimum right panel width
if (rightWidth < UI::TrainingPackUI::RIGHT_PANEL_MIN_WIDTH) {
    rightWidth = UI::TrainingPackUI::RIGHT_PANEL_MIN_WIDTH;
    leftWidth = availWidth - rightWidth - ImGui::GetStyle().ItemSpacing.x;
}

// Group the two panels together
ImGui::BeginGroup();

// === LEFT PANEL: Pack List ===
if (ImGui::BeginChild("PackListPanel", ImVec2(leftWidth, UI::TrainingPackUI::BROWSER_PANEL_HEIGHT), true)) {
    // Column header row (frozen)
    ImGui::Columns(activeColumnCount, "PackColumns_Header", true);
    for (int i = 0; i < activeColumnCount && i < (int)columnWidths.size(); i++) {
        ImGui::SetColumnWidth(i, columnWidths[i] * (leftWidth / lastWindowWidth));
    }

    // [Existing column header code: Name, Code, Difficulty, Shots, Likes, Plays]
    // ... (keep existing SortableColumnHeader calls) ...

    ImGui::Columns(1);
    ImGui::Separator();

    // Scrollable pack rows
    ImGui::BeginChild("PackRows", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Columns(activeColumnCount, "PackColumns_Body", true);

    ImGuiListClipper clipper;
    clipper.Begin((int)filteredPacks.size());

    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
            const auto& pack = filteredPacks[row];
            bool isSelected = (selectedPackCode == pack.code);

            ImGui::PushID(pack.code.c_str());

            // [Existing row rendering code - video button, selectable, etc.]
            // Key change: On click, just update selection (remove popup)
            if (ImGui::Selectable(pack.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                selectedPackCode = pack.code;
                lastSelectedRowIndex = row;
                descriptionExpanded_ = false;  // Reset expansion on new selection
            }

            // [Keep existing drag-drop source code]
            // [Keep existing right-click context menu code]

            // [Existing column rendering: Code, Difficulty, Shots, Likes, Plays]

            ImGui::PopID();
        }
    }

    ImGui::Columns(1);
    ImGui::EndChild();  // PackRows
}
ImGui::EndChild();  // PackListPanel

ImGui::SameLine();

// === RIGHT PANEL: Details ===
if (ImGui::BeginChild("DetailsPanel", ImVec2(rightWidth, UI::TrainingPackUI::BROWSER_PANEL_HEIGHT), true)) {
    RenderDetailsPanel();
}
ImGui::EndChild();  // DetailsPanel

ImGui::EndGroup();

ImGui::Spacing();
ImGui::End();
```

### Step 4: Create `RenderDetailsPanel()` Method

Add new private method in `TrainingPackUI.cpp` (after `LoadPackImmediately()`, around line 896):

```cpp
void TrainingPackUI::RenderDetailsPanel() {
    // Find selected pack in filtered list
    const TrainingEntry* selected = nullptr;
    for (const auto& pack : filteredPacks) {
        if (pack.code == selectedPackCode) {
            selected = &pack;
            break;
        }
    }

    // No selection placeholder
    if (!selected) {
        ImGui::TextColored(UI::TrainingPackUI::DETAILS_NO_SELECTION_COLOR,
            "Select a pack from the list");
        return;
    }

    // === HEADER: Pack Name ===
    ImGui::PushStyleColor(ImGuiCol_Text, UI::TrainingPackUI::DETAILS_HEADER_COLOR);
    ImGui::TextWrapped("%s", selected->name.c_str());
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // === PACK CODE (click to copy) ===
    ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Code:");
    ImGui::SameLine();
    ImGui::TextColored(UI::TrainingPackUI::DETAILS_CODE_COLOR, "%s", selected->code.c_str());
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
        ImGui::SetClipboardText(selected->code.c_str());
        browserStatus.ShowSuccess("Code copied!", 1.5f, UI::StatusMessage::DisplayMode::TimerWithFade);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click to copy");
    }

    ImGui::Spacing();

    // === STATS ===
    ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Difficulty:");
    ImGui::SameLine();
    ImGui::TextColored(GetDifficultyColor(selected->difficulty), "%s",
        selected->difficulty.empty() ? "Unranked" : selected->difficulty.c_str());

    ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Shots:");
    ImGui::SameLine();
    ImGui::TextColored(UI::TrainingPackUI::DETAILS_VALUE_COLOR, "%d", selected->shotCount);

    ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Likes:");
    ImGui::SameLine();
    ImGui::TextColored(UI::TrainingPackUI::DETAILS_VALUE_COLOR, "%d", selected->likes);

    ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Plays:");
    ImGui::SameLine();
    ImGui::TextColored(UI::TrainingPackUI::DETAILS_VALUE_COLOR, "%d", selected->plays);

    // === CREATOR ===
    if (!selected->creator.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Creator:");
        ImGui::SameLine();
        ImGui::TextColored(UI::TrainingPackUI::DETAILS_VALUE_COLOR, "%s", selected->creator.c_str());
    }

    // === TAGS ===
    if (!selected->tags.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Tags:");
        ImGui::SameLine();
        std::string tagStr;
        for (size_t i = 0; i < selected->tags.size(); i++) {
            if (i > 0) tagStr += ", ";
            tagStr += selected->tags[i];
        }
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextColored(UI::TrainingPackUI::DETAILS_TAGS_COLOR, "%s", tagStr.c_str());
        ImGui::PopTextWrapPos();
    }

    // === DESCRIPTION (with More/Less toggle) ===
    std::string description = selected->staffComments.empty() ? selected->notes : selected->staffComments;
    if (!description.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(UI::TrainingPackUI::DETAILS_LABEL_COLOR, "Description:");

        ImGui::PushStyleColor(ImGuiCol_Text, UI::TrainingPackUI::DETAILS_DESCRIPTION_COLOR);
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);

        const int maxChars = UI::TrainingPackUI::DESCRIPTION_MAX_CHARS;
        if (!descriptionExpanded_ && (int)description.length() > maxChars) {
            ImGui::TextWrapped("%s...", description.substr(0, maxChars).c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            if (ImGui::SmallButton("More")) {
                descriptionExpanded_ = true;
            }
        } else {
            ImGui::TextWrapped("%s", description.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            if ((int)description.length() > maxChars) {
                if (ImGui::SmallButton("Less")) {
                    descriptionExpanded_ = false;
                }
            }
        }
    }

    // === VIDEO LINK ===
    if (!selected->videoUrl.empty()) {
        ImGui::Spacing();
        if (ImGui::Button("Watch Video", ImVec2(-1, 0))) {
            ShellExecuteA(NULL, "open", selected->videoUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open video tutorial in browser");
        }
    }

    // === ACTION BUTTONS ===
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // LOAD NOW button (prominent green)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
    if (ImGui::Button("LOAD NOW", ImVec2(-1, 30))) {
        LoadPackImmediately(selectedPackCode);
    }
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Load this pack immediately");
    }

    // Set as Post-Match button
    if (ImGui::Button("Set as Post-Match", ImVec2(-1, 0))) {
        plugin_->settingsSync->SetQuickPicksSelected(selectedPackCode);
        plugin_->cvarManager->getCvar("suitespot_quickpicks_selected").setValue(selectedPackCode);
        plugin_->settingsSync->SetCurrentTrainingCode(selectedPackCode);
        plugin_->cvarManager->getCvar("suitespot_current_training_code").setValue(selectedPackCode);
        browserStatus.ShowSuccess("Post-Match set: " + selected->name, 2.0f,
            UI::StatusMessage::DisplayMode::TimerWithFade);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Load this pack after matches end");
    }

    // Set as Auto-Load button
    if (ImGui::Button("Set as Auto-Load", ImVec2(-1, 0))) {
        plugin_->settingsSync->SetCurrentTrainingCode(selectedPackCode);
        plugin_->cvarManager->getCvar("suitespot_current_training_code").setValue(selectedPackCode);
        browserStatus.ShowSuccess("Auto-Load set: " + selected->name, 2.0f,
            UI::StatusMessage::DisplayMode::TimerWithFade);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Set this pack as the auto-load target");
    }

    // Delete button (only for custom packs)
    if (selected->source == "custom") {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, UI::TrainingPackUI::DELETE_BUTTON_BG_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UI::TrainingPackUI::DELETE_BUTTON_HOVER_COLOR);
        if (ImGui::Button("Delete Custom Pack", ImVec2(-1, 0))) {
            if (plugin_->trainingPackMgr) {
                plugin_->trainingPackMgr->DeletePack(selectedPackCode);
                browserStatus.ShowSuccess("Deleted custom pack", 3.0f,
                    UI::StatusMessage::DisplayMode::TimerWithFade);
                selectedPackCode = "";
            }
        }
        ImGui::PopStyleColor(2);
    }

    // Clear selection button
    ImGui::Spacing();
    if (ImGui::Button("Clear Selection", ImVec2(-1, 0))) {
        selectedPackCode = "";
    }
}
```

### Step 5: Add Helper Method for Difficulty Colors

Add to `TrainingPackUI.cpp` (after `RenderDetailsPanel()`):

```cpp
ImVec4 TrainingPackUI::GetDifficultyColor(const std::string& difficulty) const {
    // Uses existing color constants from ConstantsUI.h (verified at lines 247-271)
    if (difficulty == "Bronze") return UI::TrainingPackUI::DIFFICULTY_BADGE_BRONZE_COLOR;
    if (difficulty == "Silver") return UI::TrainingPackUI::DIFFICULTY_BADGE_SILVER_COLOR;
    if (difficulty == "Gold") return UI::TrainingPackUI::DIFFICULTY_BADGE_GOLD_COLOR;
    if (difficulty == "Platinum") return UI::TrainingPackUI::DIFFICULTY_BADGE_PLATINUM_COLOR;
    if (difficulty == "Diamond") return UI::TrainingPackUI::DIFFICULTY_BADGE_DIAMOND_COLOR;
    if (difficulty == "Champion") return UI::TrainingPackUI::DIFFICULTY_BADGE_CHAMPION_COLOR;
    if (difficulty == "Grand Champion") return UI::TrainingPackUI::DIFFICULTY_BADGE_GRAND_CHAMPION_COLOR;
    if (difficulty == "Supersonic Legend") return UI::TrainingPackUI::DIFFICULTY_BADGE_SUPERSONIC_LEGEND_COLOR;
    return UI::TrainingPackUI::DIFFICULTY_BADGE_UNRANKED_COLOR;
}
```

### Step 6: Remove Redundant Action Bar

The current action bar (lines 320-368 in `TrainingPackUI.cpp`) has buttons now in details panel:
- **Delete Custom Pack** → Now in details panel (only shown for custom packs)
- **Clear Selection** → Now in details panel

**Remove** lines 320-368 (the entire action bar section).

### Step 7: Simplify Row Click Behavior

In the pack list, **remove** the popup menu on single click (lines 480-513). Selection should just update the details panel.

**Keep** the right-click context menu (lines 527-538) for quick "Set as Auto-Load" access.

### Step 8: Update Window Default Size

Change `TrainingPackUI.cpp:61`:
```cpp
// Before:
ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

// After:
ImGui::SetNextWindowSize(ImVec2(1050, 650), ImGuiCond_FirstUseEver);
```

## Dependencies

- **No external dependencies required** - All APIs from bundled ImGui 1.75 and BakkesMod SDK
- **No vcpkg packages needed** - Uses only existing project dependencies
- **No icons8 icons needed** - Text-based UI, existing YouTube icon already handles video

## Preserved Features

| Feature | Status |
|---------|--------|
| Filters (search, difficulty, tags, shots, video) | Preserved above left panel |
| Pack list with all columns (Name, Difficulty, Shots, Likes, Plays) | Preserved in left panel |
| Sorting by clicking column headers | Preserved |
| Drag-and-drop from list | Preserved (legacy code, bag feature was removed) |
| Right-click context menu | Preserved ("Set as Auto-Load") |
| Popup menu on row click | **Removed** (replaced by details panel) |
| Custom pack form | Preserved (collapsible header) |
| LOAD NOW | Moved to details panel (prominent green button) |
| Set Post-Match | Moved to details panel |
| Set Auto-Load | Moved to details panel + kept in right-click menu |
| Delete Custom Pack | Moved to details panel (only for `source == "custom"`) |
| Video button (YouTube) | Preserved in list + added to details panel |
| Usage tracking | Preserved (`LoadPackImmediately` calls `usageTracker`) |
| Code column (conditional) | Preserved (shown when search contains digits) |

## Verification Checklist

### Build Verification
- [ ] Build succeeds with `msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64`
- [ ] No new compiler warnings

### Functional Tests
- [ ] **Select row updates details panel** - Click any row, details panel shows pack info
- [ ] **Selection persists across filter changes** - Filter list, selected pack highlighted if visible
- [ ] **Filters still work** - Search, difficulty, tags, min shots, video filter all work
- [ ] **Sorting still works** - Click column header, list re-sorts, selection preserved
- [ ] **LOAD NOW still works** - Click in details panel, pack loads in game
- [ ] **Set Post-Match works** - Click button, status shows success, CVar updated
- [ ] **Set Auto-Load works** - Click button, status shows success, CVar updated
- [ ] **Drag-drop source preserved** - Drag from list row still initiates (legacy code, no target)
- [ ] **Right-click context menu works** - Right-click row shows "Set as Auto-Load"
- [ ] **Video button works** - Click "Watch Video", browser opens YouTube link
- [ ] **Custom pack form works** - Add custom pack via collapsible form

### Edge Cases
- [ ] **No crashes when list is empty** - Clear filters to 0 results, no crash
- [ ] **No crashes when selection is null** - Start with no selection, panel shows placeholder
- [ ] **Selection clears on delete** - Delete custom pack, selection clears
- [ ] **Description More/Less toggle** - Long descriptions truncate, More button expands
- [ ] **Code copy on click** - Click pack code in details, clipboard updated

### Visual Tests
- [ ] **Strong text contrast** - All text readable (no alpha < 0.8)
- [ ] **Right panel sufficient width** - 280px minimum, text wraps properly
- [ ] **Headers visually distinct** - Pack name white, labels light blue
- [ ] **Difficulty colors correct** - Badge colors match rank colors
- [ ] **Window resizes properly** - Panels scale with window width
