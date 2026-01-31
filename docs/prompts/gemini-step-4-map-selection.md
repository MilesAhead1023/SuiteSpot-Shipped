# Step 4: Map Selection State Corruption - Wrong Map Loads

## Overview

**Step 4 of 7** - Complete **Steps 1-3** first.

You've identified three candidates for map selection failure. This step traces state synchronization and finds where selection ≠ loaded map.

**Estimated token usage:** ~3000 tokens
**Time to complete:** 15 minutes

## Confirmed Candidates (From Step 1)

```
Issue 3: Map Selection State Corruption

Candidate 1: SettingsUI.cpp:305 (approx)
- RenderFreeplayMode writes to CVars inside render loop

Candidate 2: SettingsUI.cpp:430 (approx)
- selectedWorkshopIndex out of sync with currentWorkshopPath CVar

Candidate 3: WorkshopDownloader.cpp
- Stale callbacks from previous searches corrupt result list
```

## Task: Trace Selection State Flow

### Part 1: Understand the Three Selection Modes

**File:** `SettingsSync.h` - Find the CVar definitions.

```bash
grep -n "current_freeplay_code\|current_training_code\|current_workshop" SettingsSync.h
```

**Deliverable:**
```
CVar Definitions:

Mode 0 (Freeplay):
  CVar name: suitespot_current_freeplay_code
  Type: String
  Where stored: [CVar or SettingsSync member]
  Cached member: [Variable name in SettingsSync]

Mode 1 (Training):
  CVar name: suitespot_current_training_code
  Type: String
  Where stored: [CVar or SettingsSync member]
  Cached member: [Variable name in SettingsSync]

Mode 2 (Workshop):
  CVar name: suitespot_current_workshop_path
  Type: String
  Where stored: [CVar or SettingsSync member]
  Cached member: [Variable name in SettingsSync]

Synchronization:
  - When CVar changes, is callback triggered? [Yes/No]
  - Pattern: addOnValueChanged() [Location in SettingsSync.cpp]
```

### Part 2: Candidate 1 - CVar Writes in Render Loop

**Location:** `SettingsUI.cpp` line ~305 (RenderFreeplayMode)

```bash
grep -n "RenderFreeplayMode\|RenderTrainingMode\|RenderWorkshopMode" SettingsUI.cpp
```

**Deliverable:**
```
CVar Writes in Render Functions:

RenderFreeplayMode() [SettingsUI.cpp:Line X]:
  - Does it write to CVars? [Yes/No]
  - If yes, show the setValue() call: [Line Y and code]
  - When does it write? [Every frame? Only on user input?]
  - Condition: [What triggers the write?]

RenderTrainingMode() [SettingsUI.cpp:Line X]:
  - Does it write to CVars? [Yes/No]
  - If yes, show the code: [Line and pattern]

RenderWorkshopMode() [SettingsUI.cpp:Line X]:
  - Does it write to CVars? [Yes/No]
  - If yes, show the code: [Line and pattern]

Problem assessment:
  - If CVar written inside render loop:
    - Implication 1: Every render frame triggers CVar callback
    - Implication 2: Callback cascades may cause state corruption
    - Implication 3: Race conditions if multiple threads read/write

Issue severity: [Critical/High/Medium]
```

### Part 3: Candidate 2 - selectedWorkshopIndex vs currentWorkshopPath

**Location:** `SettingsUI.cpp` line ~430 (Workshop mode rendering)

Document the state variables:

```bash
grep -n "selectedWorkshopIndex\|currentWorkshopPath" SettingsUI.cpp
```

**Deliverable:**
```
Workshop Selection State Variables:

State variable 1: selectedWorkshopIndex
  - Defined where: [SettingsUI.h or SettingsUI.cpp]
  - Type: [int?]
  - What it tracks: [Which map is selected in list?]
  - Updated when: [User clicks map?]
  - Location of update: [SettingsUI.cpp:Line X]

State variable 2: currentWorkshopPath (from CVar)
  - Defined where: [SettingsSync member or CVar]
  - Type: [std::string?]
  - What it tracks: [Path to workshop map]
  - Updated when: [User selects map?]
  - Location of update: [SettingsUI.cpp:Line Y or SettingsSync]

Synchronization problem:
  - When user clicks map in list:
    1. selectedWorkshopIndex updated [Line X]
    2. Is currentWorkshopPath updated? [Yes/No/Where?]
    3. Are they updated TOGETHER in same block? [Yes/No]
    4. Or are they updated separately? [Yes/No]

Race condition scenario:
  - User clicks map at time T1
  - selectedWorkshopIndex updated
  - AutoLoadFeature reads currentWorkshopPath at time T2
  - Are they in sync? [Always/Sometimes/Never]

Evidence:
  - Show the code where selectedWorkshopIndex is updated
  - Show the code where currentWorkshopPath is updated
  - Are they in the same block? [Synchronized?]
```

### Part 4: Candidate 3 - Stale Workshop Downloader Callbacks

**Location:** `WorkshopDownloader.cpp`

Find the search result callback mechanism:

```bash
grep -n "callback\|searchGeneration\|resultsMutex" WorkshopDownloader.cpp | head -30
```

**Deliverable:**
```
Workshop Downloader Callback Mechanism:

Search initiation:
  - User enters search query
  - Where does search execute? [SettingsUI.cpp? WorkshopDownloader?]
  - Is it async? [Yes/No]

Callback registration:
  - When search starts, callback registered: [Location]
  - Callback signature: [What parameters?]
  - Where callback defined: [Function name]

Search generation tracking:
  - Is searchGeneration variable used? [Yes/No]
  - Where: [File:Line]
  - Purpose: [To prevent stale callbacks?]
  - How does it work: [Show the logic]

Stale callback scenario:
  - User searches for "map1" (search generation 1)
  - Results come back, callback fills RLMAPS_MapResultList
  - User immediately searches for "map2" (search generation 2)
  - Old callback from generation 1 arrives late
  - Does it overwrite generation 2 results? [Yes/No/How?]

Mutex protection:
  - Is resultsMutex used: [Yes/No]
  - Where: [Locations]
  - Is it sufficient to prevent corruption? [Yes/No/Why?]

Evidence of stale data:
  - Show any code that checks searchGeneration in callback
  - Show any code that doesn't check it (vulnerable)
```

### Part 5: Trace Selection → Load Flow

Create a complete state flow diagram:

```
User Selects Map → Load Path:

Freeplay Mode:
  ┌─ User clicks map in freeplay list [SettingsUI.cpp:Line X]
  ├─ Update: selectedFreeplayCode = code [Line Y]
  ├─ CVar write: suitespot_current_freeplay_code = code [Line Z]
  ├─ SetTimeout OR immediate?
  └─ Game loads: load_freeplay [code]

Training Mode:
  ┌─ User clicks pack in training list [TrainingPackUI.cpp or SettingsUI.cpp:Line X]
  ├─ Update: selectedTrainingCode = code [Line Y]
  ├─ CVar write: suitespot_current_training_code = code [Line Z]
  ├─ SetTimeout OR immediate?
  └─ Game loads: load_training [code]

Workshop Mode:
  ┌─ User clicks map in workshop list [SettingsUI.cpp:Line X]
  ├─ Update: selectedWorkshopIndex = index [Line Y]
  ├─ Derive path from index: path = RLWorkshop[index].path [Line Z]
  ├─ CVar write: suitespot_current_workshop_path = path [Line A]
  ├─ SetTimeout OR immediate?
  └─ Game loads: load_workshop [path]

Synchronization check:
  At each step, are ALL relevant variables updated?
  - In freeplay: selectedFreeplayCode + CVar + everything else?
  - In training: selectedTrainingCode + CVar + everything else?
  - In workshop: selectedWorkshopIndex + derived path + CVar + everything else?
```

### Part 6: CVar Callback Chain

**Question:** When CVar changes, what happens?

```bash
# Find all addOnValueChanged() calls
grep -n "addOnValueChanged" SettingsSync.cpp
```

**Deliverable:**
```
CVar Change Cascade:

For suitespot_current_training_code CVar:

Step 1: CVar value changes
  - Location: [Where is setValue called?]
  - New value: [code]

Step 2: Callback triggered
  - Does it have addOnValueChanged? [Yes/No]
  - If yes, callback: [Function name]
  - Location: [SettingsSync.cpp:Line X]

Step 3: Callback updates local cache
  - Code: [Show the update]
  - Variable updated: [settingsSync->currentTrainingCode or similar]

Step 4: Cascade check
  - Does changing currentTrainingCode trigger other updates?
  - Does AutoLoadFeature read this? [Yes/No/Where?]
  - Does UI re-render? [Yes/No]

Issue: If callback is missing
  - CVar changed but cache not updated
  - UI reads stale value
  - Wrong map loads
  - This could be THE BUG
```

### Part 7: Find the State Corruption Point

Based on all traces above, identify the break point:

```
Map Selection State Corruption Diagnosis:

Scenario A: CVar writes in render loop
  - Location: [SettingsUI.cpp:Line X]
  - Problem: [CVar changes every frame?]
  - Result: [State oscillates?]
  - Verdict: Is this the cause? [Yes/No/Why]

Scenario B: selectedWorkshopIndex ≠ currentWorkshopPath
  - Index selected: map 0
  - Path in CVar: path/to/map5
  - They're out of sync
  - When does this happen? [During update? After stale callback?]
  - Verdict: Is this the cause? [Yes/No/Why]

Scenario C: Stale callback overwrites
  - Search 1 completes, fills list with maps A, B, C
  - User clicks map B, CVar updated
  - Search 2 completes late, overwrites with maps X, Y, Z
  - User loads map thinking it's B, but it's actually X
  - Verdict: Is this the cause? [Yes/No/Why]

THE ROOT CAUSE:
  [Which scenario is actually happening?]
  Location: [File:Line]
  Mechanism: [Exactly what goes wrong?]
  Evidence: [Why are you certain?]
```

## Success Criteria for Step 4

✅ All three CVar modes understood (freeplay, training, workshop)
✅ CVar write locations identified
✅ selectedWorkshopIndex vs currentWorkshopPath analyzed
✅ Synchronization points documented
✅ Stale callback mechanism understood
✅ State flow diagram created
✅ CVar callback chain traced
✅ Corruption point identified

## Deliverable

Reply with:
1. **CVar Definitions** - Which CVars track what
2. **Render Loop Analysis** - Where are CVars written
3. **Workshop Index Analysis** - Sync issue between index and path
4. **Stale Callback Analysis** - Does generation check work
5. **State Flow Diagram** - Selection → Load path
6. **CVar Callback Chain** - What cascades when CVar changes
7. **Corruption Point** - Where exactly does state diverge

## Next Step

**Step 5: Function Signature Verification** will verify all APIs are called correctly.

---

**Critical focus:** Find the EXACT POINT where selection state and load state diverge. Make it specific and reproducible.
