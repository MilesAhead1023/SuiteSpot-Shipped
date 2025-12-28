# Architecture Decision Records (ADR)

This file documents key architectural decisions for SuiteSpot, explaining the **why** behind implementation choices. AI agents should consult this before making changes.

---

## ADR-001: Use Shuffle Bag Instead of Pure Random

**Date:** 2024
**Status:** Implemented

**Context:**
Users complained that random map selection sometimes repeated the same map multiple times in a row.

**Decision:**
Implemented shuffle bag algorithm that ensures all maps are played once before any can repeat.

**Consequences:**
- ✅ Better user experience (no immediate repeats)
- ✅ Maintains randomness while ensuring variety
- ⚠️ Requires persistence to `SuiteShuffleBag.txt` to maintain state across sessions

**Implementation:**
- `MapManager.cpp`: `LoadShuffleBag()`, `SaveShuffleBag()`, `GetRandomTrainingMap()`
- Data file: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\SuiteShuffleBag.txt`

**Don't Change:**
- Never replace with `rand()` or pure random selection
- Don't remove shuffle bag persistence logic

---

## ADR-002: Use SetTimeout for All Game State Access

**Date:** 2024
**Status:** Mandatory Pattern

**Context:**
BakkesMod wrappers can become invalid across frames. Direct access causes crashes.

**Decision:**
All game state access MUST use `gameWrapper->SetTimeout()` or `Execute()`.

**Consequences:**
- ✅ Thread-safe game state access
- ✅ Prevents crashes from stale wrappers
- ⚠️ Requires lambda captures by value
- ⚠️ Adds slight execution delay (usually negligible)

**Example:**
```cpp
// CORRECT
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    ServerWrapper server = gw->GetCurrentGameState();
    if (!server) return;
    // Use server immediately
}, 0.0f);

// WRONG - will crash
ServerWrapper server = gameWrapper->GetCurrentGameState();
```

**Don't Change:**
- Never remove SetTimeout wrappers "for simplicity"
- Don't cache wrappers in member variables

**See Also:**
- `SuiteSpotDocuments/instructions/context/thread-safety.instructions.md`

---

## ADR-003: Separate Settings UI from Plugin Logic

**Date:** 2024
**Status:** Implemented

**Context:**
ImGui rendering code is verbose and clutters main plugin logic.

**Decision:**
Settings UI lives in `SettingsUI.cpp` (`RenderMainSettingsWindow()`), with `Source.cpp` delegating `RenderSettings()` to the UI module. Core plugin logic remains in `SuiteSpot.cpp`.

**Consequences:**
- ✅ Clean separation of concerns
- ✅ Easier to modify UI without affecting game logic
- ⚠️ Settings must sync via CVars (registered in `onLoad()`)

**Files:**
- `SuiteSpot.cpp`: Core logic and event hooks
- `Source.cpp`: Settings UI entry point (delegates to SettingsUI)
- `SettingsUI.cpp/h`: Main ImGui settings window rendering and tab content
- `GuiBase.cpp/h`: Base class for settings window
- `SettingsSync.cpp/h`: CVar registration and settings state sync (keeps UI decoupled from core logic)
- `AutoLoadFeature.cpp/h`: Match-end auto-load/auto-queue logic using SetTimeout (ADR-002)

**Don't Change:**
- Don't move ImGui code into `SuiteSpot.cpp`
- Don't create direct dependencies from UI to game logic (use CVars)

---

## ADR-004: Store Training Maps in Plain Text

**Date:** 2024
**Status:** Implemented

**Context:**
Need to persist user's training pack codes across BakkesMod restarts.

**Decision:**
Use newline-delimited text file instead of JSON/binary format.

**Consequences:**
- ✅ Human-readable and editable
- ✅ Easy to debug and manually fix
- ✅ Simple parsing logic
- ⚠️ No metadata storage (just codes)

**Implementation:**
- File: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\SuiteSpotTrainingMaps.txt`
- Format: One training code per line (e.g., `A1B2-C3D4-E5F6-G7H8`)
- Functions: `SaveTrainingMaps()`, `LoadTrainingMaps()`

**Don't Change:**
- Don't migrate to JSON without user migration path
- Don't add complex serialization

---

## ADR-005: Use Precompiled Headers (pch.h)

**Date:** 2024
**Status:** Mandatory

**Context:**
BakkesMod SDK has large headers that slow compilation.

**Decision:**
Every `.cpp` file MUST start with `#include "pch.h"`.

**Consequences:**
- ✅ Much faster compilation (5-10x speedup)
- ⚠️ Requires `pch.cpp` to create PCH
- ⚠️ Every source file must include it first

**Implementation:**
- `pch.h`: Includes all BakkesMod SDK headers, STL, Windows headers
- `pch.cpp`: Single file that creates the precompiled header
- `.vcxproj`: Configured with `/Yu"pch.h"` (use) and `/Yc"pch.h"` (create)

**Don't Change:**
- Never remove `#include "pch.h"` from source files
- Don't put it after other includes (must be FIRST)

---

## ADR-006: Post-Build Auto-Deploy to BakkesMod

**Date:** 2024
**Status:** Implemented

**Context:**
Manual copying of DLL to BakkesMod plugins folder is tedious.

**Decision:**
Post-build event automatically:
1. Runs `bakkesmod-patch.exe` on the built DLL
2. Creates data directories if needed
3. Generates settings file

**Consequences:**
- ✅ Instant plugin reload in Rocket League
- ✅ No manual file copying
- ⚠️ Requires BakkesMod installation on build machine

**Implementation:**
- `.vcxproj`: `<PostBuildEvent>` section
- Output: `SuiteSpotv2.0\plugins\SuiteSpot.dll`
- Auto-deploy to: `%APPDATA%\bakkesmod\bakkesmod\plugins\`

**Don't Change:**
- Don't remove post-build steps (users rely on hot-reload)

---

## ADR-007: Split UI Rendering into Dedicated Modules

**Date:** 2025-12-26
**Status:** Implemented

**Context:**
Settings and overlay rendering grew beyond a single file and obscured feature logic.

**Decision:**
UI rendering is split into `SettingsUI`, `PrejumpUI`, `LoadoutUI`, and `OverlayRenderer`, with `Source.cpp` only acting as the settings window entry point.

**Consequences:**
- ✅ UI concerns are isolated and easier to modify
- ✅ Features remain ImGui-free and testable
- ⚠️ UI state must live inside UI modules, not feature modules

**Implementation:**
- `SettingsUI.cpp/h`: main settings window and tab routing
- `PrejumpUI.cpp/h`: prejump pack tab rendering
- `LoadoutUI.cpp/h`: loadout tab rendering
- `OverlayRenderer.cpp/h`: post-match overlay rendering
- `Source.cpp`: minimal entry point

**Don't Change:**
- Do not reintroduce ImGui rendering into feature modules
- UI modules should query state via SuiteSpot/SettingsSync only

---

## Template for New Decisions

When making architectural changes, add entries here:

```markdown
## ADR-XXX: [Decision Title]

**Date:** YYYY-MM-DD
**Status:** [Proposed / Implemented / Deprecated]

**Context:**
[What problem are we solving?]

**Decision:**
[What did we decide to do?]

**Consequences:**
- ✅ [Positive outcomes]
- ⚠️ [Trade-offs / considerations]
- ❌ [Negative outcomes]

**Implementation:**
[Where is this implemented? Key files/functions]

**Don't Change:**
[What should AI agents NOT do when working with this?]

**See Also:**
[Links to relevant docs]
```

---

## Usage for AI Agents

Before making changes to SuiteSpot:

1. **Check CLAUDE.md** for project overview and critical rules
2. **Check DECISIONS.md** (this file) for architectural context
3. **Check SuiteSpotDocuments/** for BakkesMod SDK patterns
4. **Ask user** if decision conflicts with existing ADRs

This ensures consistency and prevents regression of design decisions.
