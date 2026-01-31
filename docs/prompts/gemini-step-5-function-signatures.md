# Step 5: Function Signature Verification - API Correctness

## Overview

**Step 5 of 7** - Complete **Steps 1-4** first.

This step verifies that all critical function calls use correct signatures and parameters. Hidden API mismatches can cause silent failures.

**Estimated token usage:** ~2500 tokens
**Time to complete:** 10-12 minutes

## Task: Verify Critical API Calls

### Part 1: BakkesMod SetTimeout Signature

**Reference:** `docs/bakkesmod-sdk-reference.md`

Find all `SetTimeout` calls and verify signatures:

```bash
grep -n "SetTimeout\|gameWrapper->SetTimeout" *.cpp | head -20
```

**Deliverable:**
```
SetTimeout Signature Verification:

Expected signature (from SDK):
  gameWrapper->SetTimeout(std::function<void(GameWrapper*)> callback, float delay)

Location 1: AutoLoadFeature.cpp:[Line X]
  Actual call: [Show the exact code]
  Callback type: [Lambda? Function pointer?]
  Delay value: [Is it a number? Constant? Variable?]
  Verdict: ✓ Correct / ✗ Incorrect

Location 2: SuiteSpot.cpp:[Line X] (Pack healer)
  Actual call: [Show the exact code]
  Delay: [Value]
  Verdict: ✓ Correct / ✗ Incorrect

Location 3: [Any others?]

Issues found:
  - Delay = 0.0f when it should be > 0: [Where?]
  - Callback doesn't capture [this]: [Where?]
  - Missing or wrong parameters: [Where?]
```

### Part 2: BakkesMod ExecuteCommand Signature

**Reference:** CVarManagerWrapper in SDK

```bash
grep -n "executeCommand" *.cpp | head -15
```

**Deliverable:**
```
ExecuteCommand Signature Verification:

Expected signature:
  cvarManager->executeCommand(std::string command)

Location 1: AutoLoadFeature.cpp:[Line X]
  Actual call: [Show code]
  Command string: [What is it?]
  Verdict: ✓ Correct / ✗ Incorrect

Location 2: SettingsUI.cpp:[Line Y]
  Actual call: [Show code]
  Command string: [What is it?]
  Verdict: ✓ Correct / ✗ Incorrect

Location 3: TrainingPackUI.cpp:[Line Z]
  Actual call: [Show code]
  Command string: [What is it?]
  Verdict: ✓ Correct / ✗ Incorrect

Command string format check:
  "load_freeplay CODE" - Correct format? [Yes/No]
  "load_training CODE" - Correct format? [Yes/No]
  "load_workshop PATH" - Correct format? [Yes/No]

Issues found:
  - Missing space in command: [Where?]
  - Wrong command name: [Where?]
  - Empty code/path: [Where?]
```

### Part 3: ImGui ListClipper Signature

**Reference:** `IMGUI/imgui.h` v1.75

```bash
grep -n "ImGuiListClipper" *.cpp | head -20
```

**Deliverable:**
```
ImGuiListClipper Signature Verification:

Expected pattern:
  ImGuiListClipper clipper;
  clipper.Begin((int)items.size());
  while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
      // Render item i
    }
  }

Location 1: TrainingPackUI.cpp:[Lines X-Y]
  Actual code: [Show the pattern]
  Verdict: ✓ Correct / ✗ Incorrect

Location 2: SettingsUI.cpp (Workshop browser):[Lines X-Y]
  Is ImGuiListClipper used? [Yes/No]
  If yes: [Show the code]
  If no: THIS IS A BUG
  Verdict: ✓ Correct / ✗ MISSING (CRITICAL)

Issues found:
  - Begin() not called: [Where?]
  - Step() not called: [Where?]
  - DisplayStart/DisplayEnd not used: [Where?]
  - Missing entirely in workshop: [YES/NO]
```

### Part 4: Mutex & Lock Guard Usage

**Reference:** C++ STL patterns

```bash
grep -n "std::mutex\|lock_guard\|unique_lock" *.cpp *.h | head -30
```

**Deliverable:**
```
Mutex Signature & Scope Verification:

Location 1: TrainingPackManager.h
  Declaration: std::mutex packMutex_;
  Type: ✓ Correct / ✗ Incorrect

Usage 1: [File:Line X]
  Pattern: std::lock_guard<std::mutex> lock(packMutex_);
  Scope: [How many lines is lock held?]
  Verdict: ✓ Safe / ✗ Long / ✗ Missing

Usage 2: [File:Line Y]
  Pattern: [Show code]
  Scope: [How many lines?]
  Verdict: ✓ Safe / ✗ Issue

Usage 3: WorkshopDownloader or SettingsUI
  Is resultsMutex used? [Yes/No]
  Where: [Locations]
  Scope during render: [Is it held across entire render?]
  Verdict: ✓ Safe / ✗ DANGEROUS (held too long)

Issues found:
  - Recursive lock without recursive_mutex: [Where?]
  - Lock not released properly: [Where?]
  - Lock held during render (slow): [Where?]
```

### Part 5: CVar Registration & Access

**Reference:** `docs/bakkesmod-sdk-reference.md`

```bash
grep -n "registerCvar\|getCvar" SettingsSync.cpp | head -20
```

**Deliverable:**
```
CVar Registration Verification:

Location 1: SettingsSync.cpp (registerCvar)
  Example: cvarManager->registerCvar("suitespot_enabled", "0", ...)
  Format: ✓ Correct / ✗ Incorrect
  All CVars prefixed with "suitespot_": ✓ Yes / ✗ No

CVar Access Verification:

Location 2: SuiteSpot.cpp (getCvar)
  Pattern: auto cvar = cvarManager->getCvar("suitespot_enabled");
  Is it null-checked? [Yes/No]
  If no: [Line X is vulnerable to crash]

Location 3: AutoLoadFeature.cpp
  Does it access CVars? [Yes/No]
  If yes, are they null-checked? [Yes/No/Where?]

Location 4: SettingsSync.cpp (getCvar)
  Does it call getCvar? [Yes/No]
  If yes, pattern: [Show code]
  Null check: [Yes/No]

Issues found:
  - CVar accessed without null check: [Where?]
  - CVar name typo (prefix missing): [Where?]
  - CVar never initialized (registerCvar missing): [Where?]
```

### Part 6: TrainingEditorWrapper Usage

**Reference:** `docs/bakkesmod-sdk-reference.md`

```bash
grep -n "TrainingEditorWrapper\|GetTrainingData\|GetTotalRounds" SuiteSpot.cpp
```

**Deliverable:**
```
TrainingEditorWrapper Signature Verification:

Location: SuiteSpot.cpp:247-317 (TryHealCurrentPack function)

Construction:
  Code: TrainingEditorWrapper editor(server.memory_address);
  Is memory_address passed correctly? [Yes/No]
  Verdict: ✓ Correct / ✗ Incorrect

Method calls:

1. editor.GetTrainingData()
   Expected return: TrainingEditorSaveDataWrapper
   Actual: [Show the code]
   Verdict: ✓ Correct / ✗ Incorrect

2. editor.GetTotalRounds()
   Expected return: int (shot count)
   Actual: [Show the code]
   Verdict: ✓ Correct / ✗ Incorrect

3. trainingData.GetTrainingData()
   Expected return: TrainingEditorSaveDataWrapper
   Actual: [Show the code]
   Verdict: ✓ Correct / ✗ Incorrect

4. saveData.GetCode()
   Expected return: FStringWrapper
   Actual: [Show the code]
   Null checks: [Are return values checked before use?]

Issues found:
  - Method call on null pointer: [Where?]
  - Wrong method name: [Where?]
  - Missing null check: [Where?]
```

### Part 7: Lambda Captures - Critical for SetTimeout

**Reference:** C++ lambda patterns

```bash
grep -n "SetTimeout.*\[" *.cpp | head -15
```

**Deliverable:**
```
Lambda Capture Analysis:

Location 1: AutoLoadFeature.cpp (SetTimeout lambda)
  Capture: [Show what's captured: [this], [&], [=], etc.]
  Should be: [this] captured by value for safety
  Actual: [Show code]
  Verdict: ✓ Safe / ✗ DANGEROUS

Location 2: SuiteSpot.cpp (Pack healer SetTimeout)
  Capture: [Show]
  Verdict: ✓ Safe / ✗ DANGEROUS

Location 3: SettingsUI.cpp (If using SetTimeout)
  Capture: [Show or "Not used"]
  Verdict: ✓ Safe / ✗ DANGEROUS

Issues found:
  - [&] capture (dangling references): [Where?]
  - Missing [this] when needed: [Where?]
  - Capturing pointer and using it later: [Where?]

Critical: If [&this] used, it can dangle after function returns
```

### Part 8: Summary - API Correctness Score

Create a scorecard:

```
API Correctness Verification Summary:

✓ SetTimeout signatures correct: [X/Y correct]
✓ ExecuteCommand signatures correct: [X/Y correct]
✓ ImGuiListClipper usage: [X/Y correct or MISSING in workshop]
✓ Mutex usage safe: [X/Y safe, 0 dangerous holdings]
✓ CVar null checks: [X/Y safe, 0 unsafe access]
✓ TrainingEditorWrapper usage: [X/Y correct]
✓ Lambda captures safe: [X/Y safe]

CRITICAL ISSUES FOUND: [Count]
HIGH ISSUES FOUND: [Count]
MEDIUM ISSUES FOUND: [Count]

Most dangerous issue: [Which one?]
```

## Success Criteria for Step 5

✅ All SetTimeout calls verified
✅ All executeCommand calls verified
✅ ImGuiListClipper presence/absence confirmed
✅ Mutex locks scoped correctly
✅ CVar access null-checked
✅ TrainingEditorWrapper usage correct
✅ Lambda captures safe
✅ API mismatches identified

## Deliverable

Reply with:
1. **SetTimeout Verification** - All calls documented
2. **ExecuteCommand Verification** - Command strings correct
3. **ImGuiListClipper Check** - Present in workshop? Yes/No
4. **Mutex Analysis** - Safe scoping? Yes/No
5. **CVar Safety** - Null checks present? Yes/No
6. **TrainingEditorWrapper** - Usage patterns correct
7. **Lambda Captures** - All safe? Yes/No
8. **Critical Issues** - Any API mismatches found?

## Next Step

**Step 6: Collision & Race Condition Detection** will find threading issues.

---

**Critical reminder:** API mismatches often fail silently. Document each call with exact code and verdict.
