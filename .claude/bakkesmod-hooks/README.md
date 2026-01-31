# BakkesMod-Specific Claude Flow Hooks

## Overview

This directory contains Claude Flow hooks specifically designed for BakkesMod plugin development. These hooks automate validation, formatting, and safety checks tailored to the BakkesMod/Rocket League plugin environment.

## Hook Types

### Pre-Operation Hooks

#### `pre-build.md`
Validates build environment before compilation:
- Checks for BakkesMod SDK registry entry
- Validates vcxproj configuration
- Ensures version.h exists
- Verifies Visual Studio 2022 toolchain

#### `pre-edit.md`
Validates before editing C++ files:
- C++20 syntax validation
- ImGui API version check (v1.75)
- Thread safety pattern validation
- CVar naming convention check (suitespot_ prefix)
- Include guard validation

### Post-Operation Hooks

#### `post-build.md`
Post-build validation and deployment:
- Verifies DLL was patched with bakkesmod-patch.exe
- Checks resource copying to %APPDATA%
- Validates plugin load in BakkesMod
- Logs build metrics

#### `post-edit.md`
Post-edit formatting and validation:
- C++ code formatting (clang-format style)
- Include order validation
- Thread safety audit
- Mutex usage verification
- SetTimeout pattern validation

### Project-Specific Hooks

#### `project-init.md`
Initial project setup validation:
- BakkesMod SDK installation check
- vcpkg dependency verification (nlohmann/json)
- Visual Studio 2022 installation
- PowerShell execution policy check
- Directory structure validation

#### `cvar-validation.md`
CVar-specific validation:
- Ensures all CVars use suitespot_ prefix
- Validates CVar callback patterns
- Checks for null-safety in CVar access
- Verifies CVar synchronization with local variables

## BakkesMod-Specific Patterns

### Critical Patterns to Enforce

1. **Deferred Execution**: Always use `gameWrapper->SetTimeout()` for post-match loading
2. **Thread Safety**: Protect shared data structures with `std::mutex`
3. **CVar Null Checks**: Always verify CVar exists before accessing
4. **ImGui API**: Only use verified v1.75 APIs
5. **Naming Convention**: All CVars must use `suitespot_` prefix

### Code Quality Checks

- No blocking operations on render thread
- Proper mutex usage for concurrent access
- PowerShell script validation (no -i flag)
- Safe file path construction
- Memory leak prevention

## Integration with Claude Flow

These hooks integrate with the main Claude Flow system:

```yaml
# .claude-flow/config.yaml
hooks:
  enabled: true
  autoExecute: true
  customPaths:
    - .claude/bakkesmod-hooks
```

## Usage

Hooks are automatically triggered by Claude Code operations:

- **Edit/MultiEdit**: Triggers pre-edit and post-edit hooks
- **Build**: Triggers pre-build and post-build hooks
- **Project initialization**: Triggers project-init hook
- **CVar modifications**: Triggers cvar-validation hook

## Environment Requirements

- Windows 10/11 x64
- Visual Studio 2022 with MSVC v143
- BakkesMod installed
- vcpkg with nlohmann-json
- PowerShell 5.1+

## See Also

- [CLAUDE.md](../../CLAUDE.md) - Main project guidelines
- [GEMINI.md](../../GEMINI.md) - Technical reference
- [architecture.md](../../docs/architecture.md) - Architecture documentation
