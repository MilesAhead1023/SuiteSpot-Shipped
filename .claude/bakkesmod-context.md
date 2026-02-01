# BakkesMod Development Context

This file provides BakkesMod-specific guidance for Claude when working on the SuiteSpot plugin.

## Quick Reference

### Critical "Don't Do" Rules

1. **NEVER** load maps/packs immediately after match-end → Always use `SetTimeout()` (min 0.5s delay)
2. **NEVER** use `git add -A` → Stage specific files only
3. **NEVER** access CVars without null checking → Use `if (cvar) { ... }`
4. **NEVER** use PowerShell `-i` flag → Use `-NonInteractive`
5. **NEVER** hallucinate ImGui API → Check `IMGUI/imgui.h` for v1.75 signatures
6. **NEVER** block render thread → No long operations in UI code
7. **NEVER** use CVar names without `suitespot_` prefix
8. **NEVER** modify shared data without mutex protection
9. **NEVER** store wrappers as class members → Pointers become invalid
10. **NEVER** call game code from UI without Execute → Will crash
11. **NEVER** hook same function multiple times → Only first callback runs

### Always Do Rules

1. **ALWAYS** use `std::lock_guard` for shared data (training packs, workshop maps)
2. **ALWAYS** validate ImGui API against bundled v1.75 (`IMGUI/imgui.h`)
3. **ALWAYS** log with `LOG()` macro, never `std::cout`
4. **ALWAYS** include `pch.h` first in `.cpp` files
5. **ALWAYS** use `#pragma once` in headers
6. **ALWAYS** run hooks: pre-edit → edit → post-edit → pre-build → build → post-build
7. **ALWAYS** defer execution with `SetTimeout()` for post-match operations
8. **ALWAYS** sync CVar changes to local variables via callbacks
9. **ALWAYS** null check wrappers before using
10. **ALWAYS** obtain wrappers fresh when needed (don't store)
11. **ALWAYS** wrap UI-to-game calls in `gameWrapper->Execute()`

## BakkesMod Environment

### Platform Requirements

- **OS**: Windows 10/11 x64 only (Rocket League requirement)
- **Compiler**: MSVC v143 (Visual Studio 2022)
- **Language**: C++20 (`/std:c++20`)
- **Runtime**: MultiThreaded (Release) / MultiThreadedDebug (Debug)
- **Output**: DLL (Dynamic Link Library)

### Critical Dependencies

- **BakkesMod SDK**: Game hooks, wrappers (via registry at `HKEY_CURRENT_USER\Software\BakkesMod`)
- **ImGui v1.75**: Bundled in `IMGUI/` folder (DirectX 11 backend)
- **nlohmann/json**: Via vcpkg at `C:\Users\bmile\vcpkg\installed\x64-windows\include`
- **PowerShell 5.1+**: Script execution for build automation

### Build Process

```bash
# Pre-build: Auto-increment version
powershell.exe -ExecutionPolicy Bypass -NoProfile -NonInteractive -File CompiledScripts/update_version.ps1 "./version.h"

# Build
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64

# Post-build: Patch DLL and deploy
bakkesmod-patch.exe "$(TargetPath)"
xcopy /Y /I Resources\* %APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\Resources\
```

## Code Patterns

### Thread Safety Pattern (CRITICAL)

```cpp
class Manager {
private:
    std::mutex dataMutex_;           // Protect shared data
    std::vector<Entry> sharedData_;  // Accessed from multiple threads
    
public:
    void ModifyData(Entry entry) {
        std::lock_guard<std::mutex> lock(dataMutex_);
        sharedData_.push_back(entry);  // Safe with lock
    }
    
    Entry GetData(size_t idx) {
        std::lock_guard<std::mutex> lock(dataMutex_);
        return sharedData_[idx];       // Safe with lock
    }
};
```

**Where this applies**:

- `TrainingPackManager`: `packMutex_` protects `RLTraining` vector
- `MapManager`: Mutex protects `RLWorkshop` vector
- Any data accessed by both UI thread and background threads

### Deferred Execution Pattern (CRITICAL)

```cpp
void OnMatchEnded() {
    // WRONG: Immediate loading crashes the game
    // cvarManager->executeCommand("load_freeplay Map"); // CRASH!
    
    // CORRECT: Deferred execution
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        cvarManager->executeCommand("load_freeplay Map");
    }, 0.5f); // Minimum 0.5s delay
}
```

**Why**: Game crashes if you load during match-end sequence. Always defer.

### CVar Pattern (CRITICAL)

```cpp
// In SettingsSync.h
class SettingsSync {
private:
    bool isEnabled_ = false;  // Local cache for fast access
    
public:
    void RegisterAllCVars(std::shared_ptr<CVarManagerWrapper> cvarManager) {
        // 1. Register CVar
        auto cvar = cvarManager->registerCvar("suitespot_enabled", "0", "Enable SuiteSpot");
        
        // 2. Sync initial value
        if (cvar) {
            isEnabled_ = cvar.getBoolValue();
            
            // 3. Add callback for future changes
            cvar.addOnValueChanged([this](std::string oldValue, CVarWrapper c) {
                isEnabled_ = c.getBoolValue(); // Keep local cache synced
            });
        }
    }
    
    // 4. Fast getter (uses local cache, not CVar lookup)
    bool IsEnabled() const { return isEnabled_; }
};

// Usage in other files
if (settingsSync->IsEnabled()) {  // Fast: uses local variable
    // Do something
}
```

**Why**: CVar lookups are slow. Local cache is fast. Callbacks keep cache synced.

### Null Safety Pattern (CRITICAL)

```cpp
// WRONG: Direct access
bool value = cvarManager->getCvar("suitespot_enabled").getBoolValue(); // CRASH if doesn't exist!

// CORRECT: Null check
auto cvar = cvarManager->getCvar("suitespot_enabled");
if (cvar) {
    bool value = cvar.getBoolValue();
}

// BETTER: Use helper
UI::Helpers::SetCVarSafely(cvarManager, "suitespot_enabled", true);
```

### Wrapper Storage Anti-Pattern (CRITICAL)

```cpp
// WRONG: Storing wrappers as class members
class Plugin {
    ServerWrapper storedServer;  // DANGER! Pointer becomes invalid
    CarWrapper storedCar;         // DANGER! Will crash later
};

// CORRECT: Obtain fresh when needed
class Plugin {
    void ProcessMatch() {
        ServerWrapper server = gameWrapper->GetCurrentGameState();
        if (!server) { return; }
        // Use immediately, don't store
    }
};
```

**Why**: Wrappers are pointers that become invalid between frames/matches. Stored wrappers will crash.

### Execute Pattern (UI Safety - CRITICAL)

```cpp
// WRONG: Direct call from UI
if (ImGui::Button("Load Pack")) {
    cvarManager->executeCommand("load_training CODE");  // CRASH!
}

// CORRECT: Wrap in Execute
if (ImGui::Button("Load Pack")) {
    gameWrapper->Execute([this](GameWrapper* gw) {
        cvarManager->executeCommand("load_training CODE");  // Safe
    });
}
```

**Why**: UI runs on render thread. Direct game modifications crash. Execute schedules operation safely.

### Hook Registration Pattern (CRITICAL)

```cpp
// WRONG: Hooking same function multiple times
gameWrapper->HookEvent(eventName, callback1);  // Runs
gameWrapper->HookEvent(eventName, callback2);  // IGNORED!

// CORRECT: Single hook with combined logic
gameWrapper->HookEvent(eventName, [this](std::string e) {
    DoAction1();
    DoAction2();
});
```

**Why**: Only first hook callback executes. Subsequent hooks silently ignored.

### ImGui Virtual Scrolling Pattern

```cpp
// For large lists (2000+ items), use virtual scrolling
ImGuiListClipper clipper;
clipper.Begin((int)items.size());  // Total items
while (clipper.Step()) {
    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
        // Only render visible rows
        ImGui::Selectable(items[row].name.c_str(), ...);
    }
}
// Result: 60 FPS even with 2000+ items (only ~10-15 rows rendered)
```

## ImGui v1.75 API Reference

**ALWAYS** verify against `IMGUI/imgui.h` before using. Common v1.75 signatures:

```cpp
// Layout
bool BeginChild(const char* str_id, const ImVec2& size = ImVec2(0,0), bool border = false, ImGuiWindowFlags flags = 0);
void EndChild();
void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);
ImVec2 GetContentRegionAvail();

// Widgets
bool Button(const char* label, const ImVec2& size = ImVec2(0,0));
bool Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0,0));
void Text(const char* fmt, ...);
void TextColored(const ImVec4& col, const char* fmt, ...);
void TextWrapped(const char* fmt, ...);

// Input
bool Checkbox(const char* label, bool* v);
bool SliderInt(const char* label, int* v, int v_min, int v_max);
bool InputText(const char* label, char* buf, size_t buf_size);

// Tables (v1.75 has basic column support)
void Columns(int count, const char* id = NULL, bool border = true);
void NextColumn();
void SetColumnWidth(int column_index, float width);

// Clipboard
void SetClipboardText(const char* text);

// State
bool IsItemHovered();
bool IsMouseClicked(int button);
```

**NOT in v1.75**: BeginTable/EndTable (added in v1.80+)

## Project Structure

```
SuiteSpot/
├── Core/               # Backend logic (not actual directory, conceptual)
│   ├── SuiteSpot.cpp/h           # Hub: plugin lifecycle
│   ├── AutoLoadFeature.cpp/h     # Match-end automation
│   ├── SettingsSync.cpp/h        # CVar management
│   ├── TrainingPackManager.cpp/h # Pack database
│   ├── MapManager.cpp/h          # Workshop maps
│   ├── WorkshopDownloader.cpp/h  # Map downloads
│   ├── LoadoutManager.cpp/h      # Car presets
│   └── PackUsageTracker.cpp/h    # Usage stats
│
├── UI/                 # Frontend rendering (conceptual)
│   ├── SettingsUI.cpp/h         # F2 menu panel
│   ├── TrainingPackUI.cpp/h     # Floating browser
│   ├── LoadoutUI.cpp/h          # Loadout picker
│   ├── StatusMessageUI.cpp/h    # Toast notifications
│   └── HelpersUI.cpp/h          # Shared utilities
│
├── Data/               # Structures (conceptual)
│   ├── MapList.h       # MapEntry, TrainingEntry, WorkshopEntry
│   ├── DefaultPacks.h  # Embedded pack list
│   └── ConstantsUI.h   # UI constants
│
├── .claude/
│   ├── bakkesmod-hooks/  # BakkesMod-specific hooks
│   ├── commands/
│   ├── helpers/
│   └── skills/
│
├── .claude-flow/
│   └── config.yaml      # Hook configuration
│
├── docs/               # Documentation
│   ├── architecture.md
│   └── bakkesmod-sdk-reference.md
│
├── IMGUI/              # ImGui v1.75 bundled
├── CompiledScripts/    # PowerShell automation
├── Resources/          # Assets, scripts
└── [source files]      # Root-level .cpp/.h files
```

## Common Tasks

### Adding a New CVar

1. Declare in `SettingsSync.h`:

   ```cpp
   bool newSetting_ = false;
   ```

2. Register in `SettingsSync.cpp::RegisterAllCVars()`:

   ```cpp
   auto cvar = cvarManager->registerCvar("suitespot_new_setting", "0", "Description");
   if (cvar) {
       newSetting_ = cvar.getBoolValue();
       cvar.addOnValueChanged([this](std::string old, CVarWrapper c) {
           newSetting_ = c.getBoolValue();
       });
   }
   ```

3. Add getter in `SettingsSync.h`:

   ```cpp
   bool GetNewSetting() const { return newSetting_; }
   ```

4. Add UI in `SettingsUI.cpp`:

   ```cpp
   if (ImGui::Checkbox("Enable Feature", &plugin_->settingsSync->newSetting_)) {
       UI::Helpers::SetCVarSafely(plugin_->cvarManager, "suitespot_new_setting", 
           plugin_->settingsSync->newSetting_);
   }
   ```

### Adding a Training Pack Field

1. Update `MapList.h` struct
2. Update JSON parsing in `TrainingPackManager.cpp::LoadPacksFromFile()`
3. Update JSON serialization in `SavePacksToFile()`
4. Update UI display in `TrainingPackUI.cpp`

### Adding ImGui UI

1. Verify API in `IMGUI/imgui.h` (v1.75)
2. Use constants from `ConstantsUI.h`
3. Use virtual scrolling (`ImGuiListClipper`) for large lists
4. No blocking operations in render code
5. Use `ImGui::GetIO().DeltaTime` for animations

## Debugging

### Visual Studio Debugging

1. Build in Debug configuration
2. Debug → Attach to Process → RocketLeague.exe
3. Set breakpoints in .cpp files
4. Trigger action in-game

### Logging

```cpp
#include "logging.h"

LOG("Simple message");
LOG("Formatted: {}, {}", variable1, variable2);
```

Output: BakkesMod console (F6 in-game)

### Common Issues

**Issue**: Game crashes on match-end
**Cause**: Immediate loading without SetTimeout
**Fix**: Wrap in `gameWrapper->SetTimeout(..., 0.5f)`

**Issue**: Settings changes don't apply
**Cause**: No CVar callback or missing local sync
**Fix**: Add `addOnValueChanged` callback

**Issue**: FPS drops in browser
**Cause**: Rendering all 2000+ packs
**Fix**: Use `ImGuiListClipper` for virtual scrolling

**Issue**: Build error about ImGui API
**Cause**: Using API not in v1.75
**Fix**: Check `IMGUI/imgui.h`, use compatible v1.75 API

## Hook Integration

Hooks are located in `.claude/bakkesmod-hooks/`:

- **pre-edit**: Validate C++ syntax, ImGui APIs, thread safety, CVar naming
- **post-edit**: Format code, validate includes, thread safety audit
- **pre-build**: Check SDK, Visual Studio, vcpkg, project configuration
- **post-build**: Verify DLL, patch, resource deployment
- **project-init**: Complete environment validation
- **cvar-validation**: CVar naming, registration, callbacks, null safety

**Workflow**:

```
pre-edit → Edit File → post-edit → pre-build → Build → post-build
```

## Performance Targets

- **Build time**: < 30 seconds (Release x64)
- **DLL size**: ~200-300 KB
- **Render FPS**: 60+ FPS (even with 2000+ pack browser open)
- **Load time**: < 1 second for plugin initialization
- **Memory**: < 50 MB overhead

## Security Considerations

- No secrets in source code
- PowerShell scripts use `-ExecutionPolicy Bypass` (local only)
- File paths validated before use
- No arbitrary code execution from JSON
- Workshop downloads sanitized (no executable files)

## See Also

- [CLAUDE.md](../CLAUDE.md) - Quick reference for Claude Code
- [GEMINI.md](../GEMINI.md) - Comprehensive technical reference
- [docs/architecture.md](../docs/architecture.md) - Architecture deep dive
- [docs/bakkesmod-sdk-reference.md](../docs/bakkesmod-sdk-reference.md) - BakkesMod API
- `.claude/bakkesmod-hooks/` - Hook documentation
