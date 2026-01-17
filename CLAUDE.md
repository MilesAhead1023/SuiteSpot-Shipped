# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Documentation Suite

**Before writing code, read the relevant documentation:**

| Document | Purpose |
|----------|---------|
| [BAKKESMOD_API_REFERENCE.md](docs/standards/BAKKESMOD_API_REFERENCE.md) | Complete SDK API reference - wrappers, methods, data structures |
| [CODING_STANDARDS.md](docs/standards/CODING_STANDARDS.md) | Code patterns, naming conventions, anti-patterns |
| [THREAD_SAFETY.md](docs/standards/THREAD_SAFETY.md) | Threading model, Execute pattern, synchronization |

**Quick rules:**
- Always null-check wrappers before use: `if (!car) return;`
- Use `gameWrapper->Execute()` for game operations from render thread
- Use `gameWrapper->SetTimeout()` for delayed operations (never `sleep()`)
- Prefix CVars with `suitespot_`

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
# Build from command line (requires Visual Studio 2022)
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64

# Or open SuiteSpot.sln in Visual Studio and build (F7)
```

**Build output:** `plugins\SuiteSpot.dll`

**Post-build:** The build automatically:
1. Creates data directories in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\`
2. Copies the scraper script
3. Runs `bakkesmod-patch.exe` to inject the DLL

**Pre-build:** `version.h` build number is auto-incremented via `CompiledScripts/update_version.ps1`

## Architecture

```
SuiteSpot (Plugin Hub)
├── MapManager         - Discovers workshop maps from filesystem
├── TrainingPackManager - Manages 2000+ training packs from Prejump.com
├── SettingsSync       - CVar persistence (BakkesMod settings)
├── AutoLoadFeature    - Core logic: what to load when match ends
├── LoadoutManager     - Car preset switching (thread-safe)
└── UI Layer
    ├── SettingsUI     - F2 settings menu
    ├── TrainingPackUI - Floating browser window for packs
    └── LoadoutUI      - Loadout dropdown
```

**Event Flow:** Match ends → BakkesMod hook → `SuiteSpot::GameEndedEvent()` → `AutoLoadFeature::OnMatchEnded()` → schedules game command via `gameWrapper->SetTimeout()`

## Key Files

| File | Purpose |
|------|---------|
| `SuiteSpot.h/cpp` | Plugin entry point, lifecycle, window management |
| `AutoLoadFeature.cpp` | Core automation logic (single `OnMatchEnded()` entry point) |
| `TrainingPackManager.cpp` | Training pack loading, filtering, shuffle bag |
| `SettingsSync.cpp` | All CVar registrations and settings cache |
| `MapList.h` | Data structures: `MapEntry`, `TrainingEntry`, `WorkshopEntry` |
| `logging.h` | `LOG()` and `DEBUGLOG()` macros with source location |

## Data Persistence

Training packs stored at: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\prejump_packs.json`

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

- Custom widgets in `imgui/`: `imgui_rangeslider.h`, `imgui_searchablecombo.h`, `imgui_timeline.h`
- TrainingPackUI uses `ImGuiListClipper` for virtual scrolling of 2000+ packs
- Browser window uses hybrid rendering to stay open when F2 menu closes

## Logging

```cpp
LOG("Message with {} args", value);           // Always logged
DEBUGLOG("Debug only: {}", value);            // Only when DEBUG_LOG = true in logging.h
```

## No Tests

This project has no automated tests. Validation is manual/runtime via BakkesMod console.
