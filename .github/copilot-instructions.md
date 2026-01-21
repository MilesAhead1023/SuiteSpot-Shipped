# GitHub Copilot Instructions for SuiteSpot

This file provides guidance to GitHub Copilot when working with the SuiteSpot repository.

## Project Overview

SuiteSpot is a BakkesMod plugin for Rocket League that automates game mode transitions after matches end. It can auto-load freeplay, training packs, or workshop maps, and optionally auto-queue for the next match.

**Target Platform:** Windows x64 only  
**Build Tool:** Visual Studio 2022 (MSBuild)  
**Language:** C++20

## Documentation References

Before making code changes, consult these documentation files:

- [BAKKESMOD_API_REFERENCE.md](../docs/standards/BAKKESMOD_API_REFERENCE.md) - Complete SDK API reference
- [CODING_STANDARDS.md](../docs/standards/CODING_STANDARDS.md) - Code patterns, naming conventions, anti-patterns
- [THREAD_SAFETY.md](../docs/standards/THREAD_SAFETY.md) - Threading model and synchronization patterns
- [CLAUDE.md](../CLAUDE.md) - Additional context for AI assistants

## Tech Stack

- **Language:** C++20
- **SDK:** BakkesMod SDK (game integration)
- **UI Framework:** ImGui 1.75 (DirectX 11)
- **JSON Library:** nlohmann/json
- **Compiler:** MSVC (Visual Studio 2022)

## Build Commands

```bash
# Build from command line (requires VS2022 Professional)
"/c/Program Files/Microsoft Visual Studio/2022/Professional/MSBuild/Current/Bin/MSBuild.exe" SuiteSpot.sln -p:Configuration=Release -p:Platform=x64 -v:minimal

# Or open SuiteSpot.sln in Visual Studio and build (F7)
```

**Build output:** `plugins\SuiteSpot.dll`

## Critical Coding Rules

### Thread Safety
- `RenderSettings()` and all UI render methods run on the render thread
- Use `gameWrapper->Execute()` for game operations from render thread
- Use `gameWrapper->SetTimeout()` for delayed operations (never use `sleep()`)

**Example:**
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

### Null Safety
- Always null-check wrappers before use
- Example: `if (!car) return;`

### Configuration Variables
- Prefix all CVars with `suitespot_`
- Register in `SettingsSync.cpp`

### Input Handling
- Override `ShouldBlockInput()` to only block for text input
- Do not block mouse interactions

### Logging
- Use `LOG()` for always-logged messages
- Use `DEBUGLOG()` for debug-only messages (controlled by `DEBUG_LOG` in `logging.h`)
- Both support format strings: `LOG("Message with {} args", value);`

## ImGui Patterns

**Version:** ImGui 1.75 (shipped with BakkesMod)

### Best Practices
- Use `ImGuiListClipper` for virtual scrolling with large datasets (see `TrainingPackUI.cpp`)
- Use `ImGui::Begin()` not `BeginPopupModal()` for independent browser windows
- Use `SetNextWindowFocus()` with `ImGuiWindowFlags_NoBringToFrontOnFocus` for z-order control
- Implement drag-and-drop using payload-based transfer between windows

### Custom Widgets
Located in `IMGUI/` directory:
- `imgui_rangeslider.h`
- `imgui_searchablecombo.h`
- `imgui_timeline.h`

## Code Structure

### Key Files
| File | Purpose |
|------|---------|
| `SuiteSpot.h/cpp` | Plugin entry point, lifecycle, window management |
| `AutoLoadFeature.cpp` | Core automation logic (single `OnMatchEnded()` entry point) |
| `TrainingPackManager.cpp` | Training pack loading, filtering, bag rotation |
| `SettingsSync.cpp` | All CVar registrations and settings cache |
| `MapList.h` | Data structures: `MapEntry`, `TrainingEntry`, `WorkshopEntry` |
| `ConstantsUI.h` | UI constants, colors, sizing, font scale |
| `HelpersUI.cpp/h` | Shared UI helper functions |
| `logging.h` | `LOG()` and `DEBUGLOG()` macros |

### Architecture
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

## Testing

**Note:** This project has no automated tests. Validation is manual/runtime.

### BakkesMod Console Commands
```bash
plugin reload suitespot           # Hot-reload the plugin
load_training XXXX-XXXX-XXXX-XXXX # Load a training pack by code
load_freeplay                     # Load freeplay mode
togglemenu suitespot              # Toggle the browser window
```

### Debug Logging
Enable by setting `DEBUG_LOG = true` in `logging.h` and rebuilding.

## Security & Restrictions

- Do not hardcode secrets or sensitive data
- Do not modify files in `.github/agents/` directory
- Do not commit `plugins/*.dll` files (build artifacts)
- Do not remove or modify working code unless fixing a bug or vulnerability
- Training pack data is stored at: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\TrainingSuite\training_packs.json`

## Coding Style

- Follow existing code patterns in the repository
- Match the style of surrounding code when making changes
- Do not add comments unless they match existing comment style or are necessary
- Prefer using existing libraries; only add new dependencies if absolutely necessary
- See [CODING_STANDARDS.md](../docs/standards/CODING_STANDARDS.md) for detailed conventions

## Common Anti-Patterns to Avoid

- Do not use `sleep()` or blocking operations in game thread
- Do not call game operations directly from render thread without `Execute()`
- Do not block mouse input in `ShouldBlockInput()` (only block for text input)
- Do not modify wrapper objects without null-checking first
- Do not use raw pointers when wrappers are available

## Pre-build Automation

- Build number in `version.h` is auto-incremented via `CompiledScripts/update_version.ps1`
- Do not manually edit version numbers

## Post-build Process

The build automatically:
1. Creates data directories in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\`
2. Copies the training pack updater script
3. Runs `bakkesmod-patch.exe` to inject the DLL
