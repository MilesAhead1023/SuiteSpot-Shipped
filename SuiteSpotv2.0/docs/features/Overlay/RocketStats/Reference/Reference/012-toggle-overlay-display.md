# Implementation Spec: Toggle overlay display

**Capability ID**: 12 | **Slug**: toggle-overlay-display

## Overview

This capability allows users to configure toggle overlay display via a configuration variable (CVar). The setting persists across plugin sessions through BakkesMod's CVar system and the data/rocketstats.json configuration file.

## Control Surfaces

- **CVar Name**: `rs_toggle_overlay_display`
- **Type**: Boolean / Integer / Float / String
- **Persistence**: BakkesMod CVar system + JSON file
- **Evidence**: RocketStats.cpp:498 (CVar rs_disp_overlay)

## Code Path Trace

1. **Registration** -> onInit() registers CVar with default value
2. **User Input** -> Settings menu in WindowManagement.cpp updates CVar
3. **Callback Fire** -> addOnValueChanged callback triggered
4. **Refresh** -> RefreshTheme() or UpdateFiles() applies change
5. **Persistence** -> WriteConfig() saves to data/rocketstats.json

```
cvarManager->registerCvar("rs_XXX", default_value, help_text)
    |
    v
addOnValueChanged(RefreshTheme or UpdateFiles)
    |
    v
WriteConfig() -> JSON persistence
```

## Data and State

**CVar Registration**:
```cpp
cvarManager->registerCvar("rs_toggle_overlay_display", "default_value",
    "Help text for settings menu", true, true)
    .addOnValueChanged([this](auto old, CVarWrapper now) {
        RefreshTheme(old, now);
    });
```

**JSON Persistence**:
```json
{
    "rs_toggle_overlay_display": "stored_value"
}
```

**Access Pattern**:
```cpp
auto value = cvarManager->getCvar("rs_toggle_overlay_display").getValue<Type>();
```

## Threading and Safety

**Thread Safety**:
- CVar operations are atomic via BakkesMod
- No manual synchronization needed
- Safe to read/write from game thread
- JSON writes are blocking but infrequent

**Race Conditions**: None (BakkesMod guarantees atomicity)

## SuiteSpot Implementation Guide

### Step 1: Define CVar Name and Type
- Choose descriptive name with rs_ prefix
- Determine type (bool, int, float, string)

### Step 2: Register in onInit()
- Call cvarManager->registerCvar()
- Set appropriate default value
- Add help text for UI display

### Step 3: Register Callback
- Implement callback handler (RefreshTheme or UpdateFiles)
- Connect via addOnValueChanged()

### Step 4: Add Settings UI Widget
- Add ImGui control in WindowManagement.cpp::RenderSettings()
- Link widget to CVar value

### Step 5: Implement JSON Persistence
- Add to ReadConfig() to load from JSON
- Add to WriteConfig() to save to JSON

### Step 6: Test and Validate
- Change setting, verify callback fires
- Reload plugin, verify persistence
- Check JSON file for saved value

## Failure Modes

| Failure | Symptoms | Solution |
|---------|----------|----------|
| Not persistent | Resets on reload | Add JSON Read/Write handlers |
| Callback not firing | Change has no effect | Verify addOnValueChanged registration |
| Not in menu | Can't adjust via UI | Add ImGui widget to RenderSettings |
| Type mismatch | Conversion errors | Check CVar type consistency |
| Help text missing | No tooltip in menu | Add help text in registerCvar |

## Testing Checklist

- [ ] CVar registers without errors
- [ ] Default value appears in settings
- [ ] Callback fires on change
- [ ] Change applies immediately
- [ ] Value persists across reload
- [ ] JSON file contains value
- [ ] Settings menu widget works
- [ ] Multiple changes work correctly

## Related Capabilities

- Cap #131: Write configuration to JSON
- Cap #132: Read configuration from JSON
- Cap #162-168: Other CVar configuration options
