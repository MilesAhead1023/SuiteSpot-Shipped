# Step 3: Post-Match Logic Trace - Why Auto-Load is Broken

## Overview

**Step 3 of 7** - Complete **Step 1 & 2** first.

You've identified three candidates for post-match logic failure. This step traces the EXACT code path and finds where it breaks.

**Estimated token usage:** ~3000 tokens
**Time to complete:** 15 minutes

## Confirmed Candidates (From Step 1)

```
Issue 2: Post-Match Logic Broken

Candidate 1: AutoLoadFeature.cpp:65-100
- Removal of trainingMode == 1 logic affects code resolution

Candidate 2: SuiteSpot.cpp:225 (approx)
- useBagRotation hardcoded to false, dependencies may fail

Candidate 3: SettingsUI.cpp:45 (approx)
- LOAD NOW uses SetTimeout(0.0f), invalid timing
```

## Task: Trace Auto-Load Execution Flow

### Part 1: Map the Event Hook Chain

**File:** `SuiteSpot.cpp` - Find the EventMatchEnded hook.

Document the chain:

```bash
grep -n "EventMatchEnded\|GameEndedEvent" SuiteSpot.cpp
```

**Deliverable:**
```
Event Hook Chain:

1. Hook Registration:
   - Location: SuiteSpot.cpp:[Line]
   - Event hooked: [Exact event string]
   - Callback: [Function name]

2. GameEndedEvent() function:
   - Location: SuiteSpot.cpp:[Start] - [End]
   - What it does: [Brief description]

3. AutoLoadFeature::OnMatchEnded() call:
   - Location: SuiteSpot.cpp:[Line where it's called]
   - Parameters passed: [List all parameters]

4. Return flow:
   - What happens after OnMatchEnded returns? [Anything?]
```

### Part 2: Deep Dive - AutoLoadFeature::OnMatchEnded()

**File:** `AutoLoadFeature.cpp` - Read the `OnMatchEnded()` method completely.

Document the logic flow:

```
AutoLoadFeature::OnMatchEnded() Flow:

Entry point: AutoLoadFeature.cpp:[Line]

Step 1: Check if enabled
  - Location: [Line X]
  - Code: [Show the if statement]
  - Possible exit: [If not enabled, does it return?]

Step 2: Get settings (mapType, codes, delays)
  - Location: [Lines X-Y]
  - GetMapType() → mapType
  - GetCurrentFreeplayCode() → freeplayCode
  - GetCurrentTrainingCode() → trainingCode
  - GetCurrentWorkshopPath() → workshopPath

Step 3: Determine which map to load
  - Location: [Lines X-Y]
  - Decision logic: [Show the if/else branching on mapType]
  - What code/path is selected? [Trace which variable gets the selection]

Step 4: Get delay value
  - Location: [Line X]
  - If mapType == 0: delaySeconds = GetDelayFreeplaySec()
  - If mapType == 1: delaySeconds = GetDelayTrainingSec()
  - If mapType == 2: delaySeconds = GetDelayWorkshopSec()

Step 5: Build command string
  - Location: [Lines X-Y]
  - If freeplay: cmd = "load_freeplay " + code
  - If training: cmd = "load_training " + code
  - If workshop: cmd = "load_workshop " + path
  - Show the exact code that builds this string

Step 6: Execute via SetTimeout
  - Location: [Line X]
  - SetTimeout call: [Show the exact code]
  - Delay used: [delaySeconds or hardcoded?]
  - Lambda: [What code runs after delay?]

Exit point: AutoLoadFeature.cpp:[Line]
```

### Part 3: Candidate 1 - Bag Rotation Removal

**Question:** What code was deleted related to bag rotation?

```bash
git diff HEAD~5 HEAD -- AutoLoadFeature.cpp | grep -A 5 -B 5 "bag\|rotation\|trainingMode"
```

**Deliverable:**
```
Bag Rotation Removal Impact:

What was deleted:
- [Show the deleted code pattern]

Why it was deleted:
- [From commit message or code]

What replaced it:
- [Current logic]

Potential issue:
- If code depended on useBagRotation being toggleable
- And now it's hardcoded to false
- Then [scenario] would fail

Verification: Does the code still work without bag rotation?
- [Yes/No and why]
```

### Part 4: Candidate 2 - useBagRotation Hardcoded

**Location:** `SuiteSpot.cpp` line ~225

```bash
grep -n "useBagRotation" SuiteSpot.cpp
```

**Deliverable:**
```
useBagRotation Hardcoding:

Where is useBagRotation set?
  Location: SuiteSpot.cpp:[Line]
  Code: [Show the line]
  Value: false (hardcoded)

Where is it used?
  [List all locations where useBagRotation is read]

Issue assessment:
  - If useBagRotation is checked anywhere: [Where?]
  - And it's always false: [What happens?]
  - Does this break post-match? [Yes/No/How?]
```

### Part 5: Candidate 3 - SetTimeout(0.0f) Issue

**Location:** `SettingsUI.cpp` line ~45 (LOAD NOW button)

```bash
grep -n "LOAD NOW\|LoadPackImmediately\|SetTimeout.*0.0" SettingsUI.cpp | head -10
```

**Deliverable:**
```
LOAD NOW SetTimeout Issue:

Where is LOAD NOW implemented?
  Location: SettingsUI.cpp:[Line]
  Code: [Show the exact SetTimeout call]
  Delay: [Is it 0.0f? Show the value]

Why is 0.0f problematic?
  - SetTimeout(0.0f) means execute immediately or next frame?
  - Is this different from game-ending SetTimeout? [Show the delay used there]
  - Does immediate execution cause crashes? [Yes/No]

Impact on post-match:
  - If auto-load also uses SetTimeout(0.0f) → crashes
  - If auto-load uses longer delay → should work
  - So is this the issue? [Check the actual delay in OnMatchEnded]
```

### Part 6: Trace SettingsSync Access

**Question:** Are the getter methods correct?

**File:** `SuiteSpot.cpp` lines 112-122 (getter methods)

```bash
grep -n "GetCurrentTrainingCode\|GetMapType" SuiteSpot.cpp
```

**Deliverable:**
```
Settings Access Methods:

GetMapType():
  Returns: [settingsSync->GetMapType() or cached value?]
  Expected values: [0=freeplay, 1=training, 2=workshop]

GetCurrentTrainingCode():
  Returns: [What?]
  Source: [CVar directly or cached?]

GetCurrentFreeplayCode():
  Returns: [What?]
  Source: [CVar directly or cached?]

GetCurrentWorkshopPath():
  Returns: [What?]
  Source: [CVar directly or cached?]

Cache freshness issue:
  - If values are cached: Are they updated when CVar changes?
  - If not cached: Are lookups slow during every match?
```

### Part 7: Build Complete Flow Diagram

Create a visual trace of the ACTUAL code path:

```
Auto-Load Execution Path:

Match Ends
  ↓
EventMatchEnded hook fires
  ↓
SuiteSpot::GameEndedEvent() called [SuiteSpot.cpp:215]
  ↓
AutoLoadFeature::OnMatchEnded() called [SuiteSpot.cpp:220]
  │
  ├─ Check enabled? [Line X: if not enabled return]
  │
  ├─ Get mapType [Line Y: mapType = settingsSync->GetMapType()]
  │   └─ Returns: [0? 1? 2?]
  │
  ├─ Branch on mapType
  │   ├─ If 0 (freeplay):
  │   │   code = settingsSync->GetCurrentFreeplayCode() [Line Z]
  │   │   delay = settingsSync->GetDelayFreeplaySec()
  │   │   cmd = "load_freeplay " + code
  │   │
  │   ├─ If 1 (training):
  │   │   code = settingsSync->GetCurrentTrainingCode() [Line Z]
  │   │   delay = settingsSync->GetDelayTrainingSec()
  │   │   cmd = "load_training " + code
  │   │
  │   └─ If 2 (workshop):
  │       path = settingsSync->GetCurrentWorkshopPath()
  │       delay = settingsSync->GetDelayWorkshopSec()
  │       cmd = "load_workshop " + path
  │
  ├─ SetTimeout(delay) with lambda that executeCommand(cmd) [Line A]
  │
  └─ Return
     └─ Wait [delay] seconds
        └─ Execute: cvarManager->executeCommand(cmd)
           └─ Game loads map
```

### Part 8: Find the Break Point

Based on your trace, where does it fail?

```
Post-Match Logic Failure Diagnosis:

Failure scenario 1: useBagRotation dependency
  - Code path affected: [Which line?]
  - Symptom: [What goes wrong?]
  - Root cause: [Why?]
  - Fix: [What needs to change?]

Failure scenario 2: SetTimeout(0.0f) crash
  - Code path affected: [Which line?]
  - Symptom: [What goes wrong?]
  - Root cause: [Why?]
  - Fix: [What needs to change?]

Failure scenario 3: Code resolution
  - Code path affected: [AutoLoadFeature.cpp:65-100]
  - Symptom: [Wrong code selected? Empty code?]
  - Root cause: [What changed in bag removal?]
  - Fix: [What needs to change?]

THE ACTUAL BUG (singular):
  [Which scenario is the real issue?]
  Location: [File:Line]
  Mechanism: [What actually happens]
```

## Success Criteria for Step 3

✅ Full event hook chain documented
✅ AutoLoadFeature::OnMatchEnded() traced completely
✅ All three candidates analyzed with evidence
✅ Settings access methods verified
✅ Code path diagram showing exact flow
✅ Break point identified (where execution fails)
✅ Root cause statement with mechanism

## Deliverable

Reply with:
1. **Event Hook Chain** - How match-end triggers auto-load
2. **OnMatchEnded() Flow** - Step-by-step execution
3. **Bag Rotation Impact** - Consequences of removal
4. **useBagRotation Analysis** - Hardcoded value impact
5. **SetTimeout Analysis** - Timing issue verification
6. **Settings Access Verification** - Are getters correct?
7. **Break Point Diagnosis** - Exact line where it fails

## Next Step

**Step 4: Map Selection State Corruption** will trace why the wrong map loads.

---

**Critical:** Identify THE ONE LINE where post-match logic fails. Make it specific and measurable.
