# Copilot Agent Rules for SuiteSpot

This document explains how GitHub Copilot should work with the SuiteSpot project.

## Quick Start

1. Read: `CLAUDE.md` (same rules apply)
2. Reference: `../architecture/CLAUDE_AI.md` (core constraints)
3. Use patterns from: `../sessions/2025-12-27/UI_BUG_FIXES.md`

## Code Completion Rules

When Copilot suggests code:
- ✅ Accept if it includes null checks
- ✅ Accept if it validates file I/O
- ✅ Accept if it uses SetTimeout for game state
- ✅ Accept if it follows patterns from recent fixes

- ❌ Reject if it dereferences without null check
- ❌ Reject if it stores wrapper objects
- ❌ Reject if it accesses game state directly
- ❌ Reject if it uses file I/O without error checking

## Critical Constraints

**Read CLAUDE.md for full details. Quick summary:**

1. **Threading:** All game state via `gameWrapper->SetTimeout()`
2. **Wrappers:** Never store—fetch fresh each time
3. **CVars:** Use `SetCVarSafely<T>()` template
4. **File I/O:** Check `is_open()` and `fail()`
5. **Null checks:** Always validate pointers before use

## Code Style

**Use C++20 features:**
- `auto` for type deduction
- Structured bindings
- Range-based for loops
- Init-statement in if conditions

**Follow existing patterns:**
- Single responsibility functions
- Clear error logging
- Consistent naming (suitespot_* for CVars)
- PreCompiled headers (#include "pch.h" first)

## Build Verification

Every code completion must pass:
```
msbuild SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

Expected: `0 Errors 0 Warnings`

## Pattern Examples

### Pattern 1: Safe CVar Access
```cpp
// Copilot should suggest:
SetCVarSafely<bool>("setting_name", value);

// NOT:
plugin_->cvarManager->getCvar("setting_name").setValue(value);
```

### Pattern 2: Safe Game State Access
```cpp
// Copilot should suggest:
gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (gw.IsGameRunning()) {
        auto car = gw.GetLocalCar();
        // use car...
    }
}, 0.0f);

// NOT:
auto car = gameWrapper->GetLocalCar();
```

### Pattern 3: Safe File I/O
```cpp
// Copilot should suggest:
std::ofstream out(filepath);
if (!out.is_open()) {
    LOG("Failed to open: {}", filepath);
    return;
}
out << data;
if (out.fail()) {
    LOG("Error writing: {}", filepath);
}

// NOT:
std::ofstream out(filepath);
out << data;  // Silent failure possible
```

### Pattern 4: Safe Pointer Access
```cpp
// Copilot should suggest:
if (auto ptr = GetPointer()) {
    ptr->DoSomething();
}

// NOT:
auto ptr = GetPointer();
ptr->DoSomething();  // Crash if null
```

## Common Copilot Mistakes to Avoid

1. **Storing wrappers**: Replace any variable declaration of wrapper types with fetching fresh in callback
2. **Direct game state**: Wrap in SetTimeout()
3. **Unchecked CVars**: Change to SetCVarSafely<T>()
4. **Unchecked file I/O**: Add is_open() and fail() checks
5. **Unchecked pointers**: Add null check before dereference

## Testing

After Copilot generates code:
1. Build to check for compilation errors
2. Review logic for constraint violations
3. Test in-game if adding new features
4. Document changes in appropriate session folder

## Documentation

After accepting Copilot suggestions:
- Update `../sessions/YYYY-MM-DD/` with summary
- Update `TODO.md` if completing tasks
- Add new issues to `UI_BUG_FIXES.md` if bugs found

---

**Status:** ✅ Ready for integration
**Learn from:** `CLAUDE.md` (full rules) and `../architecture/CLAUDE_AI.md` (core reference)

