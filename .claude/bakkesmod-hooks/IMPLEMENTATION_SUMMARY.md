# BakkesMod Claude Flow Hooks - Implementation Summary

## Overview

This document summarizes the BakkesMod-specific Claude Flow hooks designed for the SuiteSpot plugin development environment.

## What Was Created

### 1. Hook Documentation (`.claude/bakkesmod-hooks/`)

Six comprehensive hook specifications tailored to BakkesMod plugin development:

#### **README.md**
- Overview of all BakkesMod hooks
- Integration instructions
- Environment requirements
- Usage examples

#### **pre-build.md** (Build Environment Validation)
- Checks BakkesMod SDK installation (registry validation)
- Validates Visual Studio 2022 toolchain
- Verifies vcpkg dependencies (nlohmann-json)
- Ensures version.h exists and is valid
- Validates project configuration (.vcxproj)

#### **post-build.md** (Build Output Validation)
- Verifies DLL was built successfully
- Checks bakkesmod-patch.exe ran correctly
- Validates resource deployment to %APPDATA%
- Logs build metrics (size, time, warnings)
- Optional plugin load testing

#### **pre-edit.md** (Code Quality Pre-Checks)
- C++20 syntax validation
- ImGui v1.75 API verification (against `IMGUI/imgui.h`)
- Thread safety pattern validation (mutex usage)
- CVar naming convention enforcement (`suitespot_` prefix)
- Include guard validation (`#pragma once`)
- Null safety checks (CVar access)
- Deferred execution pattern validation (`SetTimeout`)
- PowerShell script validation (no `-i` flag)

#### **post-edit.md** (Code Formatting & Validation)
- C++ code formatting (K&R style, tabs)
- Include order validation (pch.h first)
- Thread safety audit (mutex protection)
- Memory safety checks (smart pointers, RAII)
- API usage validation (BakkesMod, ImGui)
- Logging consistency (LOG macro)
- CVar callback validation
- Documentation quality checks

#### **project-init.md** (Environment Setup)
- BakkesMod SDK installation check
- Visual Studio 2022 validation
- vcpkg dependency verification
- PowerShell execution policy check
- Directory structure validation
- Build script validation
- Auto-fix capabilities for common issues

#### **cvar-validation.md** (CVar-Specific Checks)
- Naming convention enforcement (`suitespot_[a-z][a-z0-9_]*`)
- Registration pattern validation
- Callback implementation checks
- Null safety verification
- Local variable synchronization validation
- Auto-fix for naming violations

### 2. Context Documentation

#### **.claude/bakkesmod-context.md**
Comprehensive BakkesMod development context including:
- Critical "don't do" and "always do" rules
- Environment and platform requirements
- Build process documentation
- Essential code patterns (thread safety, deferred execution, CVar, null safety)
- ImGui v1.75 API reference
- Project structure
- Common tasks and debugging guides
- Hook integration workflow

### 3. Configuration Updates

#### **.claude-flow/config.yaml**
Enhanced configuration with:
- Project-specific settings (bakkesmod-plugin, cpp20, windows-x64)
- Custom hook paths (`.claude/bakkesmod-hooks`)
- Detailed hook configuration for each hook type
- BakkesMod-specific pattern enforcement:
  - Deferred execution (min 0.5s delay)
  - Thread safety (mutex/atomic requirements)
  - CVar naming (prefix, case, regex)
  - ImGui version (v1.75 with API validation)
  - Logging (LOG macro preference)

## Hook Integration Flow

```
┌─────────────────────────────────────────────────────────────┐
│                   Development Workflow                      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  project-init    │ (First-time setup)
                    │  - SDK check     │
                    │  - VS2022 check  │
                    │  - vcpkg check   │
                    └──────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────────────┐
│                    Edit Cycle                                 │
├───────────────────────────────────────────────────────────────┤
│  pre-edit → Edit File → post-edit → cvar-validation (if CVars)│
│  - Syntax     - Changes  - Format    - Naming                 │
│  - ImGui API  - Logic    - Includes  - Registration           │
│  - Thread     - Patterns - Audit     - Callbacks              │
│  - CVar names            - Memory    - Null safety            │
└───────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────────────┐
│                     Build Cycle                               │
├───────────────────────────────────────────────────────────────┤
│       pre-build → msbuild → post-build                        │
│       - SDK check  - Compile  - DLL check                     │
│       - vcxproj    - Link     - Patch verify                  │
│       - version.h  - Patch    - Resource deploy               │
│                               - Metrics                       │
└───────────────────────────────────────────────────────────────┘
```

## Key Features

### 1. BakkesMod-Specific Validations

- **SDK Integration**: Registry-based BakkesMod SDK detection
- **Platform Constraints**: Windows x64 only, MSVC v143
- **Version Locking**: ImGui v1.75 API enforcement
- **Build Tools**: bakkesmod-patch.exe validation

### 2. Code Safety Enforcement

- **Thread Safety**: Mandatory mutex protection for shared data
- **Deferred Execution**: Enforces SetTimeout for post-match loading
- **Null Safety**: CVar access must have null checks
- **Memory Safety**: Promotes smart pointers and RAII

### 3. Performance Patterns

- **CVar Caching**: Enforces local variable caching pattern
- **Virtual Scrolling**: Validates ImGuiListClipper usage
- **Render Loop**: No blocking operations in UI code

### 4. Auto-Fix Capabilities

- **CVar Naming**: Auto-corrects missing `suitespot_` prefix
- **Include Order**: Reorders to pch.h first
- **Null Checks**: Adds CVar null safety checks
- **Code Formatting**: Applies K&R style, tab indentation

## Usage Examples

### Manual Hook Execution

```bash
# Environment setup
npx claude-flow hook project-init --project bakkesmod --auto-fix

# Before editing C++ file
npx claude-flow hook pre-edit -f "TrainingPackUI.cpp" --project bakkesmod

# After editing
npx claude-flow hook post-edit -f "TrainingPackUI.cpp" --auto-format

# CVar-specific validation
npx claude-flow hook cvar-validation --auto-fix

# Before building
npx claude-flow hook pre-build --project bakkesmod

# After building
npx claude-flow hook post-build --verify-dll --check-patch
```

### Automatic Execution

Hooks are automatically triggered by Claude Code when `hooks.autoExecute: true` in config:

- **Edit/MultiEdit**: Triggers `pre-edit` and `post-edit`
- **Build operations**: Triggers `pre-build` and `post-build`
- **CVar modifications**: Triggers `cvar-validation`
- **Project initialization**: Triggers `project-init`

## Patterns Enforced

### 1. Deferred Execution (CRITICAL)
```cpp
// ENFORCED: SetTimeout with min 0.5s delay
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    cvarManager->executeCommand("load_training CODE");
}, 0.5f);
```

### 2. Thread Safety
```cpp
// ENFORCED: Mutex protection
std::lock_guard<std::mutex> lock(dataMutex_);
sharedData_.push_back(entry);
```

### 3. CVar Pattern
```cpp
// ENFORCED: Prefix, registration, callback, local cache
auto cvar = cvarManager->registerCvar("suitespot_enabled", "0");
if (cvar) {
    isEnabled_ = cvar.getBoolValue();
    cvar.addOnValueChanged([this](auto old, auto c) {
        isEnabled_ = c.getBoolValue();
    });
}
```

### 4. Null Safety
```cpp
// ENFORCED: Null check before access
auto cvar = cvarManager->getCvar("suitespot_enabled");
if (cvar) {
    bool value = cvar.getBoolValue();
}
```

### 5. ImGui API
```cpp
// ENFORCED: v1.75 compatible API only
ImGui::BeginChild(const char* str_id, const ImVec2& size, bool border);
// NOT: ImGui::BeginChild(ImGuiID id, ...) // Doesn't exist in v1.75
```

## Files Created

```
.claude/
├── bakkesmod-hooks/
│   ├── README.md              (3,182 bytes)
│   ├── pre-build.md           (3,746 bytes)
│   ├── post-build.md          (4,604 bytes)
│   ├── pre-edit.md            (6,958 bytes)
│   ├── post-edit.md           (8,465 bytes)
│   ├── project-init.md        (8,703 bytes)
│   └── cvar-validation.md     (10,537 bytes)
├── bakkesmod-context.md       (11,825 bytes)
└── [existing files]

.claude-flow/
├── config.yaml                (Updated with BakkesMod hooks)
└── [existing files]
```

**Total**: 7 hook specs + 1 context doc + 1 config update = **9 files modified/created**

## Benefits

### For Development
- **Consistent Code Quality**: Automated enforcement of project patterns
- **Early Error Detection**: Catches issues before compilation
- **Fast Iteration**: Auto-formatting and auto-fixes reduce manual work
- **Knowledge Preservation**: Hooks encode project-specific expertise

### For Onboarding
- **Environment Validation**: `project-init` ensures correct setup
- **Pattern Learning**: Hooks teach BakkesMod best practices
- **Self-Documenting**: Hook docs serve as reference material

### For Maintenance
- **Regression Prevention**: Hooks prevent reintroduction of known issues
- **Consistency**: All code follows same patterns
- **Safety**: Thread safety and null checks enforced

## Next Steps

### For Developers

1. **Run project-init**: Validate environment setup
   ```bash
   npx claude-flow hook project-init --project bakkesmod --check-all
   ```

2. **Enable auto-execution**: Hooks will run automatically with Claude Code

3. **Review context**: Read `.claude/bakkesmod-context.md` for patterns

4. **Iterate**: Edit → auto-hooks → build → test

### For CI/CD

1. **Add to build pipeline**:
   ```yaml
   - name: Validate environment
     run: npx claude-flow hook project-init --project bakkesmod
   
   - name: Pre-build checks
     run: npx claude-flow hook pre-build --project bakkesmod
   
   - name: Build
     run: msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
   
   - name: Post-build validation
     run: npx claude-flow hook post-build --verify-dll --check-patch
   ```

### For Customization

To modify hook behavior, edit `.claude-flow/config.yaml`:

```yaml
hooks:
  bakkesmod:
    preEdit:
      checkThreadSafety: false  # Disable specific check
    postBuild:
      testLoad: true            # Enable plugin load test
    projectInit:
      autoFix: true             # Auto-fix common issues
```

## Support

For issues or questions:
- Review hook documentation in `.claude/bakkesmod-hooks/`
- Check context guide in `.claude/bakkesmod-context.md`
- Refer to main docs: `CLAUDE.md` and `GEMINI.md`

## Version

- **Hook Version**: 1.0.0
- **Created**: 2026-01-31
- **Last Updated**: 2026-01-31
- **Claude Flow Version**: 3.0.0
- **Project**: SuiteSpot BakkesMod Plugin
