# SuiteSpot - BakkesMod Plugin

**Purpose:** Automated map loading for Rocket League with Freeplay, Training Pack, and Workshop map support.

**Build:** `msbuild SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64`

**Output:** `SuiteSpotv2.0\plugins\SuiteSpot.dll`

## Code Organization

| Module | Path | Purpose |
|--------|------|---------|
| Main Plugin | `SuiteSpot.h/cpp` | Lifecycle, hooks, shared state |
| Settings Sync | `SettingsSync.h/cpp` | CVar registration and settings state |
| Maps | `MapList.h/cpp` | RLMaps, RLTraining, RLWorkshop vectors |
| Map Manager | `MapManager.h/cpp` | Map persistence, shuffle bag, workshop discovery |
| Auto Load | `AutoLoadFeature.h/cpp` | Match-end auto-load/auto-queue |
| Prejump Packs | `PrejumpPackManager.h/cpp` | Scrape/cache/filter pack data |
| UI Shell | `Source.cpp` | Settings window entry point |
| Settings UI | `SettingsUI.h/cpp` | Main settings window/tabs |
| Prejump UI | `PrejumpUI.h/cpp` | Prejump pack tab UI |
| Loadout UI | `LoadoutUI.h/cpp` | Loadout management UI |
| Overlay | `OverlayRenderer.h/cpp` | Post-match overlay rendering |
| Loadouts | `LoadoutManager.h/cpp` | Car loadout switching |
| Base | `GuiBase.h/cpp` | Settings window infrastructure |

## Critical Patterns

**Thread Safety:** All game state access via `gameWrapper->SetTimeout()` or `Execute()`. Never store wrappers.

**Event Hooks:** 
- `EventMatchEnded` → triggers auto-queue
- `HandleMatchEnded` → collects post-match stats

**Data Path:** `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\`

**CVar Format:** `suitespot_*` naming convention. Register only in `onLoad()`.

**Persistence:** Call `SaveTrainingMaps()` after modifying `RLTraining` vector.

## Key References

- **Architecture:** `SuiteSpotDocuments/instructions/context/`
- **Design Decisions:** `DECISIONS.md`
- **Full API Docs:** `SuiteSpotDocuments/instructions/context/*.md`

**Version:** 1.0.0 (Build 312)
