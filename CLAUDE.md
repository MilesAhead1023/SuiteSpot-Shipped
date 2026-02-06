# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SuiteSpot is a BakkesMod plugin for Rocket League that automatically loads training content (training packs, freeplay maps, or workshop maps) after matches end. It's a C++20 Windows x64 DLL that integrates with BakkesMod's SDK.

## Build Commands

**Build the plugin:**
```bash
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

**CI build (clones BakkesMod SDK):**
```bash
git clone --depth 1 https://github.com/bakkesmodorg/BakkesModSDK.git bakkesmodsdk
cd bakkesmodsdk && git checkout 479e8f571cf554b25f4eeb64d611dec4133edcaf && cd ..
msbuild /m /p:Configuration=Release /p:BakkesModPath="." SuiteSpot.sln
```

For local development, the BakkesMod SDK path is configured in `BakkesMod.props`. The post-build step copies the DLL and resources to the BakkesMod plugins folder and patches it with `bakkesmod-patch.exe`.

## Architecture

The project uses a **Hub-and-Spoke** pattern where `SuiteSpot.cpp` (the Hub class) orchestrates all functionality:

```
SuiteSpot.cpp (Hub)
├── AutoLoadFeature     - Post-match automation logic
├── SettingsSync        - CVar-backed settings management
├── MapManager          - Workshop map discovery and paths
├── TrainingPackManager - 2300+ pack database with search/filter
├── WorkshopDownloader  - RLMAPS API integration for downloads
├── LoadoutManager      - Car preset management
├── PackUsageTracker    - Usage statistics for favorites
└── UI Components
    ├── SettingsUI      - F2 menu (tabs: Map Select, Loadout, Workshop)
    ├── TrainingPackUI  - Floating pack browser window
    ├── LoadoutUI       - Car preset selection
    └── StatusMessageUI - Toast notifications
```

Components don't communicate directly—all coordination flows through the Hub.

## Key Source Files

| File | Purpose |
|------|---------|
| `SuiteSpot.cpp/.h` | Plugin entry point, lifecycle management, event routing |
| `AutoLoadFeature.cpp/.h` | Core match-end automation with delay scheduling |
| `SettingsSync.cpp/.h` | CVars (prefixed `suitespot_*`) with change callbacks |
| `TrainingPackManager.cpp/.h` | JSON database operations, search, filtering |
| `WorkshopDownloader.cpp/.h` | HTTP requests to RLMAPS API, thread-safe downloads |
| `MapList.h` | Data structures: `MapEntry`, `TrainingEntry`, `WorkshopEntry` |
| `ConstantsUI.h` | All UI styling constants (colors, sizes, spacing) |

## Technical Patterns

**Thread Safety:** WorkshopDownloader and TrainingPackManager use mutexes. Downloads run in background threads to avoid blocking the game.

**Delayed Execution:** Uses `gameWrapper->SetTimeout()` to schedule commands after match-end. Minimum 0.1s delay prevents crashes during game state transitions.

**Settings Persistence:** All settings use BakkesMod CVars with `.addOnValueChanged()` callbacks for immediate sync. CVars auto-persist to `config.cfg`.

**UI Framework:** ImGui with DirectX 11 backend. Custom widgets in `imgui/` folder (range sliders, searchable combos, timeline).

## Data Locations

```
%APPDATA%\bakkesmod\bakkesmod\
├── plugins\SuiteSpot.dll
├── data\SuiteSpot\TrainingSuite\
│   ├── training_packs.json    (2300+ packs, 2.6MB)
│   └── pack_usage_stats.json  (user history)
└── cfg\config.cfg             (CVars/settings)
```

## External Dependencies

- **BakkesMod SDK:** Game hooks, CVars, ImGui integration
- **nlohmann/json:** JSON parsing (`json.hpp`)
- **RLMAPS API:** `https://celab.jetfox.ovh/api/v4/projects/` for workshop map search/download
- **PowerShell:** Used for ZIP extraction and training pack database updates

## CVar Naming Convention

All CVars use `suitespot_` prefix: `suitespot_enabled`, `suitespot_map_type`, `suitespot_delay_queue`, etc.

## Related Projects

**BakkesMod RAG Documentation System:** For a Python-based RAG system to query BakkesMod SDK documentation, see the separate repository at [github.com/MilesAhead1023/BakkesMod-RAG-Documentation](https://github.com/MilesAhead1023/BakkesMod-RAG-Documentation). This tool was originally developed to assist with SuiteSpot development but is now maintained independently.
