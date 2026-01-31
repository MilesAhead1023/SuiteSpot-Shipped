# BakkesMod Hooks - Quick Reference Card

## 🚀 Hook Execution Order

```
┌─────────────┐
│ FIRST TIME  │ → project-init (validate environment)
└─────────────┘
       ↓
┌─────────────────────────────────────────┐
│              EDIT CYCLE                 │
├─────────────────────────────────────────┤
│ 1. pre-edit    → Validate before edit  │
│ 2. [EDIT FILE] → Make changes          │
│ 3. post-edit   → Format & audit        │
│ 4. cvar-validation → If CVars modified │
└─────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────┐
│             BUILD CYCLE                 │
├─────────────────────────────────────────┤
│ 1. pre-build   → Check environment     │
│ 2. [BUILD]     → msbuild compile       │
│ 3. post-build  → Verify output         │
└─────────────────────────────────────────┘
```

## 📋 Hook Commands

### Environment Setup
```bash
# First-time setup (with auto-fix)
npx claude-flow hook project-init --project bakkesmod --auto-fix
```

### Before Editing
```bash
# Validate C++ file before editing
npx claude-flow hook pre-edit -f "MyFile.cpp" --project bakkesmod
```

### After Editing
```bash
# Format and validate after editing
npx claude-flow hook post-edit -f "MyFile.cpp" --auto-format
```

### CVar Changes
```bash
# Validate CVar naming and patterns (with auto-fix)
npx claude-flow hook cvar-validation --auto-fix
```

### Before Building
```bash
# Check build environment
npx claude-flow hook pre-build --project bakkesmod
```

### After Building
```bash
# Verify DLL and deployment
npx claude-flow hook post-build --verify-dll --check-patch
```

## ⚡ Critical Rules

### NEVER Do These
- ❌ Load maps without `SetTimeout()` (min 0.5s delay)
- ❌ Access CVars without null check
- ❌ Modify shared data without mutex
- ❌ Use ImGui APIs not in v1.75
- ❌ Name CVars without `suitespot_` prefix
- ❌ Use `PowerShell -i` flag
- ❌ Block render thread with long operations

### ALWAYS Do These
- ✅ Defer post-match loading: `SetTimeout(..., 0.5f)`
- ✅ Check CVar exists: `if (cvar) { ... }`
- ✅ Protect shared data: `std::lock_guard<std::mutex>`
- ✅ Verify ImGui API: Check `IMGUI/imgui.h`
- ✅ Prefix CVars: `suitespot_enabled`
- ✅ Log with macro: `LOG("message")`
- ✅ Include pch.h first in .cpp files

## 🎯 Common Patterns

### Thread Safety
```cpp
std::mutex dataMutex_;
std::vector<Data> shared_;

void Access() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    shared_.push_back(...);  // Safe
}
```

### Deferred Execution
```cpp
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    cvarManager->executeCommand("load_training CODE");
}, 0.5f);  // REQUIRED delay
```

### CVar Pattern
```cpp
// Register
auto cvar = cvarManager->registerCvar("suitespot_enabled", "0");
if (cvar) {
    enabled_ = cvar.getBoolValue();  // Cache
    cvar.addOnValueChanged([this](auto old, auto c) {
        enabled_ = c.getBoolValue();  // Sync
    });
}

// Use (fast)
bool IsEnabled() const { return enabled_; }
```

### Null Safety
```cpp
auto cvar = cvarManager->getCvar("suitespot_enabled");
if (cvar) {
    cvar.setBoolValue(true);
}
```

## 📁 Key Files

- `.claude/bakkesmod-hooks/` - Hook specifications
- `.claude/bakkesmod-context.md` - Development context
- `.claude-flow/config.yaml` - Hook configuration
- `CLAUDE.md` - Quick reference
- `GEMINI.md` - Technical reference
- `IMGUI/imgui.h` - ImGui v1.75 API reference

## 🔧 Configuration

Edit `.claude-flow/config.yaml` to customize:

```yaml
hooks:
  bakkesmod:
    preEdit:
      checkThreadSafety: true   # Enable/disable checks
    postEdit:
      validateBuild: false      # Set true for strict mode
    projectInit:
      autoFix: true             # Auto-fix common issues
```

## 🐛 Common Issues

### SDK Not Found
```bash
# Fix: Install BakkesMod, launch once, then:
npx claude-flow hook project-init --project bakkesmod --check-sdk
```

### CVar Naming Error
```bash
# Fix: Auto-correct to suitespot_ prefix:
npx claude-flow hook cvar-validation --auto-fix
```

### ImGui API Mismatch
```bash
# Fix: Check IMGUI/imgui.h for v1.75 signature
# Hooks will validate automatically
```

### Build Fails After Edit
```bash
# Fix: Validate before building:
npx claude-flow hook post-edit -f "File.cpp" --validate-build
```

## 📊 Hook Outputs

All hooks return JSON with:
- `continue`: true/false (should proceed?)
- `warnings`: []
- `errors`: []
- Hook-specific data

Example:
```json
{
  "continue": true,
  "validations": {
    "syntax": "pass",
    "threadSafety": "pass"
  },
  "warnings": ["Line 42: Consider mutex protection"],
  "errors": []
}
```

## 🎓 Learn More

1. Read `.claude/bakkesmod-context.md` for patterns
2. Review `.claude/bakkesmod-hooks/README.md` for details
3. Check individual hook docs for specific validations
4. See `IMPLEMENTATION_SUMMARY.md` for overview

## 🔄 Auto-Execution

Hooks run automatically when `hooks.autoExecute: true`:
- Edit/MultiEdit → pre-edit + post-edit
- Build → pre-build + post-build
- CVar changes → cvar-validation

Set in `.claude-flow/config.yaml`

## 📞 Support

For issues:
1. Review hook documentation
2. Check `bakkesmod-context.md`
3. Verify config in `.claude-flow/config.yaml`
4. Ensure environment setup with `project-init`

---

**Version**: 1.0.0 | **Updated**: 2026-01-31 | **Project**: SuiteSpot BakkesMod Plugin
