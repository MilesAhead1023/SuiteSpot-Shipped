# Step 6: Collision & Race Condition Detection - Threading Issues

## Overview

**Step 6 of 7** - Complete **Steps 1-5** first.

This step finds namespace collisions, ImGui ID conflicts, and race conditions in shared data.

**Estimated token usage:** ~2500 tokens
**Time to complete:** 10-12 minutes

## Task: Find Threading & Collision Issues

### Part 1: Namespace & CVar Collision Check

**Question:** Are all CVars properly namespaced?

```bash
grep -n "registerCvar\|getCvar" SettingsSync.cpp | grep -v "suitespot_" | head -10
```

**Deliverable:**
```
CVar Namespace Verification:

All CVars found:
1. suitespot_enabled ✓
2. suitespot_delay_queue ✓
3. suitespot_current_training_code ✓
4. suitespot_current_freeplay_code ✓
5. suitespot_current_workshop_path ✓
6. [List all others]

Violations (missing suitespot_ prefix):
  - [Any CVars without prefix?]
  - Location: [File:Line]
  - Verdict: ✓ No violations / ✗ Collision risk

Global variable collisions:
  - Check for any global std::string, std::vector, etc.
  - Location: [File:Line]
  - Are they static (file-scoped)? [Yes/No]
  - Verdict: ✓ Safe / ✗ Global collision risk
```

### Part 2: ImGui ID Collision Detection

**Question:** Are all ImGui widgets in loops properly scoped with PushID/PopID?

Find loops with ImGui calls:

```bash
grep -n "PushID\|PopID\|Selectable\|Button" TrainingPackUI.cpp | head -30
```

**Deliverable:**
```
ImGui ID Pattern Verification:

Pattern 1: TrainingPackUI.cpp (Training pack list loop)
  Loop starts: [Line X: for loop]
  PushID call: [Line Y: PushID(...)]
  ImGui widgets: [Lines Z-A: Selectable, Button, etc.]
  PopID call: [Line B: PopID()]
  Verdict: ✓ Correct / ✗ Missing ID scope

Pattern 2: SettingsUI.cpp (Freeplay list loop)
  Loop starts: [Line X]
  PushID call: [Line Y or MISSING]
  Widgets: [Lines]
  PopID call: [Line or MISSING]
  Verdict: ✓ Correct / ✗ MISSING (collision risk)

Pattern 3: SettingsUI.cpp (Training list loop)
  [Repeat analysis]

Pattern 4: SettingsUI.cpp (Workshop search results loop)
  Loop starts: [Line X]
  PushID call: [MISSING?]
  Widgets: [Lines]
  PopID call: [MISSING?]
  Verdict: ✓ Correct / ✗ MISSING (collision risk)

Window name collisions:
  - Count distinct ImGui::Begin() calls
  - Are all window names unique? [Yes/No]
  - Window names found: [List them]
  - Duplicates: [Any?]
```

### Part 3: RLTraining Vector Race Condition

**Question:** Is RLTraining vector accessed safely?

Find all access points:

```bash
grep -n "RLTraining" SuiteSpot.h SuiteSpot.cpp TrainingPackUI.cpp *.cpp | head -30
```

**Deliverable:**
```
RLTraining Vector Access Analysis:

Declaration:
  Location: SuiteSpot.h:[Line]
  Type: std::vector<TrainingEntry> RLTraining
  Protected by mutex? [Yes/No/What mutex?]

Read points (where is it read?):
  1. TrainingPackUI.cpp:[Line X] - for (auto& pack : RLTraining)
     Mutex protected? [Yes/No]
     Verdict: ✓ Safe / ✗ Unprotected

  2. AutoLoadFeature.cpp:[Line Y] - RLTraining[index]
     Mutex protected? [Yes/No]
     Verdict: ✓ Safe / ✗ Unprotected

  3. [Any others?]

Write points (where is it written?):
  1. TrainingPackManager.cpp:[Line X] - RLTraining.push_back()
     Mutex protected? [Yes/No]
     Verdict: ✓ Safe / ✗ Unprotected

  2. [Any others?]

Race condition scenario:
  - Thread 1: UI reads RLTraining.size() and loops
  - Thread 2: PackManager writes to RLTraining (push_back)
  - Thread 1: Tries to access RLTraining[100] but size is now 50
  - Result: Out of bounds access or crash
  - Is this possible? [Yes/No/Why?]

Verdict: ✓ Safe / ✗ RACE CONDITION EXISTS
```

### Part 4: RLWorkshop Vector Race Condition

**Question:** Is RLWorkshop accessed safely?

```bash
grep -n "RLWorkshop" *.cpp *.h | head -30
```

**Deliverable:**
```
RLWorkshop Vector Access Analysis:

Declaration:
  Location: SuiteSpot.h:[Line]
  Type: std::vector<WorkshopEntry> RLWorkshop
  Protected by mutex? [Yes/No/What?]

Read points:
  1. SettingsUI.cpp (Workshop browser render):[Line X]
     Mutex protected? [Yes/No]
     Holding lock during entire render? [Yes/No/How long?]
     Verdict: ✓ Safe / ✗ Dangerous / ✗ Unprotected

  2. AutoLoadFeature.cpp:[Line Y]
     Mutex protected? [Yes/No]
     Verdict: ✓ Safe / ✗ Unprotected

Write points:
  1. MapManager.cpp:[Line X] - RLWorkshop.push_back()
     Mutex protected? [Yes/No]
     Verdict: ✓ Safe / ✗ Unprotected

  2. WorkshopDownloader.cpp:[Line Y] - Updates to RLWorkshop
     Mutex protected? [Yes/No]
     Verdict: ✓ Safe / ✗ Unprotected

Race condition scenario:
  - UI render thread iterates RLWorkshop
  - Download thread modifies RLWorkshop
  - Iterator invalidated → Crash
  - Is this possible? [Yes/No?]

Verdict: ✓ Safe / ✗ RACE CONDITION / ✗ PERFORMANCE (long lock)
```

### Part 5: RLMAPS_MapResultList Race Condition

**Question:** Are workshop search results protected?

```bash
grep -n "RLMAPS_MapResultList\|resultsMutex\|cachedResultList" SettingsUI.cpp | head -30
```

**Deliverable:**
```
Workshop Search Results Race Condition Analysis:

State variables:
  1. RLMAPS_MapResultList (global?)
  2. cachedResultList (local to function?)
  3. resultsMutex (protects what?)

Location of RLMAPS_MapResultList:
  Declared: [File:Line]
  Type: [std::vector<...>?]
  Scope: [Global? Class member? Where?]

Access pattern in RLMAPS_RenderSearchWorkshopResults:
  ```
  {
    std::lock_guard<std::mutex> lock(resultsMutex);  // Line X
    cachedResultList = RLMAPS_MapResultList;         // Line Y - Deep copy
  }
  // Now render cachedResultList
  for (auto& result : cachedResultList) {            // Line Z
    // Render item
  }
  ```

Analysis:
  - Lock duration: [How many lines?]
  - Is deep copy necessary? [Yes/No - why?]
  - Could this be optimized? [How?]
  - Does lock hold during entire render? [Yes/No]

Stale data scenario:
  - WorkshopDownloader adds results at T1
  - UI reads and caches at T2
  - WorkshopDownloader overwrites at T3
  - User loads map from cache → could be stale
  - Verdict: ✓ Handled / ✗ STALE DATA RISK

Search generation check:
  - Is searchGeneration tracked? [Yes/No]
  - Where: [File:Line]
  - Does it prevent stale callbacks? [Yes/No/How?]

Verdict: ✓ Safe / ✗ PERFORMANCE (slow copy) / ✗ STALE DATA
```

### Part 6: SettingsSync CVar Callback Cascade

**Question:** When one CVar changes, do others cascade?

```bash
grep -n "addOnValueChanged" SettingsSync.cpp
```

**Deliverable:**
```
CVar Callback Cascade Analysis:

CVar 1: suitespot_current_training_code
  Callback: [Function or lambda]
  Location: [SettingsSync.cpp:Line X]
  What it updates: [Which member variables?]
  Does it trigger other CVars? [Yes/No]

CVar 2: suitespot_current_freeplay_code
  Callback: [Function or lambda]
  Location: [Line X]
  Cascade: [Any?]

CVar 3: suitespot_map_type (if it exists)
  Callback: [Function or lambda]
  Location: [Line X]
  Cascade: [Changes which other CVars?]

Cascade scenario:
  - User changes suitespot_current_training_code
  - Callback 1 fires (suitespot_current_training_code)
  - Does it call setValue on other CVars? [Yes/No]
  - If yes, do those trigger callbacks? [Yes/No]
  - Result: Multiple callbacks in sequence or parallel?
  - Could this cause state inconsistency? [Yes/No/How?]

Verdict: ✓ Safe / ✗ Cascade risk / ✗ Potential for state corruption
```

### Part 7: Shared Data Access Summary

Create a matrix of all shared data:

```
Shared Data Access Safety Matrix:

┌──────────────────┬────────────────┬──────────┬─────────┬──────────────────┐
│ Data             │ Protected by   │ Read     │ Write   │ Safe?            │
├──────────────────┼────────────────┼──────────┼─────────┼──────────────────┤
│ RLTraining       │ [Mutex/None]   │ [Mutex?] │ [Mutex?]│ ✓ Safe / ✗ Race  │
│ RLWorkshop       │ [Mutex/None]   │ [Mutex?] │ [Mutex?]│ ✓ Safe / ✗ Race  │
│ RLMAPS_Results   │ resultsMutex   │ [Y/N]    │ [Y/N]   │ ✓ Safe / ✗ Slow  │
│ selectedCode     │ [Mutex/None]   │ [Y/N]    │ [Y/N]   │ ✓ Safe / ✗ Race  │
│ settingsSync     │ [How?]         │ [Y/N]    │ [Y/N]   │ ✓ Safe / ✗ Issue │
│ [Others?]        │                │          │         │                  │
└──────────────────┴────────────────┴──────────┴─────────┴──────────────────┘
```

### Part 8: Critical Findings Summary

```
Threading & Collision Critical Issues Found:

Issue 1: [Type: Race/Collision/Performance]
  Location: [File:Line]
  Impact: [Could cause crash? Data corruption? Slow?]
  Severity: ✓ Critical / ✓ High / ✓ Medium

Issue 2: [Type]
  Location: [File:Line]
  Impact: [What?]
  Severity: ✓ Critical / ✓ High / ✓ Medium

Issue 3: [Type]
  Location: [File:Line]
  Impact: [What?]
  Severity: ✓ Critical / ✓ High / ✓ Medium

MOST CRITICAL: [Which issue?]
Why: [Explain impact]
```

## Success Criteria for Step 6

✅ All CVars namespaced correctly
✅ ImGui ID collisions checked (PushID/PopID)
✅ RLTraining vector race conditions identified
✅ RLWorkshop vector race conditions identified
✅ RLMAPS_MapResultList access safety verified
✅ CVar callback cascades documented
✅ Shared data access matrix complete
✅ All critical threading issues found

## Deliverable

Reply with:
1. **CVar Namespace Check** - Violations? Yes/No
2. **ImGui ID Safety** - Collisions found? Yes/No
3. **RLTraining Analysis** - Race conditions? Yes/No
4. **RLWorkshop Analysis** - Race conditions? Yes/No
5. **RLMAPS Results Safety** - Protected? Safe? Performance issue?
6. **CVar Callback Cascade** - Any unwanted cascades?
7. **Shared Data Matrix** - All data accounted for?
8. **Critical Issues** - List all threading/collision issues

## Next Step

**Step 7: Fix Strategy & Implementation** will create an action plan based on all findings.

---

**Critical reminder:** Race conditions are subtle. Look for places where two threads access the same data without mutual exclusion.
