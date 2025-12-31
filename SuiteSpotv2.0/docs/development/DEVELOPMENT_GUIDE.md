# SuiteSpot Development Guide

A quick reference for maintaining and extending the SuiteSpot plugin.

---

## Quick Links

| Document | Purpose | When to Use |
|----------|---------|------------|
| **CLAUDE_AI.md** | Architecture, APIs, constraints | Before writing any code |
| **UI_BUG_FIXES.md** | Stability issues tracker | When fixing bugs or debugging crashes |
| **TODO.md** | Feature requests and tasks | For planning work |
| **SESSION_SUMMARY.md** | Recent work log | To understand latest changes |
| **DECISIONS.md** | Design decision records | To understand "why" |
| **BUILD_TROUBLESHOOTING.md** | Compilation help | When build fails |

---

## Common Workflows

### Fix a Bug
1. Read **UI_BUG_FIXES.md** - Check if it's already documented
2. Review **CLAUDE_AI.md** - Understand relevant architecture
3. Make code changes following patterns shown in fixed issues
4. Build and verify: `0 errors, 0 warnings`
5. Update **UI_BUG_FIXES.md** with new issue (if not already there)
6. Commit with descriptive message

### Add a Feature
1. Check **TODO.md** - See if task already exists
2. Read **CLAUDE_AI.md** - Critical constraints section
3. Search **SuiteSpotDocuments/instructions/** - Look for related docs
4. Implement following existing code patterns
5. Build and test thoroughly
6. Update **TODO.md** when complete
7. Commit with feature description

### Debug a Crash
1. Check **UI_BUG_FIXES.md** pending section
2. Review **CLAUDE_AI.md** thread safety rules
3. Look for:
   - Null pointer dereferences (see Issue #5 pattern)
   - Unmatched ImGui calls (see Issue #1 pattern)
   - Invalid CVars (see SetCVarSafely usage)
4. Add null checks following patterns in completed fixes
5. Update **UI_BUG_FIXES.md** when fixed

### Investigate Build Failure
1. Check **BUILD_TROUBLESHOOTING.md** for similar errors
2. Review recent commits using: `git log --oneline -10`
3. Check linker errors - usually missing files or renamed symbols
4. Check compilation errors - usually API changes
5. Search related files for the symbol/file mentioned in error
6. Revert recent changes if uncertain, then rebuild

### Review Code Before Committing
1. Verify build succeeds:
   ```powershell
   Build succeeded. 0 Warning(s) 0 Error(s)
   ```
2. Check git diff for unintended changes
3. Ensure no build artifacts in commit (files in `build/` folder)
4. Update **UI_BUG_FIXES.md** if fixing issues
5. Update **TODO.md** if completing tasks
6. Write clear commit message following recent examples
7. Verify DLL deployed: `%APPDATA%\bakkesmod\bakkesmod\plugins\SuiteSpot.dll`

---

## Essential Patterns

### Safe CVar Access
✅ **Correct:**
```cpp
SetCVarSafely("suitespot_enabled", true);
```

❌ **Wrong:**
```cpp
plugin_->cvarManager->getCvar("suitespot_enabled").setValue(true);
```

**Location:** SettingsUI.cpp lines 901-909

---

### Safe File Operations
✅ **Correct:**
```cpp
std::ofstream out(path, std::ios::trunc);
if (!out.is_open()) {
    LOG("Failed to open file: {}", path);
    return;
}
// ... write data ...
if (out.fail()) {
    LOG("Error writing file: {}", path);
}
```

❌ **Wrong:**
```cpp
std::ofstream out(path);
out << data;  // Silent failure possible
```

**Location:** MapManager.cpp SaveTrainingMaps/SaveShuffleBag

---

### Input Validation
✅ **Correct:**
```cpp
// Validate before use
if (!std::filesystem::exists(path)) {
    LOG("Path does not exist: {}", path);
    return false;
}
// Safe to use path now
```

❌ **Wrong:**
```cpp
// Use without validation
std::filesystem::create_directories(path);  // May fail silently
```

**Location:** SettingsUI.cpp workshop path validation

---

### Efficient Pointer Checking
✅ **Correct:**
```cpp
if (auto* ptr = GetPointer()) {
    // Use ptr...
}
```

❌ **Wrong:**
```cpp
if (GetPointer()) {
    auto* ptr = GetPointer();  // Calls twice!
    // Use ptr...
}
```

**Location:** SettingsUI.cpp line 627

---

## Build & Test

### Standard Build
```powershell
cd C:\Users\bmile\suiteSpotProject\SuiteSpotv2.0
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" SuiteSpot.vcxproj /p:Configuration=Release /p:Platform=x64
```

### Verify Success
```
Build succeeded.
  0 Warning(s)
  0 Error(s)
```

### Test in Game
1. Launch Rocket League
2. Open console (F6)
3. Load plugin: `plugin load suitespot`
4. Show logs: `plugin show suitespot`
5. Test overlay: `ss_testoverlay`

---

## File Structure

```
SuiteSpotv2.0/
├── Core Plugin
│   ├── SuiteSpot.h/cpp       - Main plugin class
│   ├── Source.cpp            - Settings window entry
│   └── GuiBase.h/cpp         - Base GUI class
├── UI Systems
│   ├── SettingsUI.h/cpp      - Main settings window
│   ├── PrejumpUI.h/cpp       - Prejump packs UI
│   └── LoadoutUI.h/cpp       - Loadout management UI
├── Game Features
│   ├── AutoLoadFeature.h/cpp - Auto-load/queue
│   ├── LoadoutManager.h/cpp  - Car loadout switching
│   └── PrejumpPackManager.h/cpp - Prejump pack cache
├── Data Management
│   ├── MapManager.h/cpp      - File I/O & persistence
│   ├── MapList.h/cpp         - Map collections
│   └── SettingsSync.h/cpp    - CVar registration
├── Theme System
│   ├── ThemeManager.h/cpp    - Theme loading/rendering
│   ├── OverlayTypes.h        - Theme data structures
│   └── OverlayUtils.h/cpp    - Theme utilities
└── Documentation
    ├── CLAUDE_AI.md          - Architecture & APIs
    ├── UI_BUG_FIXES.md       - Stability tracker
    ├── TODO.md               - Task list
    ├── SESSION_SUMMARY.md    - Work log
    ├── DECISIONS.md          - Design rationale
    └── BUILD_TROUBLESHOOTING.md - Build help
```

---

## Critical Rules (From CLAUDE_AI.md)

**MUST DO:**
1. Register CVars only in `SettingsSync::RegisterAllCVars()`
2. Access game state only via `SetTimeout()` callback
3. Check null pointers before dereferencing (see Issue #5 pattern)
4. Validate file I/O success (see Issue #7 pattern)
5. Search docs before changing code

**MUST NOT:**
1. Store wrapper objects - fetch fresh on each use
2. Capture wrappers by reference in lambdas
3. Access CVars without null checking
4. Assume file operations succeed
5. Make changes without reading relevant docs

---

## Documentation Maintenance

### After Every Session
- [ ] Update **UI_BUG_FIXES.md** build date
- [ ] Mark any new issues as pending
- [ ] Mark completed fixes as ✅ COMPLETED
- [ ] Update **TODO.md** with new findings
- [ ] Verify build: `0 errors, 0 warnings`

### When Committing
- [ ] Write descriptive commit message
- [ ] Reference issue numbers (e.g., "Issue #5: ...")
- [ ] Exclude build artifacts
- [ ] All docs are current

### When Pushing
- [ ] Verify all tests pass locally
- [ ] Check that DLL loads without errors
- [ ] Review commit history for clarity

---

## Troubleshooting

**Build fails with "LNK2001: unresolved external"**
- Check file is added to `.vcxproj` file
- Check header exists where referenced
- See **BUILD_TROUBLESHOOTING.md**

**Plugin crashes on load**
- Check **UI_BUG_FIXES.md** pending section
- Look for null pointer dereferences
- Add null checks following Issue #5 pattern
- Check logs for error messages

**CVar not saving**
- Verify registered in `SettingsSync::RegisterAllCVars()`
- Use `SetCVarSafely()` not direct `setValue()`
- Check file permissions in `%APPDATA%\bakkesmod\`

**Overlay not rendering**
- Check theme files in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\themes\`
- Verify JSON syntax in theme config files
- Check log output for theme loading errors

---

## Commit Message Template

```
Short summary of changes (50 chars max)

## Summary
- Bullet point of each major change
- Reference any issues fixed (e.g., Issue #5)
- Note any architectural decisions

## Impact
- What breaks/changes for users
- Performance implications
- Compatibility notes

🤖 Generated with Claude Code

Co-Authored-By: Claude Haiku 4.5 <noreply@anthropic.com>
```

---

## Quick Reference: Recent Fixes

**2025-12-27 - UI Stability Pass**
- ✅ Issue #1: Null pointer check flow
- ✅ Issue #5: CVarWrapper null checks (SetCVarSafely)
- ✅ Issue #7: File I/O error handling
- ✅ Issue #18: Training pack code validation
- ✅ Issue #19: Double GetThemeManager calls
- ✅ Issue #17: Workshop path validation
- ⏳ Issue #2, #4, #8: Buffer replacement (deferred)
- ⏳ Issue #13: Thread safety (deferred)

See **UI_BUG_FIXES.md** for detailed documentation.

---

## Next Steps

1. **Read** CLAUDE_AI.md to understand architecture
2. **Review** UI_BUG_FIXES.md to understand stability improvements
3. **Check** TODO.md for current tasks
4. **Follow** patterns shown in completed fixes
5. **Keep** documentation current after changes
6. **Test** thoroughly before committing

---

**Last Updated:** 2025-12-27
**Build Status:** ✅ Verified 0 errors, 0 warnings
