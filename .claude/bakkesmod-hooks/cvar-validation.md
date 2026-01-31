# hook cvar-validation (BakkesMod)

Execute CVar-specific validation for BakkesMod plugin development.

## Usage

```bash
npx claude-flow hook cvar-validation --project bakkesmod [options]
```

## Options

- `--file, -f <path>` - Specific file to check (optional, checks all if omitted)
- `--check-naming` - Validate CVar naming conventions (default: true)
- `--check-registration` - Validate CVar registration patterns (default: true)
- `--check-callbacks` - Validate callback implementations (default: true)
- `--check-null-safety` - Ensure null checks before CVar access (default: true)
- `--check-sync` - Validate local variable synchronization (default: true)
- `--auto-fix` - Attempt to fix naming violations (default: false)

## CVar Validation Rules

### 1. Naming Convention

All CVars must use the `suitespot_` prefix:

**CORRECT examples**:
```cpp
cvarManager->registerCvar("suitespot_enabled", "0");
cvarManager->registerCvar("suitespot_delay_queue", "3");
cvarManager->registerCvar("suitespot_current_training_code", "");
cvarManager->registerCvar("suitespot_auto_load_mode", "0");
cvarManager->registerCvar("suitespot_workshop_enabled", "1");
```

**INCORRECT examples**:
```cpp
// Missing prefix
cvarManager->registerCvar("enabled", "0");                    // ERROR
cvarManager->registerCvar("delay_queue", "3");                // ERROR

// Wrong prefix
cvarManager->registerCvar("ss_enabled", "0");                 // ERROR
cvarManager->registerCvar("suite_enabled", "0");              // ERROR

// Wrong case
cvarManager->registerCvar("SUITESPOT_enabled", "0");          // ERROR
cvarManager->registerCvar("SuiteSpot_Enabled", "0");          // ERROR
```

**Validation regex**: `^suitespot_[a-z][a-z0-9_]*$`

**Rules**:
- Prefix: Must be exactly `suitespot_` (lowercase)
- Name: Lowercase letters, numbers, underscores only
- First char after prefix: Must be a letter
- No consecutive underscores
- No trailing underscore

### 2. CVar Registration Pattern

Standard registration pattern in `SettingsSync.cpp`:

**CORRECT pattern**:
```cpp
void SettingsSync::RegisterAllCVars(std::shared_ptr<CVarManagerWrapper> cvarManager) {
    // Register with all parameters
    cvarManager->registerCvar(
        "suitespot_enabled",        // Name (with prefix)
        "0",                        // Default value
        "Enable SuiteSpot auto-load", // Description
        true,                       // Searchable
        true                        // Has minimum
        // min, hasMax, max, saveToCfg as needed
    );
    
    // Store reference for callback
    auto enabledCvar = cvarManager->getCvar("suitespot_enabled");
    if (enabledCvar) {
        enabledCvar.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            isEnabled = cvar.getBoolValue(); // Sync to local variable
            LOG("SuiteSpot enabled: {}", isEnabled);
        });
    }
}
```

**INCORRECT patterns**:
```cpp
// No description
cvarManager->registerCvar("suitespot_enabled", "0");  // Missing metadata

// No callback
cvarManager->registerCvar("suitespot_mode", "0", "Mode", true, true);
// ... but never adds onValueChanged callback

// Direct access without null check
cvarManager->getCvar("suitespot_enabled").setBoolValue(true); // CRASH!
```

### 3. Callback Implementation

Every CVar should have a callback to sync with local variables:

**CORRECT pattern**:
```cpp
class SettingsSync {
private:
    bool isEnabled = false;  // Local cache for fast access
    int delayQueue = 3;
    std::string currentCode = "";
    
public:
    void RegisterAllCVars(std::shared_ptr<CVarManagerWrapper> cvarManager) {
        // Register CVar
        auto enabledCvar = cvarManager->registerCvar("suitespot_enabled", "0");
        if (enabledCvar) {
            // Immediately sync initial value
            isEnabled = enabledCvar.getBoolValue();
            
            // Add callback for future changes
            enabledCvar.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
                isEnabled = cvar.getBoolValue(); // Keep local cache in sync
            });
        }
    }
    
    // Fast getter (uses local cache, no CVar lookup)
    bool IsEnabled() const { return isEnabled; }
};
```

**Why this pattern?**
- CVar lookups via `getCvar()` are slow (~microseconds)
- Render loop runs at 60+ FPS (every ~16ms)
- Direct access to local variables is fast (~nanoseconds)
- Callbacks keep cache synchronized

**INCORRECT patterns**:
```cpp
// NO LOCAL CACHE - Slow!
bool IsEnabled() {
    auto cvar = cvarManager->getCvar("suitespot_enabled");
    return cvar ? cvar.getBoolValue() : false; // Called 60+ times per second!
}

// NO CALLBACK - Stale data!
void RegisterCVar() {
    isEnabled = false;  // Set once, never updated
    cvarManager->registerCvar("suitespot_enabled", "0");
    // Missing: onValueChanged callback!
}
```

### 4. Null Safety

Always check CVar exists before accessing:

**CORRECT examples**:
```cpp
// Getting a CVar
auto cvar = cvarManager->getCvar("suitespot_enabled");
if (cvar) {
    bool value = cvar.getBoolValue();
    // Use value...
}

// Setting a CVar
auto cvar = cvarManager->getCvar("suitespot_mode");
if (cvar) {
    cvar.setIntValue(2);
}

// Helper function (from HelpersUI.h)
UI::Helpers::SetCVarSafely(cvarManager, "suitespot_enabled", true);
```

**INCORRECT examples**:
```cpp
// NO NULL CHECK - Will crash if CVar doesn't exist!
bool value = cvarManager->getCvar("suitespot_enabled").getBoolValue();

cvarManager->getCvar("suitespot_mode").setIntValue(2);

// Assuming CVar exists
auto cvar = cvarManager->getCvar("suitespot_invalid");
std::string value = cvar.getStringValue(); // CRASH!
```

### 5. Local Variable Synchronization

Verify local variables are kept in sync:

**Pattern to validate**:
1. CVar registered → local variable declared
2. Callback added → local variable updated
3. Initial value synced → callback handles future changes
4. Getter uses local variable → fast access in render loop

**Example audit**:
```cpp
// ✓ GOOD: Complete sync pattern
class Settings {
    bool enabled_;  // Local cache
    
    void Register() {
        auto cvar = cvarManager->registerCvar("suitespot_enabled", "0");
        if (cvar) {
            enabled_ = cvar.getBoolValue();  // Initial sync
            cvar.addOnValueChanged([this](std::string old, CVarWrapper c) {
                enabled_ = c.getBoolValue();  // Ongoing sync
            });
        }
    }
    
    bool IsEnabled() const { return enabled_; }  // Fast getter
};

// ✗ BAD: Missing local cache
class BadSettings {
    void Register() {
        cvarManager->registerCvar("suitespot_enabled", "0");
        // No local variable!
    }
    
    bool IsEnabled() {
        // Slow lookup every call!
        auto c = cvarManager->getCvar("suitespot_enabled");
        return c ? c.getBoolValue() : false;
    }
};
```

## Validation Checklist

For each CVar found in the codebase:

- [ ] Name uses `suitespot_` prefix
- [ ] Name follows lowercase convention
- [ ] Registered with description
- [ ] Has corresponding local variable in SettingsSync
- [ ] Callback added to sync local variable
- [ ] Initial value synced to local variable
- [ ] All access has null checks OR uses helper
- [ ] Getter uses local variable (not CVar lookup)

## Files to Check

**Primary file**: `SettingsSync.cpp` / `SettingsSync.h`
- All CVar registrations should be here
- All callbacks should be here
- All local cache variables should be here

**Secondary files**:
- `SettingsUI.cpp` - CVar access in UI
- `AutoLoadFeature.cpp` - Uses settings via SettingsSync
- `TrainingPackUI.cpp` - CVar usage for UI state
- `*.cpp` files - Ad-hoc CVar usage (audit for violations)

## Output

Returns JSON with validation results:

```json
{
  "cvarsFound": 15,
  "violations": {
    "namingErrors": [],
    "missingCallbacks": [],
    "nullSafetyIssues": [
      {
        "file": "TrainingPackUI.cpp",
        "line": 245,
        "issue": "CVar access without null check",
        "cvar": "suitespot_current_training_code"
      }
    ],
    "syncIssues": []
  },
  "warnings": [
    "CVar 'suitespot_workshop_enabled' has callback but no local cache variable"
  ],
  "summary": {
    "total": 15,
    "compliant": 13,
    "violations": 2
  },
  "autoFixesApplied": []
}
```

## Auto-Fixes

With `--auto-fix` enabled:

### Naming Violations

```cpp
// Before
cvarManager->registerCvar("enabled", "0");

// After (auto-fixed)
cvarManager->registerCvar("suitespot_enabled", "0");
```

### Null Safety

```cpp
// Before
bool val = cvarManager->getCvar("suitespot_enabled").getBoolValue();

// After (auto-fixed)
auto cvar = cvarManager->getCvar("suitespot_enabled");
bool val = cvar ? cvar.getBoolValue() : false;
```

## Common Issues

### CVar Not Found at Runtime

**Symptom**: `getCvar()` returns null, but code expects it

**Cause**: CVar name typo or not registered

**Solution**:
1. Verify exact spelling: `suitespot_enabled` not `suitespot_enable`
2. Check `SettingsSync::RegisterAllCVars()` includes registration
3. Ensure `onLoad()` calls `RegisterAllCVars()`

### Stale Settings Data

**Symptom**: Changing setting in F2 menu doesn't affect behavior

**Cause**: No callback to update local variable

**Solution**:
```cpp
// Add callback
auto cvar = cvarManager->getCvar("suitespot_mode");
if (cvar) {
    cvar.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        mode = cvar.getIntValue(); // Update local cache
    });
}
```

### Render Loop Performance

**Symptom**: FPS drops when checking settings

**Cause**: CVar lookups in render loop (60+ times per second)

**Solution**: Use local cache pattern (see section 3 above)

## Integration

This hook is automatically called by Claude Code when:

- Modifying `SettingsSync.cpp` or `SettingsSync.h`
- Adding new CVars anywhere in the codebase
- Refactoring settings-related code
- Before committing settings changes

Manual usage:

```bash
# Check all CVars in project
npx claude-flow hook cvar-validation --project bakkesmod

# Check specific file
npx claude-flow hook cvar-validation -f "SettingsSync.cpp"

# Auto-fix violations
npx claude-flow hook cvar-validation --auto-fix

# Just check naming
npx claude-flow hook cvar-validation --check-naming --check-registration=false
```

## See Also

- `SettingsSync.cpp` - CVar registration implementation
- `HelpersUI.cpp` - `SetCVarSafely()` helper function
- [CLAUDE.md](../../CLAUDE.md) - CVar naming convention
- [GEMINI.md](../../GEMINI.md) - CVar callback pattern
