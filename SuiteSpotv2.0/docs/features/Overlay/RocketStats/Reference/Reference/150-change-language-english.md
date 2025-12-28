# Implementation Spec: Change plugin language to English (INT)

**Capability ID**: 150 | **Slug**: change-language-english

## Overview

This capability manages plugin language selection. The plugin supports English (INT) and French (FRA) with translations embedded in resources.

## Control Surfaces

- **Languages**: INT (English), FRA (French)
- **Resource**: Embedded JSON files
- **Persistence**: BakkesMod settings
- **Evidence**: LangManagement.cpp::ChangeLang (IDB_LANG_INT), RocketStats.cpp:342

## Code Path Trace

1. **Detection** -> Detect OS/BakkesMod language
2. **Load** -> Load matching language resource
3. **Override** -> User can change via command
4. **Substitution** -> All text uses language strings
5. **Persist** -> Save choice to settings

## Data and State

**Language Files**:
```
Resources/Languages/
├── INT.json  (English)
└── FRA.json  (French)
```

**Usage**:
```cpp
std::string text = GetLang(STRING_ID);  // Returns localized string
```

## Threading and Safety

- Strings read-only after load
- No mutations
- Safe multi-threaded access
- Settings atomic via BakkesMod

## SuiteSpot Implementation Guide

### Step 1: Detect Language
- Read BakkesMod UI setting
- Default to INT if unavailable

### Step 2: Load Resource
- Extract JSON from embedded resource
- Parse language map
- Cache for lookups

### Step 3: Implement Switching
- Add ChangeLang() function
- Load new language resource
- Refresh UI elements
- Persist choice

### Step 4: Use Strings
- Never hardcode text
- Always use GetLang(ID)
- Provide English fallback

### Step 5: Test Both
- Test in INT
- Test in FRA
- Check all UI text

### Step 6: Verify Persistence
- Change language, reload
- Verify choice persists

## Failure Modes

| Failure | Symptoms | Solution |
|---------|----------|----------|
| Not switching | Stays in default | Implement ChangeLang |
| Missing translations | English text in FRA | Add to translation file |
| File load error | Language blank | Check JSON syntax |
| Not persistent | Resets on reload | Add persistence handler |

## Testing Checklist

- [ ] INT loads correctly
- [ ] FRA loads correctly
- [ ] All text translates
- [ ] Menu displays correctly
- [ ] Overlay translates
- [ ] Switch works immediately
- [ ] Persists across reload
- [ ] All strings translated
- [ ] No English leaks
- [ ] Resources valid

## Related Capabilities

- Cap #155: Load language resource
- Cap #156: Migrate config
