# SuiteSpot - Claude Code Context

This file is auto-loaded at the start of every Claude Code session.

## Project Overview

**SuiteSpot** is a BakkesMod plugin for Rocket League that provides:
- Automated map loading after matches (Freeplay, Training, Workshop)
- Auto-queue functionality with configurable delays
- Training pack management with Prejump integration
- Loadout management
- Overlay rendering system

**Tech Stack:** C++17, BakkesMod SDK, ImGui, Visual Studio 2022

## Build Command

```bash
powershell -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' SuiteSpot.vcxproj /p:Configuration=Release /p:Platform=x64"
```

**Output:** `plugins/SuiteSpot.dll`

## Project Structure

```
SuiteSpotv2.0/
├── SuiteSpot.h/.cpp          # Main plugin class (lifecycle, hooks, CVars)
├── Source.cpp                # Settings entry point (delegates to SettingsUI)
├── SettingsUI.h/.cpp         # Main ImGui settings window
├── SettingsSync.h/.cpp       # CVar registration and state sync
├── AutoLoadFeature.h/.cpp    # Match-end auto-load logic
├── MapList.h/.cpp            # Map data vectors (RLMaps, RLTraining, RLWorkshop)
├── MapManager.h/.cpp         # Map persistence, shuffle, workshop discovery
├── PrejumpUI.h/.cpp          # Prejump pack tab UI
├── PrejumpPackManager.h/.cpp # Prejump pack loading/filtering
├── LoadoutUI.h/.cpp          # Loadout management tab
├── LoadoutManager.h/.cpp     # Car loadout logic
├── OverlayRenderer.h/.cpp    # Post-match overlay system
├── ThemeManager.h/.cpp       # Theme loading for overlay
├── GuiBase.h/.cpp            # Base ImGui window class
├── pch.h/.cpp                # Precompiled headers (MUST be first include)
├── logging.h                 # LOG() macro
├── IMGUI/                    # ImGui library
├── plugins/                  # Build output
├── themes/                   # Overlay themes
└── SuiteSpotDocuments/       # API reference docs
```

## Critical Constraints

### 1. Thread Safety (MANDATORY)

All game state access MUST go through `SetTimeout()` or event handlers:

```cpp
// WRONG - Direct access
auto car = gameWrapper->GetLocalCar();

// CORRECT - Via SetTimeout
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    if (gw->IsInGame()) {
        auto car = gw->GetLocalCar();
    }
}, 0.0f);
```

### 2. Wrapper Lifetime

Never store wrapper objects across frames - always fetch fresh:

```cpp
// WRONG
ServerWrapper server = gameWrapper->GetGameEvent();
// ... later use server (stale!)

// CORRECT
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    ServerWrapper server = gw->GetGameEvent();
    // Use immediately
}, 0.0f);
```

### 3. Null Pointer Checks

Always validate before dereferencing:

```cpp
// WRONG
auto tm = plugin_->GetThemeManager();
tm->LoadTheme(...);  // Crash if null

// CORRECT
if (auto tm = plugin_->GetThemeManager()) {
    tm->LoadTheme(...);
}
```

### 4. CVar Access Safety

Use triple-layer null checks:

```cpp
// WRONG
cvarManager->getCvar("name").setValue(true);

// CORRECT
if (cvarManager) {
    auto cvar = cvarManager->getCvar("name");
    if (cvar) {
        cvar.setValue(true);
    }
}
```

### 5. File I/O Error Handling

Always check file operations:

```cpp
std::ofstream out(filepath);
if (!out.is_open()) {
    LOG("Failed to open: {}", filepath);
    return;
}
out << data;
if (out.fail()) {
    LOG("Write error: {}", filepath);
}
```

### 6. Precompiled Header

Every `.cpp` file MUST include `pch.h` as the FIRST line:

```cpp
#include "pch.h"
// ... rest of includes
```

## Key CVars

| CVar | Type | Description |
|------|------|-------------|
| `suitespot_enabled` | bool | Enable plugin |
| `suitespot_map_type` | int | 0=Freeplay, 1=Training, 2=Workshop |
| `suitespot_auto_queue` | bool | Auto-queue after load |
| `suitespot_delay_queue_sec` | float | Queue delay (0-300) |
| `suitespot_training_shuffle` | bool | Shuffle training packs |

## Common Event Hooks

```cpp
// Match ended
"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"

// Training pack loaded
"Function TAGame.GameEvent_TrainingEditor_TA.OnInit"

// Freeplay start
"Function TAGame.GameEvent_Freeplay_TA.OnInit"
```

## File Locations

| Purpose | Path |
|---------|------|
| Training maps | `%APPDATA%/bakkesmod/bakkesmod/data/SuiteTraining/SuiteSpotTrainingMaps.txt` |
| Shuffle bag | `%APPDATA%/bakkesmod/bakkesmod/data/SuiteTraining/SuiteShuffleBag.txt` |
| Prejump cache | `%APPDATA%/bakkesmod/bakkesmod/data/SuiteTraining/prejump_packs.json` |
| Plugin DLL | `%APPDATA%/bakkesmod/bakkesmod/plugins/SuiteSpot.dll` |

## Common Tasks

### Add a new CVar

```cpp
// In SettingsSync.cpp or SuiteSpot.cpp onLoad()
cvarManager->registerCvar("suitespot_new_var", "0", "Description", true, true, 0, true, 100)
    .addOnValueChanged([this](std::string, CVarWrapper cvar) {
        myVar = cvar.getBoolValue();
    });
```

### Add an event hook

```cpp
// In SuiteSpot.cpp onLoad()
gameWrapper->HookEvent("Function TAGame.Something.Event",
    std::bind(&SuiteSpot::OnEvent, this, std::placeholders::_1));
```

### Execute delayed command

```cpp
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    cvarManager->executeCommand("load_freeplay Stadium_P");
}, delaySeconds);
```

## Documentation Reference

For detailed API documentation, see `SuiteSpotDocuments/`:
- `bakkesmod.instructions.md` - SDK overview
- `thread-safety.instructions.md` - Threading patterns
- `gamewrapper-api.md` - GameWrapper methods
- `canvaswrapper-api.md` - Overlay rendering
- `imgui-guibase-api.md` - ImGui patterns

## Git Workflow

- Main branch: `main`
- Remote: `shipped` (https://github.com/MilesAhead1023/SuiteSpot-Shipped.git)
- Push: `git push shipped main`
