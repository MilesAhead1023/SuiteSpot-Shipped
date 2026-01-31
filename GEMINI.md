# GEMINI.md - SuiteSpot Technical Reference

Comprehensive technical reference for the SuiteSpot BakkesMod plugin. This document provides architectural context, implementation patterns, and API references.

## Project Overview

**SuiteSpot** is a BakkesMod plugin that automates Rocket League training workflows by seamlessly transitioning players from matches to their preferred training environment (freeplay maps, training packs, or workshop maps).

**Key Value Propositions:**
- **Zero-friction training:** Match ends → training loads automatically
- **2000+ curated training packs:** Pre-scraped database with metadata, tags, difficulty ratings
- **Workshop integration:** Download and manage custom maps from RLMAPS API
- **Smart persistence:** Remembers your preferences, tracks pack usage statistics

---

## 🏗️ Architecture Overview

### Design Pattern: Hub-and-Spoke

```
         SuiteSpot.cpp (Hub)
                |
    +-----------+-----------+
    |           |           |
MapManager  SettingsSync  AutoLoad
    |           |           |
Training    Workshop   Loadout
PackMgr    Downloader  Manager
```

**The Hub (`SuiteSpot.cpp`):**
- Entry point registered via `BAKKESMOD_PLUGIN` macro (line 153)
- Manages plugin lifecycle: `onLoad()` (line 319), `onUnload()` (line 367)
- Coordinates event hooks (match-end, training pack load)
- Bridges BakkesMod framework to custom features

**The Spokes (Managers):**
Each manager is a specialized subsystem with a single responsibility.

---

## 📁 Component Breakdown

### Core Components (Backend Logic)

| Component | Files | Responsibility | Key Methods |
|-----------|-------|----------------|-------------|
| **SettingsSync** | SettingsSync.cpp/h | Manages all CVars (plugin settings) | `RegisterAllCVars()`, `IsEnabled()`, `GetMapType()` |
| **AutoLoadFeature** | AutoLoadFeature.cpp/h | Match-end automation logic | `OnMatchEnded()` (scheduled via `SetTimeout`) |
| **MapManager** | MapManager.cpp/h | Discovers workshop `.udk` files on disk | `LoadWorkshopMaps()`, `DiscoverWorkshopInDir()` |
| **TrainingPackManager** | TrainingPackManager.cpp/h | CRUD for 2000+ pack database | `LoadPacksFromFile()`, `UpdateTrainingPackList()`, `HealPack()` |
| **WorkshopDownloader** | WorkshopDownloader.cpp/h | Downloads maps from RLMAPS API | `DownloadMap()`, `FetchMapInfo()` |
| **LoadoutManager** | LoadoutManager.cpp/h | Switches car presets | `LoadLoadout()` |
| **PackUsageTracker** | PackUsageTracker.cpp/h | Tracks pack load counts, timestamps | `IncrementLoadCount()`, `SaveStats()` |

### UI Components (Frontend Rendering)

| Component | Files | Description | Render Location |
|-----------|-------|-------------|-----------------|
| **SettingsUI** | SettingsUI.cpp/h | F2 menu settings panel | `SuiteSpot::RenderSettings()` |
| **TrainingPackUI** | TrainingPackUI.cpp/h | Standalone floating browser window | `TrainingPackUI::Render()` |
| **LoadoutUI** | LoadoutUI.cpp/h | Car loadout picker | Embedded in SettingsUI |
| **StatusMessageUI** | StatusMessageUI.cpp/h | Toast notifications | Called by UI components |
| **HelpersUI** | HelpersUI.cpp/h | Shared UI utilities | Static helpers (e.g., `SetCVarSafely()`) |

### Data Structures

| File | Description |
|------|-------------|
| **MapList.h** | Defines `MapEntry`, `TrainingEntry`, `WorkshopEntry` structs |
| **DefaultPacks.h** | Embedded list of 50 starter training packs |
| **ConstantsUI.h** | UI constants: colors, dimensions, difficulty levels, paths |
| **version.h** | Auto-generated version info (major.minor.patch.build) |

---

## 🔧 Build System

### Build Environment

- **Compiler:** MSVC v143 (Visual Studio 2022)
- **Platform:** Windows x64 only (Rocket League requirement)
- **Language Standard:** C++20 (`/std:c++20`)
- **Runtime Library:** MultiThreaded (Release), MultiThreadedDebug (Debug)
- **Output Type:** Dynamic Link Library (.dll)

### Build Commands

**Via MSBuild (command line):**
```bash
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

**Via Visual Studio:**
1. Open `SuiteSpot.sln`
2. Set configuration to **Release | x64**
3. Press **F7** to build

### Build Pipeline (Automatic Steps)

1. **Pre-Build Event** (line 92-94 in `.vcxproj`):
   ```powershell
   powershell.exe -ExecutionPolicy Bypass -File CompiledScripts/update_version.ps1 "./version.h"
   ```
   - Auto-increments `VERSION_BUILD` in `version.h`
   - Updates build timestamp

2. **Compilation:**
   - Compiles all `.cpp` files with precompiled headers (`pch.h`)
   - Links against BakkesMod SDK (via `BakkesMod.props`)
   - Outputs to `plugins/SuiteSpot.dll`

3. **Post-Build Event** (lines 95-123):
   - Creates data directories in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\`
   - Copies `Resources/` folder (images, scripts) to data dir
   - Runs `bakkesmod-patch.exe` to inject DLL into BakkesMod

### Dependencies

**System Dependencies:**
- BakkesMod SDK (path from registry: `HKLM\SOFTWARE\BakkesMod`)
- Windows SDK 10.0
- vcpkg at `C:\Users\bmile\vcpkg`

**Code Dependencies:**
- **BakkesMod SDK:** Game hooks, CVars, wrappers (via `BakkesMod.props`)
- **ImGui v1.75:** Bundled in `IMGUI/` folder (DirectX 11 backend)
- **nlohmann/json:** Installed via vcpkg (`C:\Users\bmile\vcpkg\installed\x64-windows\include`)
- **PowerShell:** Used for zip extraction (`Expand-Archive`) and version updates

---

## 🧠 Critical Implementation Patterns

### 1. CVar Naming Convention

All plugin settings use the `suitespot_` prefix:

```cpp
// Examples from SettingsSync.cpp
cvarManager->registerCvar("suitespot_enabled", "0");
cvarManager->registerCvar("suitespot_delay_queue", "3");
cvarManager->registerCvar("suitespot_current_training_code", "");
```

**Why?** Prevents namespace collisions with other BakkesMod plugins.

### 2. Thread Safety

**Problem:** ImGui runs on the render thread, but data updates happen on background threads.

**Solutions used in this codebase:**

```cpp
// TrainingPackManager.h - Protects pack list access
std::mutex packMutex_;

// SuiteSpot.h - Prevents concurrent render issues
std::atomic<bool> isRenderingSettings{false};

// Usage pattern
{
    std::lock_guard<std::mutex> lock(packMutex_);
    // Safe to read/write packs here
}
```

### 3. Deferred Execution (Critical for Game Stability)

**⚠️ NEVER load maps/packs immediately after match-end!** The game crashes if you run `load_freeplay` or `load_training` during the match-end sequence.

**Correct pattern:**

```cpp
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    cvarManager->executeCommand("load_freeplay DFH_Stadium_P");
}, delaySeconds);
```

**Where this appears:**
- `AutoLoadFeature.cpp` - All `OnMatchEnded()` logic uses `SetTimeout`
- `SuiteSpot.cpp:179-182` - Pack healer uses 1.5s delay

### 4. CVar Callback Pattern (Fast Read Access)

**Pattern:** Sync CVar changes to local member variables immediately.

```cpp
// SettingsSync.cpp:42-47
auto enabledCvar = cvarManager->getCvar("suitespot_enabled");
enabledCvar.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
    isEnabled = cvar.getBoolValue();  // Cache locally
});
```

**Why?** CVarManager lookups are slow. Local caching enables fast reads during the 60 FPS render loop.

### 5. Data Persistence Locations

All data stored under `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\`:

```
SuiteSpot/
├── TrainingSuite/
│   ├── training_packs.json       (2000+ packs with metadata)
│   ├── pack_usage_stats.json     (load counts, timestamps)
│   └── workshop_loader_config.json
└── Resources/
    ├── SuitePackGrabber.ps1      (PowerShell scraper)
    └── icons/                    (YouTube icon, etc.)
```

**Access pattern:**

```cpp
auto dataRoot = gameWrapper->GetDataFolder();  // Returns %APPDATA%\bakkesmod\bakkesmod\data
auto packPath = dataRoot / L"SuiteSpot" / L"TrainingSuite" / L"training_packs.json";
```

### 6. ImGui Virtual Scrolling (Performance)

**Problem:** 2000+ pack list would tank FPS if all rendered.

**Solution:** `ImGuiListClipper` only renders visible rows.

```cpp
// TrainingPackUI.cpp (existing pattern)
ImGuiListClipper clipper;
clipper.Begin((int)filteredPacks.size());
while (clipper.Step()) {
    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
        // Render only visible rows
    }
}
```

**Performance:** ~60 FPS with 2000+ packs, only ~10-15 rows rendered per frame.

---

## 📂 File Organization

### Root Directory Structure

```
SuiteSpot/
├── SuiteSpot.cpp/h              # Hub: plugin lifecycle, event coordination
├── pch.h/cpp                    # Precompiled headers
├── version.h                    # Auto-generated build version
│
├── Core/                        # Backend logic
│   ├── AutoLoadFeature.cpp/h
│   ├── MapManager.cpp/h
│   ├── SettingsSync.cpp/h
│   ├── TrainingPackManager.cpp/h
│   ├── WorkshopDownloader.cpp/h
│   ├── LoadoutManager.cpp/h
│   └── PackUsageTracker.cpp/h
│
├── UI/                          # Frontend rendering
│   ├── SettingsUI.cpp/h
│   ├── TrainingPackUI.cpp/h
│   ├── LoadoutUI.cpp/h
│   ├── StatusMessageUI.cpp/h
│   └── HelpersUI.cpp/h
│
├── Data/                        # Structures and constants
│   ├── MapList.h
│   ├── DefaultPacks.h
│   └── ConstantsUI.h
│
├── IMGUI/                       # ImGui v1.75 (bundled)
│   ├── imgui.cpp/h
│   ├── imgui_impl_dx11.cpp/h
│   ├── imgui_impl_win32.cpp/h
│   └── [various widgets]
│
├── CompiledScripts/             # PowerShell automation
│   └── update_version.ps1
│
├── Resources/                   # Assets
│   ├── SuitePackGrabber.ps1
│   └── icons/
│
└── docs/                        # Documentation
    ├── architecture.md
    ├── bakkesmod-sdk-reference.md
    └── plans/
        └── training-browser-two-panel-layout.md
```

---

## 🔍 Understanding the Data Flow

### Training Pack Workflow

1. **Initialization (`SuiteSpot::onLoad()` - line 319):**
   ```
   Create TrainingPackManager
   → Check if training_packs.json exists
   → If yes: LoadPacksFromFile()
   → If no: Schedule scraping for next UI render
   ```

2. **User Opens Browser:**
   ```
   User presses togglemenu button
   → TrainingPackUI::Render() called
   → Filters applied to RLTraining vector
   → ImGuiListClipper renders visible rows
   → User clicks row → selectedPackCode updated
   ```

3. **User Loads Pack:**
   ```
   User clicks "LOAD NOW"
   → LoadPackImmediately() called
   → cvarManager->executeCommand("load_training <code>")
   → usageTracker->IncrementLoadCount()
   → pack_usage_stats.json updated
   ```

4. **Auto-Load After Match:**
   ```
   Match ends
   → GameEndedEvent() hook triggered
   → AutoLoadFeature::OnMatchEnded() called
   → SetTimeout(delaySeconds) scheduled
   → After delay: cvarManager->executeCommand("load_training <code>")
   → usageTracker->IncrementLoadCount()
   ```

5. **Pack Healing (Shot Count Correction):**
   ```
   Pack loaded in-game
   → "Function TAGame.GameEvent_TrainingEditor_TA.OnInit" hook
   → SetTimeout(1.5s) → TryHealCurrentPack()
   → Extract real shot count from TrainingEditorWrapper
   → trainingPackMgr->HealPack(code, realShots)
   → Update training_packs.json with correct shot count
   ```

### Workshop Map Workflow

1. **Discovery:**
   ```
   SuiteSpot::LoadWorkshopMaps()
   → MapManager::DiscoverWorkshopInDir(path)
   → Recursively scan for *.udk and *.upk files
   → Read metadata from JSON or folder names
   → Populate RLWorkshop vector
   ```

2. **Downloading:**
   ```
   User enters Steam Workshop ID
   → WorkshopDownloader::DownloadMap(id)
   → API request to https://celab.jetfox.ovh/api/v4/projects/{id}
   → Parse release data, find .zip URL
   → Download .zip to temp directory
   → system("powershell.exe Expand-Archive ...")
   → Move extracted .udk to configured workshop root
   → Refresh workshop list
   ```

---

## 🎨 UI Architecture (ImGui Patterns)

### Inheritance Hierarchy

```
BakkesModPlugin (framework base)
    ↓
SettingsWindowBase (ImGui context handler)
    ↓
SuiteSpot (multi-interface implementation)
    ├─ implements PluginSettingsWindow → F2 settings tab
    ├─ implements PluginWindow → standalone windows
```

### Render Chain

**F2 Settings Menu:**
```
BakkesMod Framework
→ SuiteSpot::RenderSettings() (Source.cpp)
→ SettingsUI::Render()
    → Workshop Browser (two-panel layout)
    → Main settings form (checkboxes, sliders)
    → LoadoutUI::RenderLoadoutPicker()
```

**Standalone Browser Window:**
```
BakkesMod Framework
→ TrainingPackUI::Render() (registered as PluginWindow)
    → Filters section
    → Pack list table
    → Details panel (after two-panel redesign)
```

### ImGui Widget Reference (Verified v1.75 APIs)

**Layout:**
- `BeginChild(str_id, size, border, flags)` / `EndChild()` - Nested panels
- `BeginGroup()` / `EndGroup()` - Logical grouping
- `SameLine()` - Horizontal layout
- `GetContentRegionAvail()` - Available space for child widgets
- `Columns(count, id, border)` - Multi-column tables
- `SetColumnWidth(idx, width)` - Column sizing

**Text:**
- `Text(fmt, ...)` - Plain text
- `TextColored(color, fmt, ...)` - Colored text
- `TextWrapped(fmt, ...)` - Auto-wrapping text
- `PushTextWrapPos(wrap_x)` / `PopTextWrapPos()` - Manual wrap control

**Interaction:**
- `Button(label, size)` - Clickable button
- `SmallButton(label)` - Compact button
- `Selectable(label, selected, flags)` - Row selection
- `IsItemHovered()` - Mouse over check
- `IsMouseClicked(button)` - Click detection
- `SetTooltip(fmt, ...)` - Hover tooltip

**Clipboard:**
- `SetClipboardText(text)` - Copy to clipboard

**Styling:**
- `PushStyleColor(idx, col)` / `PopStyleColor(count)` - Temporary color overrides
- `PushID(str_id)` / `PopID()` - Unique widget IDs in loops

---

## 🔑 Key Patterns You'll Use Repeatedly

### Pattern 1: Safe CVar Access

```cpp
// CORRECT: Check for null before using
auto cvar = cvarManager->getCvar("suitespot_enabled");
if (cvar) {
    bool value = cvar.getBoolValue();
}

// INCORRECT: Will crash if CVar doesn't exist
bool value = cvarManager->getCvar("suitespot_enabled").getBoolValue();
```

**Helper available:** `UI::Helpers::SetCVarSafely()` in `HelpersUI.h:39`

### Pattern 2: Logging

```cpp
#include "logging.h"

LOG("Simple message");
LOG("Formatted message: {}, {}", variable1, variable2);
```

**Output:** Writes to BakkesMod console (accessible via F6 in-game).

### Pattern 3: Status Messages (Toast Notifications)

```cpp
// In TrainingPackUI.h, StatusMessageUI.h is included
UI::StatusMessage browserStatus;  // Member variable

// In your code
browserStatus.ShowSuccess("Pack loaded!", 2.0f, UI::StatusMessage::DisplayMode::TimerWithFade);
browserStatus.ShowError("Failed to load pack", 3.0f, UI::StatusMessage::DisplayMode::TimerWithFade);

// In Render()
browserStatus.Render(ImGui::GetIO().DeltaTime);
```

### Pattern 4: Accessing Game State

```cpp
// Check if in training
if (!gameWrapper->IsInCustomTraining()) {
    LOG("Not in training mode");
    return;
}

// Get training data
auto server = gameWrapper->GetGameEventAsServer();
if (!server) return;

TrainingEditorWrapper editor(server.memory_address);
auto trainingData = editor.GetTrainingData();
```

**Common wrappers:**
- `GameWrapper` - Main game state
- `ServerWrapper` - Server/match data
- `TrainingEditorWrapper` - Training pack data
- `CVarManagerWrapper` - Settings access

### Pattern 5: File I/O with nlohmann/json

```cpp
#include <nlohmann/json.hpp>
#include <fstream>

// Read JSON
std::ifstream file(path);
nlohmann::json data = nlohmann::json::parse(file);
std::string name = data["name"];

// Write JSON
nlohmann::json output;
output["code"] = "ABCD-1234-EFGH-5678";
std::ofstream outFile(path);
outFile << output.dump(2);  // Pretty print with 2-space indent
```

### Pattern 6: Event Hooks

```cpp
// Register hook during onLoad()
gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    [this](std::string eventName) {
        GameEndedEvent(eventName);
    });

// Register console command
cvarManager->registerNotifier("ss_heal_current_pack",
    [this](std::vector<std::string> args) {
        TryHealCurrentPack(gameWrapper.get());
    },
    "Manually heal the currently loaded training pack",
    PERMISSION_ALL);
```

**Common hooks:**
- `EventMatchEnded` - Match completion
- `GameEvent_TrainingEditor_TA.OnInit` - Training pack loaded
- `TrainingShotAttempt` - Player starts shot attempt

---

## 🚨 Critical "Gotchas" and Warnings

### ❌ Do NOT Do These Things

1. **DO NOT load maps immediately after match-end**
   - Always use `SetTimeout()` with minimum 0.5s delay
   - Immediate loading crashes the game

2. **DO NOT use `git add -A` or `git add .` for commits**
   - Stage specific files only: `git add <file1> <file2>`
   - Prevents accidental commits of `.env`, credentials, or build artifacts

3. **DO NOT use PowerShell's `-i` flag**
   - Interactive flags don't work in system() calls
   - Use `-NonInteractive` instead

4. **DO NOT modify git config or run destructive git commands**
   - No `push --force`, `reset --hard`, `clean -f` unless explicitly requested
   - NEVER skip hooks (`--no-verify`)

5. **DO NOT assume CVar exists**
   - Always null-check: `auto cvar = cvarManager->getCvar("name"); if (cvar) { ... }`

6. **DO NOT block the render thread**
   - Long operations (downloads, scraping) must run in background threads
   - Use `std::thread` or PowerShell scripts via `system()`

### ✅ Do These Things

1. **DO use verified APIs only**
   - Check `IMGUI/imgui.h` for ImGui signatures
   - Check `docs/bakkesmod-sdk-reference.md` for BakkesMod wrappers
   - Never hallucinate API parameters

2. **DO use vcpkg for external dependencies**
   - Install at `C:\Users\bmile\vcpkg`
   - Update `.vcxproj` AdditionalIncludeDirectories

3. **DO use icons8 for icons**
   - Download from icons8.com
   - Place in `Resources/icons/`

4. **DO preserve all existing features**
   - Check `docs/plans/` for feature inventories
   - Never remove functionality unless explicitly requested

5. **DO log important state changes**
   ```cpp
   LOG("SuiteSpot: AutoLoad triggered - mode: {}, code: {}", mapType, code);
   ```

6. **DO ensure thread safety for shared data**
   - Training pack lists, workshop maps, usage stats
   - Use `std::lock_guard<std::mutex>`

---

## 🧪 Common Development Workflows

### Adding a New CVar

1. **Register in `SettingsSync.cpp:RegisterAllCVars()`:**
   ```cpp
   cvarManager->registerCvar("suitespot_new_setting", "default_value",
       "Description shown in console", true, true);
   ```

2. **Add local member variable in `SettingsSync.h`:**
   ```cpp
   bool newSetting = false;
   ```

3. **Add callback in `RegisterAllCVars()`:**
   ```cpp
   auto newCvar = cvarManager->getCvar("suitespot_new_setting");
   newCvar.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
       newSetting = cvar.getBoolValue();
   });
   ```

4. **Add getter in `SettingsSync.h`:**
   ```cpp
   bool GetNewSetting() const { return newSetting; }
   ```

5. **Add UI control in `SettingsUI.cpp`:**
   ```cpp
   if (ImGui::Checkbox("Enable New Feature", &plugin_->settingsSync->newSetting)) {
       UI::Helpers::SetCVarSafely(plugin_->cvarManager, "suitespot_new_setting",
           plugin_->settingsSync->newSetting);
   }
   ```

### Adding a New Training Pack Field

1. **Update `MapList.h` TrainingEntry struct:**
   ```cpp
   struct TrainingEntry {
       std::string code, name, creator;
       std::string newField;  // Add here
       // ... rest of fields
   };
   ```

2. **Update `TrainingPackManager.cpp` JSON parsing:**
   ```cpp
   // In LoadPacksFromFile()
   entry.newField = packJson.value("new_field", "default");
   ```

3. **Update `TrainingPackManager.cpp` JSON serialization:**
   ```cpp
   // In SavePacksToFile()
   packJson["new_field"] = pack.newField;
   ```

4. **Add UI display in `TrainingPackUI.cpp`:**
   ```cpp
   // In RenderDetailsPanel() or table column
   ImGui::Text("New Field: %s", pack.newField.c_str());
   ```

### Debugging in Visual Studio

1. **Set breakpoint** in `.cpp` file
2. **Attach to process:**
   - Debug → Attach to Process...
   - Find `RocketLeague.exe`
   - Attach
3. **Trigger breakpoint** by performing action in-game
4. **Inspect variables** in Autos/Locals window

**Pro tip:** Use conditional breakpoints on loops:
```cpp
Right-click breakpoint → Conditions → pack.code == "ABCD-1234-EFGH-5678"
```

---

## 📚 BakkesMod SDK Quick Reference

### Essential Wrappers

**GameWrapper** (`gameWrapper` global):
- `IsInCustomTraining()` - Returns true if in training mode
- `GetGameEventAsServer()` - Returns ServerWrapper for current match
- `SetTimeout(lambda, seconds)` - Schedule deferred execution
- `GetDataFolder()` - Returns `%APPDATA%\bakkesmod\bakkesmod\data`

**CVarManagerWrapper** (`cvarManager` global):
- `registerCvar(name, default, description, searchable, hasMin, min, hasMax, max, saveToCfg)` - Create setting
- `getCvar(name)` - Returns CVarWrapper (check for null!)
- `executeCommand(cmd)` - Run console command (e.g., `"load_training CODE"`)
- `registerNotifier(cmd, callback, description, permissions)` - Create console command

**CVarWrapper**:
- `getBoolValue()` / `getIntValue()` / `getFloatValue()` / `getStringValue()` - Read value
- `setValue(value)` - Write value
- `addOnValueChanged(callback)` - React to changes

**ServerWrapper**:
- `memory_address` - Raw pointer for wrapper constructors

**TrainingEditorWrapper**:
- `GetTrainingData()` - Returns TrainingEditorSaveDataWrapper
- `GetTotalRounds()` - Shot count (Method 1)

**TrainingEditorSaveDataWrapper**:
- `GetCode()` - Returns FStringWrapper with pack code
- `GetNumRounds()` - Shot count (Method 2, backup)

### Event Hook Signatures

```cpp
// Signature
gameWrapper->HookEventPost(
    "Function Class.EventName",  // Event string
    [this](std::string eventName) {  // Lambda callback
        // Your code here
    }
);

// Common events
"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"  // Match end
"Function TAGame.GameEvent_TrainingEditor_TA.OnInit"   // Pack loaded
"Function TAGame.TrainingEditorMetrics_TA.TrainingShotAttempt"  // Shot start
```

---

## 🛠️ Common Tasks Reference

### Task: Add a New UI Constant

**File:** `ConstantsUI.h`

```cpp
namespace UI::TrainingPackUI {
    constexpr float NEW_CONSTANT = 42.0f;
    inline const ImVec4 NEW_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // RGBA
}
```

**Note:** Use `inline const` for non-integral types like `ImVec4`.

### Task: Add a Method to TrainingPackUI

1. **Declare in `TrainingPackUI.h`:**
   ```cpp
   void NewMethod();  // Add to private: section
   ```

2. **Implement in `TrainingPackUI.cpp`:**
   ```cpp
   void TrainingPackUI::NewMethod() {
       // Implementation
   }
   ```

3. **Call from `Render()`:**
   ```cpp
   void TrainingPackUI::Render() {
       // ...
       NewMethod();
       // ...
   }
   ```

### Task: Filter the Training Pack List

**Existing pattern** (see `TrainingPackUI.cpp:267-317`):

```cpp
// Apply filters to RLTraining vector
filteredPacks.clear();
for (const auto& pack : plugin_->RLTraining) {
    bool matchesSearch = searchFilter.empty() ||
        (pack.name.find(searchFilter) != std::string::npos);
    bool matchesDifficulty = (selectedDifficultyFilter == 0) ||
        (pack.difficulty == difficultyLevels[selectedDifficultyFilter]);

    if (matchesSearch && matchesDifficulty) {
        filteredPacks.push_back(pack);
    }
}
```

---

## 📖 Code Examples

### Example 1: Loading a Training Pack

```cpp
void TrainingPackUI::LoadPackImmediately(const std::string& code) {
    if (code.empty()) return;

    // Execute load command
    std::string cmd = "load_training " + code;
    plugin_->cvarManager->executeCommand(cmd);

    // Track usage
    if (plugin_->usageTracker) {
        plugin_->usageTracker->IncrementLoadCount(code);
    }

    // Show success notification
    browserStatus.ShowSuccess("Loading pack...", 2.0f,
        UI::StatusMessage::DisplayMode::TimerWithFade);

    LOG("TrainingPackUI: Loading pack {}", code);
}
```

### Example 2: Two-Panel Layout (from WorkshopBrowserUI)

```cpp
// Calculate panel widths
float availWidth = ImGui::GetContentRegionAvail().x;
float leftWidth = availWidth * 0.60f;  // 60% for list
float rightWidth = availWidth - leftWidth - ImGui::GetStyle().ItemSpacing.x;

// Left panel
if (ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 400), true)) {
    // Render list/table
}
ImGui::EndChild();

ImGui::SameLine();

// Right panel
if (ImGui::BeginChild("RightPanel", ImVec2(rightWidth, 400), true)) {
    // Render details
}
ImGui::EndChild();
```

### Example 3: Difficulty Color Mapping

```cpp
ImVec4 GetDifficultyColor(const std::string& difficulty) {
    if (difficulty == "Bronze") return UI::TrainingPackUI::DIFFICULTY_BADGE_BRONZE_COLOR;
    if (difficulty == "Silver") return UI::TrainingPackUI::DIFFICULTY_BADGE_SILVER_COLOR;
    if (difficulty == "Gold") return UI::TrainingPackUI::DIFFICULTY_BADGE_GOLD_COLOR;
    if (difficulty == "Platinum") return UI::TrainingPackUI::DIFFICULTY_BADGE_PLATINUM_COLOR;
    if (difficulty == "Diamond") return UI::TrainingPackUI::DIFFICULTY_BADGE_DIAMOND_COLOR;
    if (difficulty == "Champion") return UI::TrainingPackUI::DIFFICULTY_BADGE_CHAMPION_COLOR;
    if (difficulty == "Grand Champion") return UI::TrainingPackUI::DIFFICULTY_BADGE_GRAND_CHAMPION_COLOR;
    if (difficulty == "Supersonic Legend") return UI::TrainingPackUI::DIFFICULTY_BADGE_SUPERSONIC_LEGEND_COLOR;
    return UI::TrainingPackUI::DIFFICULTY_BADGE_UNRANKED_COLOR;  // Default
}
```

---

## 🔬 Testing Strategies

### Manual Testing Checklist

**Build verification:**
```bash
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
# Should complete with 0 errors
```

**In-game verification:**
1. Launch Rocket League
2. Press F6 → BakkesMod console → type `plugin list` → verify SuiteSpot loaded
3. Press F2 → Settings → SuiteSpot tab
4. Test feature-specific workflows

**Regression tests:**
- Filters still work (search, difficulty, tags, shots, video)
- Sorting works (click column headers)
- Load pack works (LOAD NOW button)
- Auto-load works (match ends → pack loads)
- Custom pack form works (add/delete)

### Log Analysis

**Enable verbose logging:**
```cpp
LOG("Component: Action - variable: {}", value);
```

**View logs:**
- Press F6 in-game → BakkesMod console
- Filter by "SuiteSpot:" prefix

**Common log locations:**
- `SuiteSpot.cpp:221` - "AutoLoadFeature::OnMatchEnded triggered"
- `TrainingPackManager.cpp` - Pack CRUD operations
- `WorkshopDownloader.cpp` - Download progress

---

## 🎓 Learning Resources

### Project Documentation

- **`docs/architecture.md`** - High-level architecture, data flow diagrams
- **`docs/bakkesmod-sdk-reference.md`** - BakkesMod API reference
- **`docs/plans/training-browser-two-panel-layout.md`** - Current implementation plan

### External Resources

- **BakkesMod Plugin SDK:** https://wiki.bakkesplugins.com/
- **ImGui Manual:** `IMGUI/imgui.h` (inline documentation)
- **nlohmann/json docs:** https://json.nlohmann.me/

---

## 🚀 Quick Start Checklist

1. **Verify build environment:**
   ```bash
   msbuild /version  # Should show v17.x (VS 2022)
   ```

2. **Check dependencies:**
   - BakkesMod installed? Check `%APPDATA%\bakkesmod\`
   - vcpkg at `C:\Users\bmile\vcpkg`?
   - Rocket League installed?

3. **Build the plugin:**
   ```bash
   msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
   ```

4. **Read the plan:**
   ```bash
   cat docs/plans/training-browser-two-panel-layout.md
   ```

5. **Start implementation:**
   - Follow plan steps sequentially
   - Verify each change builds before moving to next step
   - Test in-game after completing all steps

---

## 💡 Tips for Success

1. **Always read before editing:**
   - Use Read tool to view current file contents
   - Preserve exact indentation (tabs vs spaces)
   - Match existing code style

2. **Incremental changes:**
   - Implement one plan step at a time
   - Build after each step to catch errors early
   - Test functionality before moving on

3. **Verify APIs before using:**
   - Check `IMGUI/imgui.h` for function signatures
   - Check `docs/bakkesmod-sdk-reference.md` for wrappers
   - Never assume parameter types or return values

4. **Preserve existing features:**
   - Check plan's "Preserved Features" table
   - Don't remove code unless plan explicitly says to
   - Maintain drag-drop, right-click menus, keyboard shortcuts

5. **Strong visual contrast:**
   - All text alpha >= 0.8 for screenshot readability
   - Use constants from `ConstantsUI.h` for colors
   - Test in-game with different backgrounds

6. **Log everything during development:**
   - Add `LOG()` statements for debugging
   - Remove or reduce verbosity before final commit

---

## 📋 Current Project State

### Git Status

**Modified files:**
- `WorkshopDownloader.cpp` - Workshop download implementation
- `version.h` - Auto-generated build version

**Untracked files:**
- `.clangd` - clangd language server config
- `.vscode/` - VS Code settings

### Active Branch

`main` - Current working branch

### Recent Work

1. Workshop map loader/downloader implementation (PR #13)
2. Race condition fixes in search/render (PR #14)
3. Pack healing feature (auto-corrects shot counts)
4. Bag rotation feature removed (earlier iteration)

### Next Task

**Implement two-panel Training Browser redesign** per plan at:
`docs/plans/training-browser-two-panel-layout.md`

---

## ❓ FAQ

**Q: Can I use C++23 features?**
A: No, the project uses C++20 (`/std:c++20` in .vcxproj line 80). Stick to C++20.

**Q: Can I add new dependencies?**
A: Yes, via vcpkg only. Update `.vcxproj` AdditionalIncludeDirectories.

**Q: Can I modify the build scripts?**
A: Only if explicitly requested. Build pipeline is stable and tested.

**Q: Should I create documentation files (.md)?**
A: Only if explicitly requested. Don't create README, CHANGELOG, etc. proactively.

**Q: Can I refactor code while implementing a feature?**
A: No. Only make changes directly requested or clearly necessary. Don't "improve" surrounding code.

**Q: Should I add comments/docstrings?**
A: Only where logic isn't self-evident. Don't add comments to code you didn't change.

**Q: How do I handle errors?**
A: Only validate at system boundaries (user input, external APIs). Trust internal code and framework guarantees.

**Q: Can I create abstractions/utilities?**
A: No premature abstractions. Three similar lines of code is better than a premature helper function.

---

## 🎉 You're Ready!

You now have everything you need to work on SuiteSpot. Key takeaways:

✅ Hub-and-Spoke architecture with SuiteSpot.cpp as the hub
✅ Always use `SetTimeout()` for post-match loading
✅ Thread safety with mutex for shared data
✅ CVar pattern: register → callback → local cache
✅ ImGui v1.75 APIs only (verify in `IMGUI/imgui.h`)
✅ Build with `msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64`
✅ Test in-game after every change

**Current task:** Implement two-panel Training Browser (see `docs/plans/training-browser-two-panel-layout.md`)

Happy coding! 🚀
