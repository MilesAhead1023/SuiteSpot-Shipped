# Claude AI Agent Rules for SuiteSpot

This document explains how Claude Code should work with the SuiteSpot project.

## Your Primary References

**Before writing ANY code:**
1. Read: `../architecture/CLAUDE_AI.md` (5 min - core constraints)
2. Read: `../development/DEVELOPMENT_GUIDE.md` (10 min - workflows)
3. Reference: `../sessions/2025-12-27/UI_BUG_FIXES.md` (patterns from recent fixes)

## Critical Constraints (MANDATORY)

You MUST follow these rules or code will break:

### 1. Thread Safety — NON-NEGOTIABLE
**Rule:** All game state access goes through `gameWrapper->SetTimeout()`

```cpp
// ❌ WRONG - Direct game state access
if (gameWrapper->IsGameRunning()) {
    auto car = gameWrapper->GetLocalCar();
}

// ✅ CORRECT - Via SetTimeout callback
gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (gw.IsGameRunning()) {
        auto car = gw.GetLocalCar();
    }
}, 0.0f);
```

**Location:** `architecture/CLAUDE_AI.md` → "Critical Constraints"

### 2. Wrapper Lifetime
**Rule:** Never store wrapper objects across frames

```cpp
// ❌ WRONG - Storing wrapper
ServerWrapper server = gameWrapper->GetGameEvent();
// ... later frame ...
// server is now stale/invalid

// ✅ CORRECT - Fetch fresh each time
gameWrapper->SetTimeout([this](GameWrapper gw) {
    ServerWrapper server = gw.GetGameEvent();
    // Use immediately, don't store
}, 0.0f);
```

### 3. CVar Access Safety
**Rule:** Use `SetCVarSafely<T>()` template for all CVar operations

```cpp
// ❌ WRONG - No null checking
plugin_->cvarManager->getCvar("suitespot_enabled").setValue(true);

// ✅ CORRECT - Triple-layer null checks
SetCVarSafely<bool>("suitespot_enabled", true);
```

**Implementation:** See `sessions/2025-12-27/UI_BUG_FIXES.md` → Issue #5

### 4. File I/O Error Handling
**Rule:** Always check `is_open()` before and `fail()` after file operations

```cpp
// ❌ WRONG - No error checking
std::ofstream out(filepath);
out << data;  // Silent failure possible

// ✅ CORRECT - Full error checking
std::ofstream out(filepath);
if (!out.is_open()) {
    LOG("Failed to open: {}", filepath);
    return;
}
out << data;
if (out.fail()) {
    LOG("Error writing: {}", filepath);
}
```

**Location:** `sessions/2025-12-27/UI_BUG_FIXES.md` → Issue #7

### 5. Null Pointer Checks
**Rule:** Always validate before dereferencing

```cpp
// ❌ WRONG
auto tm = plugin_->GetThemeManager();
tm->LoadTheme(...);  // Crash if null

// ✅ CORRECT
if (auto tm = plugin_->GetThemeManager()) {
    tm->LoadTheme(...);
}
```

**More patterns:** `sessions/2025-12-27/UI_BUG_FIXES.md` → Issues #1, #5

## File Organization

```
SuiteSpotv2.0/
├── Source code (C++ headers and .cpp)
├── docs/                         ← You are here
├── README.md                     ← Plugin overview
└── TODO.md                       ← Active task list
```

**API References:** `reference/API/` — Find your wrapper class

**Code Structure:** `reference/CodeMap.md` — Understand module relationships

## Common Tasks

### Task: Fix a Bug

**Step 1:** Search for the bug
→ Check `sessions/2025-12-27/UI_BUG_FIXES.md` (6 fixed, 2 deferred)

**Step 2:** Find the pattern
→ Look at the "Code Changed" or "Solution" section
→ Copy the pattern shown

**Step 3:** Implement
→ Follow the pattern exactly
→ Add null checks (constraint #5)
→ Add error handling (constraint #4)

**Step 4:** Build & verify
→ Run PowerShell: `& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' SuiteSpot.sln /p:Configuration=Release /p:Platform=x64`
→ Or use bash: `powershell -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' SuiteSpot.sln /p:Configuration=Release /p:Platform=x64"`
→ Verify: `0 Errors 0 Warnings` (build output shows "Build succeeded")

**Step 5:** Update docs
→ Mark issue as fixed in `sessions/YYYY-MM-DD/` folder
→ Create new session folder if needed (format: YYYY-MM-DD)

### Task: Add a Feature

**Step 1:** Check constraints
→ Read `architecture/DECISIONS.md` (7 architecture decision records)
→ Understand if your feature aligns with design

**Step 2:** Check TODO list
→ Read `TODO.md` (active tasks)
→ Confirm feature isn't already planned

**Step 3:** Plan implementation
→ Use established patterns from `sessions/2025-12-27/`
→ Follow code style from existing implementations

**Step 4:** Implement
→ Write code following patterns
→ Add null checks
→ Add error handling
→ Include logging for debugging

**Step 5:** Build & test
→ Build with: `msbuild ...` (Release|x64)
→ Test manually in game
→ Verify no new issues

**Step 6:** Document
→ Update `TODO.md` mark feature complete
→ Create summary in `sessions/YYYY-MM-DD/`

### Task: Debug a Crash

**Step 1:** Check recent fixes
→ Read `sessions/2025-12-27/UI_BUG_FIXES.md`
→ Look for similar crash patterns

**Step 2:** Common causes
→ Null pointer dereference (see Issue #1, #5 patterns)
→ Unmatched ImGui calls (see Issue #1)
→ Invalid CVar access (see Issue #5)
→ File I/O failure (see Issue #7)

**Step 3:** Add debugging
→ Add null checks following patterns
→ Add logging to understand flow
→ Build and test

**Step 4:** Document the fix
→ Create new issue entry in `sessions/YYYY-MM-DD/UI_BUG_FIXES.md`
→ Show before/after code
→ Explain why it crashed and why the fix works

## What You Must Read BEFORE Coding

| File | Purpose | Read Time |
|------|---------|-----------|
| `../architecture/CLAUDE_AI.md` | Core reference (constraints) | 5 min |
| `../development/DEVELOPMENT_GUIDE.md` | Workflows & patterns | 10 min |
| `../sessions/2025-12-27/UI_BUG_FIXES.md` | Recent fixes & patterns | 15 min |
| `../architecture/DECISIONS.md` | Why decisions were made | 5 min (ref) |

**Total before-coding read:** ~30 minutes first time, ~10 minutes thereafter

## Build Command

### Full Path (Recommended - Most Reliable)
```powershell
# PowerShell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' `
  SuiteSpot.sln /p:Configuration=Release /p:Platform=x64

# Or from bash
powershell -Command "& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' SuiteSpot.sln /p:Configuration=Release /p:Platform=x64"
```

### Expected Output
```
Build succeeded.
    0 Warning(s)
    0 Error(s)

Time Elapsed 00:00:05.32
```

### DLL Location After Build
```
SuiteSpotv2.0/plugins/SuiteSpot.dll (1.1 MB)
```

### Post-Build Verification
- ✅ DLL created at `plugins/SuiteSpot.dll`
- ✅ Plugin auto-reloaded in BakkesMod
- ✅ Settings file generated
- ✅ Log shows: `[PostBuild] Success.`

**Complete instructions:** `../development/DEVELOPMENT_GUIDE.md` or `../archive/BUILD_TROUBLESHOOTING.md`

## Reference Paths

**When you need to...**

→ **Access GameWrapper API?**
  Read: `reference/API/GameWrapper.md`

→ **Access ServerWrapper API?**
  Read: `reference/API/ServerWrapper.md`

→ **Find a CVar?**
  Search: `architecture/CLAUDE_AI.md` → "Common Tasks" section

→ **Understand threading?**
  Read: `architecture/CLAUDE_AI.md` → "Critical Constraints"

→ **See an example of safe CVar usage?**
  Read: `sessions/2025-12-27/UI_BUG_FIXES.md` → Issue #5

→ **See an example of safe file I/O?**
  Read: `sessions/2025-12-27/UI_BUG_FIXES.md` → Issue #7

→ **See an example of input validation?**
  Read: `sessions/2025-12-27/UI_BUG_FIXES.md` → Issue #18

## What NOT to Do

❌ **Don't write code in git:**
→ Review docs first, understand patterns, then code

❌ **Don't use deprecated CLAUDE.md:**
→ This file (agents/CLAUDE.md) or architecture/CLAUDE_AI.md are current

❌ **Don't store wrappers:**
→ Always fetch fresh via SetTimeout()

❌ **Don't skip null checks:**
→ Every pointer access needs validation

❌ **Don't assume file I/O succeeds:**
→ Always check is_open() and fail()

❌ **Don't make changes without reading architecture:**
→ Read CLAUDE_AI.md first, understand constraints

## Success Criteria

After your work, you should be able to answer:

- ✅ "My code follows all critical constraints" (threading, wrappers, null checks, file I/O)
- ✅ "Build succeeds with 0 errors, 0 warnings"
- ✅ "I documented what I changed"
- ✅ "I updated TODO.md or UI_BUG_FIXES.md as appropriate"
- ✅ "I can explain why each constraint matters"

## Getting Help

**If you're stuck:**
1. Search `sessions/2025-12-27/UI_BUG_FIXES.md` for similar issue
2. Read relevant section of `architecture/CLAUDE_AI.md`
3. Check `development/DEVELOPMENT_GUIDE.md` → "Troubleshooting"
4. Look at git log to see how similar issues were solved before

---

**Last Updated:** 2025-12-27
**Status:** ✅ Active and current
**Next Steps:** Read `../architecture/CLAUDE_AI.md` before your first task

