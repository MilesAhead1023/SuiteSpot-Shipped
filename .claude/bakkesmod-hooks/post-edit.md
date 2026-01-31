# hook post-edit (BakkesMod)

Execute post-edit formatting and validation for BakkesMod C++ files.

## Usage

```bash
npx claude-flow hook post-edit --file <path> --project bakkesmod [options]
```

## Options

- `--file, -f <path>` - File path that was edited (required)
- `--auto-format` - Format code with project style (default: true)
- `--validate-build` - Check if file still builds (default: false)
- `--check-thread-safety` - Audit thread safety (default: true)
- `--memory-key, -m <key>` - Store edit context in memory
- `--train-patterns` - Train neural patterns from edit

## Post-Edit Processing

### 1. Code Formatting

Applies BakkesMod project code style:

**Formatting rules**:
- Indentation: Tabs (4 spaces wide)
- Braces: K&R style (opening brace on same line)
- Line length: Prefer < 120 characters
- Spacing: Space after `if`, `for`, `while`
- Pointers: `Type* ptr` not `Type *ptr`

**Example transformation**:
```cpp
// Before formatting
void  foo(int*ptr,bool   flag){
if(flag)
{
doSomething(  );
}}

// After formatting
void foo(int* ptr, bool flag) {
    if (flag) {
        doSomething();
    }
}
```

### 2. Include Order Validation

Ensures correct include order:

**Standard order**:
```cpp
// 1. Precompiled header (pch.h)
#include "pch.h"

// 2. Corresponding header (for .cpp files)
#include "MapManager.h"

// 3. Project headers
#include "logging.h"
#include "ConstantsUI.h"

// 4. BakkesMod SDK headers
#include "bakkesmod/plugin/bakkesmodplugin.h"

// 5. System/library headers
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
```

### 3. Thread Safety Audit

Post-edit verification of thread safety:

**Checks**:
- All shared data access is mutex-protected
- No blocking operations in render loop
- Proper use of `std::atomic` for flags
- No race conditions in callbacks

**Example audit**:
```cpp
// FLAGGED: Potential race condition
class Manager {
    std::vector<Pack> packs_; // Shared data
    
    void AddPack(Pack p) {
        packs_.push_back(p); // WARNING: Not thread-safe!
    }
};

// RECOMMENDED FIX:
class Manager {
    std::mutex packMutex_;
    std::vector<Pack> packs_;
    
    void AddPack(Pack p) {
        std::lock_guard<std::mutex> lock(packMutex_);
        packs_.push_back(p); // Now thread-safe
    }
};
```

### 4. Memory Safety Check

Validates memory management:

**Checks**:
- No raw `new`/`delete` (use smart pointers)
- Proper RAII patterns
- No memory leaks in loops
- Safe string operations

**Example**:
```cpp
// FLAGGED: Raw pointer usage
void LoadData() {
    Data* data = new Data();
    ProcessData(data);
    delete data; // What if ProcessData throws?
}

// RECOMMENDED:
void LoadData() {
    auto data = std::make_unique<Data>();
    ProcessData(data.get()); // Automatic cleanup
}
```

### 5. API Usage Validation

Verifies BakkesMod and ImGui API patterns:

**BakkesMod patterns**:
```cpp
// CORRECT: CVar null check
auto cvar = cvarManager->getCvar("suitespot_enabled");
if (cvar) {
    cvar.setBoolValue(true);
}

// CORRECT: Deferred execution
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    cvarManager->executeCommand("load_training CODE");
}, 1.0f);
```

**ImGui patterns**:
```cpp
// CORRECT: Proper Begin/End pairing
if (ImGui::BeginChild("Panel", size, true)) {
    // ... content ...
}
ImGui::EndChild(); // Always called

// CORRECT: Virtual scrolling
ImGuiListClipper clipper;
clipper.Begin((int)items.size());
while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
        // ... render item i ...
    }
}
```

### 6. Logging Consistency

Ensures proper use of logging:

**Standard patterns**:
```cpp
// CORRECT: Use LOG macro
LOG("MapManager: Loaded {} workshop maps", count);

// INCORRECT: Don't use std::cout
std::cout << "Loaded " << count << " maps" << std::endl; // Wrong!
```

### 7. CVar Callback Validation

Checks CVar callback patterns:

**Expected pattern**:
```cpp
// Register CVar
auto enabledCvar = cvarManager->registerCvar("suitespot_enabled", "0");

// Add callback with null check
if (enabledCvar) {
    enabledCvar.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        isEnabled = cvar.getBoolValue(); // Cache locally for fast access
        LOG("SuiteSpot enabled: {}", isEnabled);
    });
}
```

### 8. Documentation Quality

Validates comments and documentation:

**Checks**:
- Complex algorithms have explanatory comments
- Public API functions have descriptions
- Magic numbers are explained
- TODOs have issue references

**Example**:
```cpp
// GOOD: Explains why
// We must delay loading by at least 0.5s because the game crashes
// if we load during the match-end sequence
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    LoadTraining(code);
}, 0.5f);

// BAD: Obvious comment
i++; // Increment i
```

## Output

Returns JSON with validation and formatting results:

```json
{
  "file": "TrainingPackUI.cpp",
  "formatted": true,
  "changes": {
    "includeOrder": "fixed",
    "whitespace": "normalized",
    "braceStyle": "consistent"
  },
  "validations": {
    "threadSafety": "pass",
    "memorySafety": "pass",
    "apiUsage": "pass",
    "logging": "pass"
  },
  "warnings": [
    "Line 342: Consider adding mutex protection for shared vector"
  ],
  "errors": [],
  "memorySaved": "ui/training-pack-browser/filter-implementation",
  "patternsTrained": 2
}
```

## Auto-Fixes Applied

### Include Order

Automatically reorders includes to project standard:

```cpp
// Before
#include <vector>
#include "MapManager.h"
#include "pch.h"

// After
#include "pch.h"
#include "MapManager.h"
#include <vector>
```

### Whitespace Normalization

- Trailing whitespace removed
- Consistent indentation (tabs)
- Blank lines normalized (max 2 consecutive)

### Brace Style

Ensures consistent K&R brace placement:

```cpp
// Before
if (condition)
{
    doSomething();
}

// After
if (condition) {
    doSomething();
}
```

## Memory Storage

Stores edit context for future reference:

**Stored information**:
- Edit description and rationale
- Patterns used (e.g., "added mutex protection")
- Related files modified
- Implementation decisions

**Memory key structure**: `<component>/<feature>/<detail>`

Examples:
- `ui/training-pack-browser/two-panel-layout`
- `core/auto-load/deferred-execution`
- `data/training-packs/heal-shot-count`

## Pattern Training

Learns from successful edits:

**Patterns tracked**:
- Thread safety implementations
- ImGui layout patterns
- CVar registration and callbacks
- Error handling approaches
- Performance optimizations

## Common Post-Edit Issues

### Build Errors After Edit

**Symptom**: Code formatted but doesn't build

**Solution**:
```bash
# Validate build
npx claude-flow hook post-edit --file "MyFile.cpp" --validate-build
```

If build fails, hook will:
1. Show compiler errors
2. Suggest fixes
3. Optionally revert to backup

### Thread Safety Warnings

**Symptom**: Hook flags potential race conditions

**Investigation**:
1. Review shared data access patterns
2. Add mutex protection if needed
3. Use `std::atomic` for simple flags

### ImGui Begin/End Mismatch

**Symptom**: Warning about unbalanced Begin/End calls

**Solution**:
```cpp
// Ensure every Begin has matching End
if (ImGui::BeginChild("Panel", size)) {
    // ...
}
ImGui::EndChild(); // Must always call, even if Begin failed
```

## Integration

This hook is automatically called by Claude Code when:

- After Edit tool completes
- Following code generation
- After file saves
- During refactoring

Manual usage:

```bash
# After editing
npx claude-flow hook post-edit --file "TrainingPackUI.cpp" --project bakkesmod

# With memory storage
npx claude-flow hook post-edit -f "AutoLoadFeature.cpp" -m "auto-load/match-end-handler"

# With pattern training
npx claude-flow hook post-edit -f "MapManager.cpp" --train-patterns
```

## Workflow Example

```bash
# 1. Pre-edit validation
npx claude-flow hook pre-edit -f "TrainingPackUI.cpp"

# 2. Make changes (via Claude Code Edit tool)
# ... editing ...

# 3. Post-edit processing (automatic)
npx claude-flow hook post-edit -f "TrainingPackUI.cpp" --auto-format

# 4. Verify build (optional)
npx claude-flow hook post-edit -f "TrainingPackUI.cpp" --validate-build
```

## See Also

- `hook pre-edit` - Pre-edit validation
- `hook pre-build` - Build environment checks
- [CLAUDE.md](../../CLAUDE.md) - Code style guidelines
- [.clang-format](../../.clang-format) - Formatting rules (if exists)
