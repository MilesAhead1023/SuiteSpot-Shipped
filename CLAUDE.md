# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Reference Documentation

| Document | Purpose |
|----------|---------|
| [bakkesmod-sdk-reference.md](docs/bakkesmod-sdk-reference.md) | BakkesMod SDK API reference |
| [bakkesmod_imgui_signatures_annotated.md](docs/bakkesmod_imgui_signatures_annotated.md) | ImGui function signatures for BakkesMod |

## Critical Rules

- **Null-check wrappers**: Always check before use: `if (!car) return;`
- **Thread safety**: Use `gameWrapper->Execute()` for game operations from render thread
- **Delayed operations**: Use `gameWrapper->SetTimeout()` (never `sleep()` or `std::this_thread::sleep_for`)
- **CVar naming**: Prefix all CVars with `suitespot_`
- **Input blocking**: Override `ShouldBlockInput()` to only block for text input, not mouse interactions

## Project Overview

SuiteSpot is a BakkesMod plugin for Rocket League that automates game mode transitions after matches end. It can auto-load freeplay, training packs, or workshop maps, and optionally auto-queue for the next match.

**Key Technologies:**
- C++20, Windows x64 only
- Visual Studio 2022 (MSBuild)
- BakkesMod SDK for game integration
- ImGui for UI (DirectX 11)
- nlohmann/json for JSON handling

## Build Commands

```bash
# Build from command line (VS2022 Professional)
"/c/Program Files/Microsoft Visual Studio/2022/Professional/MSBuild/Current/Bin/MSBuild.exe" SuiteSpot.sln -p:Configuration=Release -p:Platform=x64 -v:minimal

# Or open SuiteSpot.sln in Visual Studio and build (F7)
```

**Build output:** `plugins\SuiteSpot.dll`

**Post-build:** The build automatically:
1. Creates data directories in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\`
2. Copies the training pack updater script
3. Runs `bakkesmod-patch.exe` to inject the DLL

**Pre-build:** `version.h` build number is auto-incremented via `CompiledScripts/update_version.ps1`

## Architecture

```
SuiteSpot (Plugin Hub)
├── MapManager          - Discovers workshop maps from filesystem
├── TrainingPackManager - Manages 2000+ training packs, bag rotation
├── SettingsSync        - CVar registration and settings cache
├── AutoLoadFeature     - Core logic: what to load when match ends
├── LoadoutManager      - Car preset switching (thread-safe)
└── UI Layer
    ├── SettingsUI      - F2 settings menu (tabs, controls)
    ├── TrainingPackUI  - Floating browser window with drag-drop
    ├── LoadoutUI       - Loadout dropdown selector
    ├── StatusMessageUI - Toast-style user feedback
    └── HelpersUI       - Shared UI utilities
```

**Event Flow:** Match ends → BakkesMod hook → `SuiteSpot::GameEndedEvent()` → `AutoLoadFeature::OnMatchEnded()` → schedules game command via `gameWrapper->SetTimeout()`

## Key Files

| File | Purpose |
|------|---------|
| `SuiteSpot.h/cpp` | Plugin entry point, lifecycle, window management |
| `AutoLoadFeature.cpp` | Core automation logic (single `OnMatchEnded()` entry point) |
| `TrainingPackManager.cpp` | Training pack loading, filtering, bag rotation |
| `SettingsSync.cpp` | All CVar registrations and settings cache |
| `MapList.h` | Data structures: `MapEntry`, `TrainingEntry`, `WorkshopEntry` |
| `ConstantsUI.h` | UI constants, colors, sizing, font scale |
| `HelpersUI.cpp/h` | Shared UI helper functions |
| `logging.h` | `LOG()` and `DEBUGLOG()` macros with source location |

## Data Persistence

Training packs stored at: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\TrainingSuite\training_packs.json`

CVars (settings) use BakkesMod's built-in persistence with prefix `suitespot_`.

## Thread Safety

See [THREAD_SAFETY.md](docs/standards/THREAD_SAFETY.md) for detailed patterns.

**Critical:** `RenderSettings()` runs on render thread. Use `gameWrapper->Execute()` for game operations:

```cpp
void RenderSettings() {
    if (ImGui::Button("Spawn Ball")) {
        gameWrapper->Execute([](GameWrapper* gw) {
            auto server = gw->GetCurrentGameState();
            if (server) server.SpawnBall({0,0,100}, true, false);
        });
    }
}
```

## ImGui Notes

**Version**: ImGui 1.75 (shipped with BakkesMod)

**Custom Widgets**: `IMGUI/` contains `imgui_rangeslider.h`, `imgui_searchablecombo.h`, `imgui_timeline.h`

**Key Patterns**:
- **Virtual Scrolling**: TrainingPackUI uses `ImGuiListClipper` for 2000+ packs
- **Window Independence**: Browser uses `ImGui::Begin()` not `BeginPopupModal()` for concurrent interaction
- **Input Blocking**: Override `ShouldBlockInput()` - only block for text input, not mouse interactions
- **Drag-and-Drop**: Payload-based transfer between windows sharing same ImGui context
- **Focus Management**: Use `SetNextWindowFocus()` + `ImGuiWindowFlags_NoBringToFrontOnFocus` conditionally

**Real-World Example**: See `TrainingPackUI.cpp` for drag-and-drop from browser to bag manager with proper input blocking and z-order management. See `CODING_STANDARDS.md` for detailed ImGui patterns.

## Logging

```cpp
LOG("Message with {} args", value);           // Always logged
DEBUGLOG("Debug only: {}", value);            // Only when DEBUG_LOG = true in logging.h
```

## Debugging

**BakkesMod Console Commands:**
```bash
plugin reload suitespot           # Hot-reload the plugin
load_training XXXX-XXXX-XXXX-XXXX # Load a training pack by code
load_freeplay                     # Load freeplay mode
togglemenu suitespot              # Toggle the browser window
```

**Enable debug logging** by setting `DEBUG_LOG = true` in `logging.h` and rebuilding.

## No Tests

This project has no automated tests. Validation is manual/runtime via BakkesMod console.
