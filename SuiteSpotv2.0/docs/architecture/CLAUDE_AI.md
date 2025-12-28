# AI Development Reference

## Build
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```
Output: `SuiteSpotv2.0\plugins\SuiteSpot.dll`

## Critical Constraints

**Thread Safety (MANDATORY):**
- All game state: `gameWrapper->SetTimeout([this](GameWrapper* gw) { ... }, 0.0f)`
- Never store wrappers
- Never capture by reference in lambdas
- See: `SuiteSpotDocuments/instructions/context/thread-safety.instructions.md`

**Wrapper Pattern:**
```cpp
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    ServerWrapper server = gw->GetCurrentGameState();
    if (!server) return;
    // Use fresh wrapper immediately
}, 0.0f);
```

**CVars:**
- Register only in `SettingsSync::RegisterAllCVars()`
- Naming: `suitespot_*`
- All updates go to SettingsSync state via callback

**Persistence:**
- `SaveTrainingMaps()` after modifying `RLTraining`
- Location: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\`

**Documentation First (MANDATORY):**
- **Rule:** Before writing code, search `SuiteSpotv2.0\SuiteSpotDocuments\instructions` for relevant `.md` files.
- **Requirement:** State "Read [filename]" or "No docs found for [topic]" before changing code.

**File Format:**
- Every `.cpp` starts: `#include "pch.h"`
- Tabs, not spaces
- No raw `new`/`delete` without matching pair

## Architecture

| Module | File | Purpose |
|--------|------|---------|
| Plugin | `SuiteSpot.h/cpp` | Lifecycle, state, hooks |
| Settings Sync | `SettingsSync.h/cpp` | CVar registration and state cache |
| Maps | `MapList.h/cpp` | RLMaps, RLTraining, RLWorkshop |
| Map Manager | `MapManager.h/cpp` | Persistence, shuffle bag, workshop discovery |
| Auto Load | `AutoLoadFeature.h/cpp` | Match-end auto-load/queue |
| Prejump Packs | `PrejumpPackManager.h/cpp` | Scrape/cache/filter pack data |
| UI Shell | `Source.cpp` | Settings window entry point |
| Settings UI | `SettingsUI.h/cpp` | Main settings window |
| Prejump UI | `PrejumpUI.h/cpp` | Prejump tab rendering |
| Loadout UI | `LoadoutUI.h/cpp` | Loadout tab rendering |
| Overlay | `OverlayRenderer.h/cpp` | Post-match overlay rendering |
| Loadouts | `LoadoutManager.h/cpp` | Car loadout switching |

## Key APIs

**GameWrapper:** `GetCurrentGameState()`, `SetTimeout(lambda, delay)`, `HookEventWithCallerPost()`

**ServerWrapper:** `GetBall()`, `GetCars()`, `GetTeams()`, `GetPlayers()`

**Event Hooks:**
- `Function TAGame.GameEvent_Soccar_TA.EventMatchEnded` → auto-queue
- `Function TAGame.AchievementManager_TA.HandleMatchEnded` → post-match stats

See: `SuiteSpotDocuments/instructions/context/` for full API docs

## Design Rules (from DECISIONS.md)

- **ADR-001:** Shuffle bag algorithm for non-repeating maps
- **ADR-002:** SetTimeout for ALL game state access

## Common Tasks

**Add CVar:**
```cpp
cvarManager->registerCvar("suitespot_name", "default", "Desc", true, true, 0, true, 100)
    .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        memberVar = cvar.getBoolValue();
    });
```

**Load maps after change:**
```cpp
SaveTrainingMaps();
LoadTrainingMaps();
```

**Hook event:**
```cpp
gameWrapper->HookEventWithCallerPost(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    bind(&SuiteSpot::OnMatchEnded, this, placeholders::_1, placeholders::_2, placeholders::_3));
```

## Testing
```
plugin load suitespot    # Load in RL console (F6)
plugin show suitespot    # View logs
```

## Development Tracking

**Important:** After any session making code changes, update both:
1. `UI_BUG_FIXES.md` - For stability/correctness fixes
2. `TODO.md` - For feature work and remaining tasks

**When to Update:**
- After completing any fix: Mark as ✅ COMPLETED in UI_BUG_FIXES.md
- When discovering new issues: Add to pending fixes section
- After session: Verify build succeeds and update build date
- Before committing: Ensure all tracking documents are current

**Build Verification:**
Always run full build before updating docs:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" SuiteSpot.vcxproj /p:Configuration=Release /p:Platform=x64
```

---

## Reference

- **UI Fixes:** `UI_BUG_FIXES.md` - Stability and correctness improvements
- **Docs:** `SuiteSpotDocuments/instructions/context/`
- **Decisions:** `DECISIONS.md`
- **Build Issues:** `BUILD_TROUBLESHOOTING.md`
- **Release:** `DEPLOYMENT.md`
- **Tasks:** `TODO.md`
