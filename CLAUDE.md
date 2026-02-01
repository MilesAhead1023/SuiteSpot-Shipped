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

### File Structure
```
SuiteSpot/
├── SuiteSpot.cpp/h          (529 LOC) - Central hub and lifecycle
├── Core Components/
│   ├── AutoLoadFeature.cpp/h           - Match-end automation ⚠️ *
│   ├── SettingsSync.cpp/h              - CVar management
│   ├── MapManager.cpp/h      (367 LOC) - Workshop map discovery
│   ├── TrainingPackManager.cpp/h (652 LOC) - Pack database (2000+ packs)
│   ├── WorkshopDownloader.cpp/h (500 LOC) - RLMAPS API integration
│   ├── LoadoutManager.cpp/h  (294 LOC) - Car preset switching
│   └── PackUsageTracker.cpp/h          - Usage statistics
├── UI Components/
│   ├── SettingsUI.cpp/h     (1023 LOC) - F2 menu settings panel
│   ├── TrainingPackUI.cpp/h  (895 LOC) - Floating browser window
│   ├── LoadoutUI.cpp/h                 - Car loadout selector
│   ├── StatusMessageUI.cpp/h (190 LOC) - Toast notifications
│   └── HelpersUI.cpp/h       (239 LOC) - Shared UI utilities
├── Data Structures/
│   ├── MapList.cpp/h                   - Map/Pack entry definitions
│   ├── DefaultPacks.h        (460 LOC) - Embedded training pack list
│   ├── ConstantsUI.h         (390 LOC) - UI constants, difficulty levels
│   └── EmbeddedPackGrabber.h (460 LOC) - Pack data embedding
├── tests/
│   ├── integration/AutoLoadFeatureTests.cpp
│   ├── unit/WorkshopDownloaderTests.cpp
│   ├── threading/ConcurrencyTests.cpp
│   └── mocks/MockGameWrapper.h
└── Source.cpp - Legacy RenderSettings() for F2 menu UI

Total: 37 C++ files, 7,610 lines of code
```

⚠️ *AutoLoadFeature has direct dependencies on SettingsSync and PackUsageTracker, violating pure hub-and-spoke pattern. See "Architectural Notes" below.

### Entry Point

- **`SuiteSpot.cpp:153`** - `BAKKESMOD_PLUGIN(SuiteSpot, ...)` macro registers the plugin
- **`SuiteSpot.cpp:319-328`** - `onLoad()` creates all managers and UI components
- **`Source.cpp`** - Contains `RenderSettings()` for the F2 menu UI

### Core Components (Spokes)

| Component | File | Dependencies | Spoke Purity |
|-----------|------|--------------|--------------|
| `SuiteSpot` | SuiteSpot.cpp/h | *All managers* | N/A (Hub) |
| `AutoLoadFeature` | AutoLoadFeature.cpp/h | SettingsSync, PackUsageTracker, MapList | ⚠️ **Impure** |
| `SettingsSync` | SettingsSync.cpp/h | None | ✅ Pure |
| `MapManager` | MapManager.cpp/h | None | ✅ Pure |
| `TrainingPackManager` | TrainingPackManager.cpp/h | EmbeddedPackGrabber (data) | ✅ Pure |
| `WorkshopDownloader` | WorkshopDownloader.cpp/h | None | ✅ Pure |
| `LoadoutManager` | LoadoutManager.cpp/h | None | ✅ Pure |
| `PackUsageTracker` | PackUsageTracker.cpp/h | None | ✅ Pure |

### Architectural Notes

**Hub-and-Spoke Implementation:**
- ✅ **Hub (SuiteSpot.cpp)**: Creates and owns all manager instances (lines 319-328)
- ✅ **Pure Spokes**: MapManager, SettingsSync, WorkshopDownloader, TrainingPackManager have no cross-dependencies
- ⚠️ **Impure Spoke**: AutoLoadFeature directly includes SettingsSync.h and PackUsageTracker.h

**Architectural Violation:**
```cpp
// AutoLoadFeature.cpp - Direct dependencies (should go through hub)
#include "SettingsSync.h"       // ❌ Spoke-to-spoke dependency
#include "PackUsageTracker.h"   // ❌ Spoke-to-spoke dependency
```

This violates pure hub-and-spoke where spokes should only communicate through the hub. However, this is a pragmatic design choice - AutoLoadFeature acts as a "coordinator" that needs real-time access to settings and tracker state.

**Memory Management:**
Mixed raw and smart pointers in SuiteSpot.h:137-143:
```cpp
// Raw pointers (manual delete in onUnload)
MapManager* mapManager = nullptr;
SettingsSync* settingsSync = nullptr;
AutoLoadFeature* autoLoadFeature = nullptr;

// Smart pointers (automatic cleanup)
std::unique_ptr<LoadoutManager> loadoutManager;
std::unique_ptr<WorkshopDownloader> workshopDownloader;
std::shared_ptr<TrainingPackUI> trainingPackUI;
```

**Recommendation**: Standardize on `std::unique_ptr` for all owned managers to prevent memory leaks and clarify ownership.

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
- Uses `std::mutex` for protecting concurrent access to training pack lists (see TrainingPackManager.cpp)
- UI rendering and background data updates run on different threads
- `std::atomic<bool> isRenderingSettings` (SuiteSpot.h:147) prevents concurrent render issues during F2 menu rendering
- **Critical**: Workshop downloader runs asynchronously - ensure proper synchronization when accessing download state

### Deferred Execution
Uses `gameWrapper->SetTimeout()` to schedule commands after match-end. **Immediate loading during match-end sequence crashes the game** due to game state transitions. Always defer training pack/map loads by at least the configured delay (see SettingsSync for delay CVars).

### Data Persistence
All data stored in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\TrainingSuite\`:
- `training_packs.json` - Pack database (2000+ packs)
- `pack_usage_stats.json` - Usage tracking and favorites
- `workshop_loader_config.json` - Workshop map directory configuration

### Include Dependencies (as of analysis)
Most included header across the codebase:
1. `MapList.h` (9 includes) - Core data structures
2. `bakkesmod/plugin/bakkesmodplugin.h` (9 includes) - Plugin base
3. `logging.h` (7 includes) - Logging utilities
4. `SuiteSpot.h` (6 includes) - Hub interface
5. `SettingsSync.h` (6 includes) - Configuration access

## Domain-Driven Design (DDD) Mapping

For architectural tracking and AI monitoring (Claude Flow V3), components map to 5 conceptual domains:

| DDD Domain | Status | Mapped Components | Purpose |
|------------|--------|-------------------|---------|
| **Task Management** | ✅ Active | AutoLoadFeature.cpp | Executes core automation task (match-end loading) |
| **Session Management** | ✅ Active | SettingsSync.cpp, PackUsageTracker.cpp | Persists configuration and usage state |
| **Health Monitoring** | ✅ Active | `std::mutex` locks, error handling, atomic flags | Ensures stability and thread safety |
| **Lifecycle Management** | ✅ Active | SuiteSpot.cpp:319 (`onLoad`), hub pattern | Controls plugin initialization and coordination |
| **Event Coordination** | ✅ Active | MapManager, TrainingPackManager, WorkshopDownloader | Coordinates external data sources (maps, packs, API) |

**Note**: These are conceptual categories for understanding architecture, not a requirement to reorganize files into 5 folders. The current flat component-based structure is optimal for this codebase size.

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

## Testing

Test suite located in `tests/` directory:

| Test Type | File | Purpose |
|-----------|------|---------|
| **Integration** | `tests/integration/AutoLoadFeatureTests.cpp` | End-to-end automation workflow tests |
| **Unit** | `tests/unit/WorkshopDownloaderTests.cpp` | API download logic unit tests |
| **Threading** | `tests/threading/ConcurrencyTests.cpp` | Thread safety and race condition tests |
| **Mocks** | `tests/mocks/MockGameWrapper.h` | Mock BakkesMod interfaces for testing |

**Running Tests:**
```bash
# Build tests (if configured)
msbuild SuiteSpot.sln /t:Tests /p:Configuration=Debug /p:Platform=x64

# Or run individual test files through Visual Studio Test Explorer
```

## Common Development Tasks

### Adding a New Training Pack
1. Update `DefaultPacks.h` with new pack metadata
2. Ensure pack code is valid (11-character Steam Workshop code format)
3. TrainingPackManager will auto-load on next plugin initialization

### Adding a New Manager Component
1. Create `YourManager.cpp/h` in root directory
2. Add forward declaration in `SuiteSpot.h` (around line 56)
3. Add member pointer in `SuiteSpot.h` private section (around line 137)
4. Initialize in `SuiteSpot::onLoad()` (around line 319)
5. Clean up in `SuiteSpot::onUnload()` if using raw pointers
6. **Recommendation**: Use `std::unique_ptr<YourManager>` to avoid manual cleanup

### Debugging Match-End Events
Set breakpoints in:
- `SuiteSpot::GameEndedEvent()` - Match end detection
- `AutoLoadFeature::Execute()` - Automation trigger
- Check `suitespot_enabled` CVar is true via BakkesMod console

## Known Issues & Gotchas

1. **Memory Management Inconsistency**: Mixed raw/smart pointers - see Architecture section
2. **AutoLoadFeature Cross-Dependency**: Breaks pure hub-and-spoke pattern but functionally necessary
3. **Immediate Loading Crashes**: Always use `gameWrapper->SetTimeout()` for post-match loads
4. **Thread Safety**: UI code must check `isRenderingSettings` atomic flag before rendering
5. **Workshop Map Discovery**: Requires user to configure workshop directory path via settings UI
