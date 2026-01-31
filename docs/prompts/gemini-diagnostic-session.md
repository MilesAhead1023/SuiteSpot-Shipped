# Diagnostic Session - Performance Regression & Logic Bugs

## Critical Context

Read **`GEMINI.md`** first for complete project context, architecture, and patterns.

## Problem Statement

After the most recent commit, three critical issues have emerged:

1. **CRITICAL: Workshop browser kills frame rate** - Opening the workshop browser causes severe FPS drops and game instability
2. **CRITICAL: Post-match logic broken** - Auto-load after match ends is malfunctioning
3. **HIGH: Map selection state corruption** - When a map is selected, the wrong map loads (state not updating correctly)

## Your Diagnostic Mission

Perform a **full logic trace and code lint** to identify root causes:

### Phase 1: Establish Baseline

1. **Check current git state:**
   ```bash
   git status
   git log -5 --oneline
   git diff HEAD~1 WorkshopDownloader.cpp
   git diff HEAD~1 --stat
   ```

2. **Identify what changed in recent commits:**
   - What files were modified?
   - What logic was added/removed?
   - Were any mutex locks added/removed?
   - Were any render loops changed?

3. **Build and check for warnings:**
   ```bash
   msbuild SuiteSpot.sln /t:Clean /p:Configuration=Release /p:Platform=x64
   msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
   ```
   Document all compiler warnings (even suppressed ones).

### Phase 2: Frame Rate Regression Analysis

**Target:** Identify what in workshop browser rendering is causing FPS drops.

**Investigation checklist:**

1. **Read `SettingsUI.cpp` (Workshop Browser section):**
   - Lines 418-571 contain the workshop browser two-panel layout
   - Look for rendering loops that don't use `ImGuiListClipper`
   - Check for blocking I/O on render thread (file reads, network calls)
   - Look for expensive operations inside render loops (string allocations, JSON parsing)

2. **Read `WorkshopDownloader.cpp` (recently modified):**
   - Check for any render-thread blocking (mutex locks held too long)
   - Look for image loading on render thread (should be background thread)
   - Check for `system()` calls during render (PowerShell invocations)
   - Verify all long operations use `std::thread` or async patterns

3. **Check for mutex contention:**
   - Does workshop code use mutexes?
   - Are locks held during rendering?
   - Are there nested locks (deadlock risk)?
   - Pattern from `TrainingPackManager.h:std::mutex packMutex_`

4. **Verify virtual scrolling:**
   - Is `ImGuiListClipper` used for workshop map list?
   - Reference: `TrainingPackUI.cpp` uses clipper for 2000+ packs (lines ~480-510)
   - If missing, that's likely the FPS killer

5. **Check for texture/image issues:**
   - Are workshop map images loaded every frame?
   - Should be cached or loaded asynchronously
   - Reference: `WorkshopDownloader.cpp` should load images to disk, not memory

**Expected findings:**
- Missing `ImGuiListClipper` in workshop browser
- Blocking I/O on render thread
- Mutex held during entire render pass
- Images reloaded every frame

### Phase 3: Post-Match Logic Trace

**Target:** Trace why auto-load fails after match ends.

**Investigation checklist:**

1. **Read the event hook chain:**
   - `SuiteSpot.cpp:161-164` - `EventMatchEnded` hook
   - `SuiteSpot.cpp:215-243` - `GameEndedEvent()` implementation
   - `AutoLoadFeature.cpp` - `OnMatchEnded()` method

2. **Verify SetTimeout usage:**
   - CRITICAL: Must use `gameWrapper->SetTimeout()` with delay
   - Check `AutoLoadFeature.cpp` - ALL load commands should be deferred
   - Pattern: `SetTimeout([](GameWrapper* gw) { executeCommand(...); }, delay);`
   - If SetTimeout was removed/bypassed → that's the bug

3. **Check settings access:**
   - Does `AutoLoadFeature::OnMatchEnded()` call `settingsSync->GetMapType()`?
   - Does it correctly branch on mapType (0=freeplay, 1=training, 2=workshop)?
   - Are the delay CVars being read correctly (`GetDelayFreeplaySec()`, etc.)?

4. **Trace command execution:**
   - What command string is built? (`load_freeplay`, `load_training`, `load_workshop`?)
   - Is `cvarManager->executeCommand()` being called?
   - Add temporary LOG statements to trace execution path

5. **Check for removed code:**
   ```bash
   git diff HEAD~5 AutoLoadFeature.cpp
   git diff HEAD~5 SuiteSpot.cpp
   ```
   Look for deleted SetTimeout calls or changed timing logic.

**Expected findings:**
- SetTimeout removed (immediate execution crashes game)
- Wrong settings being read (mapType always 0 or wrong code)
- Command string malformed
- Callback lambda not capturing correct variables

### Phase 4: Map Selection State Corruption

**Target:** Find why selected map doesn't match loaded map.

**Investigation checklist:**

1. **Trace CVar update flow:**
   - When user selects map in UI, which CVar is set?
   - `suitespot_current_freeplay_code`?
   - `suitespot_current_training_code`?
   - `suitespot_current_workshop_path`?

2. **Read selection update code:**
   - `TrainingPackUI.cpp` - When row is clicked, what happens?
   - `SettingsUI.cpp` - When workshop map selected, what happens?
   - Look for `SetCVarSafely()` calls
   - Verify CVar name matches what AutoLoad reads

3. **Check for stale cache:**
   - Does `SettingsSync` cache CVar values locally?
   - Is the cache updated when CVar changes?
   - Pattern: `addOnValueChanged()` callbacks (see `SettingsSync.cpp`)
   - If callback missing → stale reads

4. **Verify getters:**
   - `SuiteSpot.cpp:112-122` - Getter methods delegate to SettingsSync
   - Do they return cached values or read CVars directly?
   - Cached = fast but can be stale
   - Direct read = slow but always fresh

5. **Check for variable shadowing:**
   - Multiple `selectedPackCode` variables?
   - One in UI, one in Settings, one in AutoLoad?
   - Are they synchronized?

**Expected findings:**
- CVar name mismatch (UI writes `suitespot_foo`, AutoLoad reads `suitespot_bar`)
- Missing `addOnValueChanged()` callback (stale cache)
- Selection state stored in wrong scope (UI-local vs plugin-global)
- Race condition (CVar updated after AutoLoad reads it)

### Phase 5: Function Signature Verification

**Verify all critical function signatures match expected patterns:**

1. **BakkesMod API calls:**
   - Check `docs/bakkesmod-sdk-reference.md` for correct signatures
   - Common errors:
     - Wrong parameter types (e.g., `std::string` vs `const char*`)
     - Missing null checks on wrapper returns
     - Incorrect memory_address usage

2. **ImGui API calls:**
   - Check `IMGUI/imgui.h` (v1.75) for correct signatures
   - Common errors:
     - Wrong `ImVec2` vs `ImVec4` usage
     - Missing `PushID()`/`PopID()` in loops
     - Unmatched `PushStyleColor()`/`PopStyleColor()`

3. **Mutex usage patterns:**
   - Every `std::lock_guard` has matching scope?
   - No mutex locked in one function, unlocked in another?
   - No recursive locking without `std::recursive_mutex`?

4. **Lambda captures:**
   - `[this]` used correctly (not `[&]` which can dangle)?
   - `SetTimeout` lambdas capture by value for safety?

### Phase 6: Collision Detection

**Check for:**

1. **Namespace collisions:**
   - All CVars prefixed with `suitespot_`?
   - Any global variables without static linkage?

2. **ImGui ID collisions:**
   - Each widget in loop has `PushID(unique_id)` / `PopID()`?
   - No duplicate window names?

3. **Memory corruption:**
   - Any raw pointers without ownership tracking?
   - Any `delete` without matching `new`?
   - Any `std::unique_ptr` double-deleted?

4. **Thread race conditions:**
   - Any shared data accessed without mutex?
   - Specifically check `RLWorkshop` vector (workshop maps)
   - Specifically check `RLTraining` vector (training packs)

## Diagnostic Commands

Run these to gather evidence:

```bash
# Check what changed recently
git log --oneline --graph -10
git show HEAD --stat
git diff HEAD~1 HEAD

# Search for potential issues
grep -r "TODO" --include="*.cpp" --include="*.h"
grep -r "FIXME" --include="*.cpp" --include="*.h"
grep -r "HACK" --include="*.cpp" --include="*.h"

# Find all SetTimeout calls (should be used for post-match loading)
grep -r "SetTimeout" --include="*.cpp"

# Find all mutex usage
grep -r "std::mutex" --include="*.h"
grep -r "lock_guard" --include="*.cpp"

# Find all render loops
grep -r "ImGuiListClipper" --include="*.cpp"
grep -r "BeginChild" --include="*.cpp"

# Check for blocking operations on render thread
grep -r "system(" --include="*.cpp"
grep -r "std::ifstream" --include="*.cpp"
```

## Expected Output Format

### Bug Report Template

For each issue found, document:

```markdown
## Bug #N: [Brief Title]

**Severity:** Critical / High / Medium / Low
**Component:** [File:Line]
**Type:** Performance / Logic / State / Threading

**Symptoms:**
- [What the user experiences]

**Root Cause:**
[Technical explanation with code references]

**Evidence:**
```cpp
// Code snippet showing the bug
[line numbers and actual code]
```

**Fix Strategy:**
[How to fix it - be specific]

**Verification:**
[How to test the fix]
```

## Deliverable

Provide:

1. **Bug inventory** - List of all issues found (with severity, location, root cause)
2. **Regression analysis** - What changed in recent commits that caused these bugs?
3. **Fix recommendations** - Specific code changes needed (with line numbers)
4. **Test plan** - How to verify each fix works

## Reference: Working Implementations

If you need examples of correct patterns, request access to:
- Git history (e.g., `git show <commit-hash>:WorkshopDownloader.cpp`)
- Specific commits where features worked correctly

The user has confirmed that working implementations exist in git history and can be provided as reference.

## Success Criteria

✅ **All 3 critical issues diagnosed** with root causes identified
✅ **Performance regression traced** to specific code change
✅ **Post-match logic flow documented** with where it breaks
✅ **Map selection bug isolated** to specific CVar/state issue
✅ **Function signatures verified** against BakkesMod SDK and ImGui v1.75
✅ **Fix recommendations provided** with specific line numbers and code changes

Begin your diagnostic investigation now.
