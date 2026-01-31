# hook pre-edit (BakkesMod)

Execute pre-edit validations for BakkesMod C++ files.

## Usage

```bash
npx claude-flow hook pre-edit --file <path> --project bakkesmod [options]
```

## Options

- `--file, -f <path>` - File path to be edited (required)
- `--validate-syntax` - C++20 syntax validation (default: true)
- `--check-imgui-api` - Verify ImGui v1.75 API usage (default: true)
- `--check-thread-safety` - Validate thread safety patterns (default: true)
- `--check-cvar-naming` - Ensure CVar naming conventions (default: true)
- `--check-includes` - Validate include guards and order (default: true)
- `--backup-file` - Create backup before editing (default: true)

## BakkesMod-Specific Validations

### 1. C++20 Syntax Validation

Checks file for C++20 compatibility:

**Common patterns to validate**:
- No use of removed features (e.g., `std::auto_ptr`)
- Proper use of `std::mutex`, `std::lock_guard`
- Modern `nullptr` instead of `NULL`
- Range-based for loops
- `std::atomic<T>` for thread safety

### 2. ImGui API Version Check (v1.75)

Verifies ImGui function signatures match v1.75:

**Critical APIs to validate**:
```cpp
// CORRECT (v1.75)
ImGui::BeginChild(const char* str_id, const ImVec2& size, bool border, ImGuiWindowFlags flags)
ImGui::Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags)
ImGui::Text(const char* fmt, ...)

// INCORRECT (newer versions)
ImGui::BeginChild(ImGuiID id, ...) // ID overload doesn't exist in v1.75
```

**Reference file**: `IMGUI/imgui.h` - This is the source of truth

### 3. Thread Safety Pattern Validation

Checks for proper mutex usage:

**Required patterns**:
```cpp
// CORRECT: Mutex protection for shared data
class Manager {
    std::mutex dataMutex_;
    std::vector<Entry> data_;
    
    void ModifyData() {
        std::lock_guard<std::mutex> lock(dataMutex_);
        // Safe to modify data_ here
    }
};

// WARNING: Unprotected shared data
class BadManager {
    std::vector<Entry> data_; // Accessed from multiple threads!
    
    void ModifyData() {
        data_.push_back(...); // RACE CONDITION!
    }
};
```

**Checks**:
- Training pack lists → Must be protected by `packMutex_`
- Workshop map lists → Must be protected by mutex
- UI render flags → Should use `std::atomic<bool>`
- CVar callbacks → No blocking operations

### 4. CVar Naming Convention

Ensures all CVar registrations use `suitespot_` prefix:

**CORRECT**:
```cpp
cvarManager->registerCvar("suitespot_enabled", "0", "Enable SuiteSpot", true, true);
cvarManager->registerCvar("suitespot_delay_queue", "3", "Queue delay", true, true);
```

**INCORRECT**:
```cpp
cvarManager->registerCvar("enabled", "0", ...);          // Missing prefix
cvarManager->registerCvar("ss_enabled", "0", ...);       // Wrong prefix
cvarManager->registerCvar("SUITESPOT_ENABLED", "0", ...); // Wrong case
```

**Validation regex**: `^suitespot_[a-z_]+$`

### 5. Include Guard Validation

Checks header files for proper include guards:

**Expected pattern**:
```cpp
// File: MapManager.h
#pragma once

// NOT:
#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H
// ...
#endif
```

**Standard**: Use `#pragma once` for all headers

### 6. CVar Null Safety

Validates CVar access patterns:

**CORRECT**:
```cpp
auto cvar = cvarManager->getCvar("suitespot_enabled");
if (cvar) {
    bool value = cvar.getBoolValue();
}
```

**INCORRECT**:
```cpp
bool value = cvarManager->getCvar("suitespot_enabled").getBoolValue(); // CRASH if doesn't exist!
```

### 7. Deferred Execution Pattern

Checks for proper use of `SetTimeout`:

**CORRECT**:
```cpp
// Post-match loading MUST be deferred
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    cvarManager->executeCommand("load_freeplay DFH_Stadium_P");
}, 0.5f); // Minimum 0.5s delay
```

**INCORRECT**:
```cpp
// Immediate loading in match-end handler
void OnMatchEnded() {
    cvarManager->executeCommand("load_freeplay DFH_Stadium_P"); // CRASH!
}
```

### 8. PowerShell Script Validation

Checks PowerShell invocations:

**CORRECT**:
```cpp
system("powershell.exe -ExecutionPolicy Bypass -NoProfile -NonInteractive -File script.ps1");
```

**INCORRECT**:
```cpp
system("powershell.exe -i -File script.ps1"); // -i (interactive) doesn't work
```

## File-Type Specific Checks

### For `.cpp` files:

- Precompiled header: First include must be `#include "pch.h"`
- Logging: Use `LOG()` macro, not `std::cout`
- No `main()` function (this is a DLL)

### For `.h` files:

- Include guards: Must use `#pragma once`
- Forward declarations: Prefer over includes when possible
- No implementation in headers (except templates)

### For UI files (`*UI.cpp`, `*UI.h`):

- ImGui namespaces: Use `UI::<Component>` namespace
- Virtual scrolling: Use `ImGuiListClipper` for large lists
- Thread safety: No blocking operations in render loop
- Constants: Use values from `ConstantsUI.h`

## Output

Returns JSON with validation results:

```json
{
  "continue": true,
  "file": "TrainingPackUI.cpp",
  "validations": {
    "syntax": "pass",
    "imguiApi": "pass",
    "threadSafety": "pass",
    "cvarNaming": "pass",
    "includes": "pass"
  },
  "warnings": [
    "Line 245: Consider using std::lock_guard instead of manual unlock"
  ],
  "errors": [],
  "backupPath": ".backups/TrainingPackUI.cpp.2024-01-31-13-22-00.bak"
}
```

## Auto-Fixes

Some issues can be auto-fixed:

- **Include order**: Automatically reorder to `pch.h` first
- **Include guards**: Convert `#ifndef` to `#pragma once`
- **Null safety**: Add CVar null checks

## Common Issues

### ImGui API Mismatch

**Error**: Using API not available in v1.75

**Solution**:
1. Check `IMGUI/imgui.h` for correct signature
2. Refer to `docs/bakkesmod_imgui_signatures_annotated.md`
3. Update code to use v1.75 compatible API

### Missing Mutex Protection

**Error**: Shared data structure accessed without lock

**Solution**:
```cpp
// Add mutex member
std::mutex dataMutex_;

// Wrap all access with lock
void ModifyData() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    // ... access data ...
}
```

### Immediate Post-Match Loading

**Error**: Load command in match-end handler without SetTimeout

**Solution**:
```cpp
void OnMatchEnded() {
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        cvarManager->executeCommand("load_training CODE");
    }, 1.0f); // Deferred by 1 second
}
```

## Integration

This hook is automatically called by Claude Code when:

- Using Edit or MultiEdit on `.cpp` or `.h` files
- Before refactoring operations
- During code generation

Manual usage:

```bash
# Before editing
npx claude-flow hook pre-edit --file "TrainingPackUI.cpp" --project bakkesmod

# With specific checks
npx claude-flow hook pre-edit -f "AutoLoadFeature.cpp" --check-thread-safety --backup-file
```

## See Also

- `hook post-edit` - Post-edit formatting
- [CLAUDE.md](../../CLAUDE.md) - Coding patterns
- [GEMINI.md](../../GEMINI.md) - Technical reference
- `IMGUI/imgui.h` - ImGui v1.75 API reference
