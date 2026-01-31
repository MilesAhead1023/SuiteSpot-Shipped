# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SuiteSpot is a BakkesMod plugin for Rocket League that automates loading training packs, freeplay maps, and workshop maps after matches. It provides seamless training-focused gameplay by automatically transitioning players to their chosen practice environment when a match ends.

## Build Commands

**Build the plugin (Visual Studio):**
```bash
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

**Or open `SuiteSpot.sln` in Visual Studio 2022 and build with F7 (Release x64).**

The build process:
1. Pre-build: Runs `CompiledScripts/update_version.ps1` to auto-increment the build number in `version.h`
2. Compile: C++20 with MSVC v143
3. Post-build: Runs `bakkesmod-patch.exe` to inject the DLL and copies resources to `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\`

Output: `plugins/SuiteSpot.dll`

## Architecture

The project follows a **Hub-and-Spoke pattern** where `SuiteSpot.cpp` is the central hub managing the plugin lifecycle.

### Entry Point

- **`SuiteSpot.cpp:153`** - `BAKKESMOD_PLUGIN(SuiteSpot, ...)` macro registers the plugin
- **`SuiteSpot.cpp:319`** - `onLoad()` is the actual initialization entry point
- **`Source.cpp`** - Contains `RenderSettings()` for the F2 menu UI

### Core Components (Spokes)

| Component | File | Responsibility |
|-----------|------|----------------|
| `SuiteSpot` | SuiteSpot.cpp/h | Central hub, lifecycle, event coordination |
| `AutoLoadFeature` | AutoLoadFeature.cpp/h | Match-end automation logic |
| `SettingsSync` | SettingsSync.cpp/h | CVar management, config persistence |
| `MapManager` | MapManager.cpp/h | Workshop map discovery on disk |
| `TrainingPackManager` | TrainingPackManager.cpp/h | Training pack database (2000+ packs) |
| `WorkshopDownloader` | WorkshopDownloader.cpp/h | Downloads maps from RLMAPS API |
| `LoadoutManager` | LoadoutManager.cpp/h | Car preset switching |
| `PackUsageTracker` | PackUsageTracker.cpp/h | Usage statistics for favorites |

### UI Components

| Component | File | Description |
|-----------|------|-------------|
| `SettingsUI` | SettingsUI.cpp/h | F2 menu settings panel |
| `TrainingPackUI` | TrainingPackUI.cpp/h | Floating browser window |
| `LoadoutUI` | LoadoutUI.cpp/h | Car loadout selector |
| `StatusMessageUI` | StatusMessageUI.cpp/h | Toast notifications |
| `HelpersUI` | HelpersUI.cpp/h | Shared UI utilities |

### Data Structures

- **`MapList.h`** - Defines `MapEntry`, `TrainingEntry`, `WorkshopEntry` structs
- **`DefaultPacks.h`** - Embedded training pack list
- **`ConstantsUI.h`** - UI constants, difficulty levels

## Key Implementation Patterns

### CVar Naming Convention
All settings use the `suitespot_` prefix (e.g., `suitespot_enabled`, `suitespot_delay_queue`).

### Thread Safety
- Uses `std::mutex` for protecting concurrent access to training pack lists
- UI rendering and background data updates run on different threads
- `std::atomic<bool> isRenderingSettings` prevents concurrent render issues

### Deferred Execution
Uses `gameWrapper->SetTimeout()` to schedule commands after match-end. Immediate loading during match-end sequence crashes the game.

### Data Persistence
All data stored in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\TrainingSuite\`:
- `training_packs.json` - Pack database
- `pack_usage_stats.json` - Usage tracking

## Dependencies

- **BakkesMod SDK** - Game hooks, CVars, ImGui integration (via `BakkesMod.props`)
- **ImGui** - Immediate-mode GUI (bundled in `IMGUI/` folder)
- **nlohmann/json** - JSON parsing (via vcpkg at `C:\Users\bmile\vcpkg`)
- **PowerShell** - Used for zip extraction and version updates

## Build Requirements

- Visual Studio 2022 with MSVC v143
- Windows SDK 10.0
- BakkesMod installed (SDK path read from registry)
- vcpkg with nlohmann-json installed
