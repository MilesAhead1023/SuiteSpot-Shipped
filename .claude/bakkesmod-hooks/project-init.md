# hook project-init (BakkesMod)

Execute project initialization validation for BakkesMod plugin development.

## Usage

```bash
npx claude-flow hook project-init --project bakkesmod [options]
```

## Options

- `--check-all` - Run all validation checks (default: true)
- `--check-sdk` - Verify BakkesMod SDK installation
- `--check-vs` - Verify Visual Studio 2022 installation
- `--check-vcpkg` - Verify vcpkg and dependencies
- `--check-powershell` - Check PowerShell execution policy
- `--check-structure` - Validate project directory structure
- `--auto-fix` - Attempt to fix common issues (default: false)

## Validation Checks

### 1. BakkesMod SDK Installation

Verifies BakkesMod is properly installed:

**Registry check**:
```powershell
# HKEY_CURRENT_USER\Software\BakkesMod\AppPath@BakkesModPath
# Expected value: C:\Users\<user>\AppData\Roaming\bakkesmod\bakkesmod
```

**File structure check**:
```
%APPDATA%\bakkesmod\bakkesmod\
├── bakkesmodsdk/
│   ├── include/
│   │   ├── bakkesmod/
│   │   │   ├── plugin/
│   │   │   │   ├── bakkesmodplugin.h
│   │   │   │   ├── pluginwindow.h
│   │   │   ├── wrappers/
│   │   │   │   ├── GameWrapper.h
│   │   │   │   ├── CVarManagerWrapper.h
│   │   │   │   └── ...
│   ├── lib/
│   │   └── pluginsdk.lib
│   └── bakkesmod-patch.exe
├── data/
└── plugins/
```

**Auto-fix**:
- Prompts to download/install BakkesMod if missing
- Creates registry entry if BakkesMod exists but registry is missing

### 2. Visual Studio 2022 Installation

Confirms VS 2022 with correct components:

**Required components**:
- MSVC v143 toolset (x64/x86)
- Windows 10/11 SDK
- C++ CMake tools (optional but recommended)
- C++ core features

**Check locations**:
```
C:\Program Files\Microsoft Visual Studio\2022\Community\
├── VC\Tools\MSVC\14.3x\
│   └── bin\Hostx64\x64\cl.exe
└── MSBuild\Current\Bin\MSBuild.exe
```

**Auto-fix**:
- Provides download link to VS 2022 installer
- Lists required workload: "Desktop development with C++"

### 3. vcpkg Dependencies

Verifies vcpkg installation and dependencies:

**Expected vcpkg path**: `C:\Users\<user>\vcpkg\`

**Required packages** (x64-windows):
- `nlohmann-json` - JSON parsing
- Other dependencies as needed

**Validation**:
```bash
# Check vcpkg is installed
vcpkg version

# Check nlohmann-json
vcpkg list | grep nlohmann-json
```

**Expected output**:
```
nlohmann-json:x64-windows    3.11.2    JSON for Modern C++
```

**Auto-fix**:
```bash
# If vcpkg missing
git clone https://github.com/Microsoft/vcpkg.git C:\Users\<user>\vcpkg
cd C:\Users\<user>\vcpkg
.\bootstrap-vcpkg.bat

# If nlohmann-json missing
.\vcpkg install nlohmann-json:x64-windows
```

### 4. PowerShell Execution Policy

Checks PowerShell can execute scripts:

**Required policy**: RemoteSigned or Bypass (for current user)

**Validation**:
```powershell
Get-ExecutionPolicy -Scope CurrentUser
```

**Expected**: `RemoteSigned` or `Bypass` or `Unrestricted`

**Auto-fix**:
```powershell
# Set execution policy for current user
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

**Note**: This is required for:
- `CompiledScripts/update_version.ps1` (pre-build)
- `Resources/SuitePackGrabber.ps1` (training pack scraping)
- Workshop map extraction (Expand-Archive)

### 5. Project Directory Structure

Validates required directories and files exist:

**Required structure**:
```
SuiteSpot/
├── .claude/
│   ├── bakkesmod-hooks/     # This hook set
│   ├── commands/
│   ├── helpers/
│   └── skills/
├── .claude-flow/
│   └── config.yaml
├── docs/
│   ├── architecture.md
│   └── bakkesmod-sdk-reference.md
├── IMGUI/
│   ├── imgui.cpp
│   ├── imgui.h
│   └── ...
├── CompiledScripts/
│   └── update_version.ps1
├── Resources/
│   ├── SuitePackGrabber.ps1
│   └── icons/
├── SuiteSpot.sln
├── SuiteSpot.vcxproj
├── BakkesMod.props
├── CLAUDE.md
├── GEMINI.md
├── version.h
├── pch.h
├── pch.cpp
├── logging.h
└── [source files]
```

**Auto-fix**:
- Creates missing directories
- Cannot auto-create missing essential files (requires manual intervention)

### 6. Project Configuration Validation

Checks `.vcxproj` and `BakkesMod.props`:

**SuiteSpot.vcxproj requirements**:
- Platform: x64 only
- Configuration: Release (and optionally Debug)
- PlatformToolset: v143
- LanguageStandard: stdcpp20
- CharacterSet: Unicode
- ConfigurationType: DynamicLibrary
- PrecompiledHeader: Use
- PrecompiledHeaderFile: pch.h

**BakkesMod.props requirements**:
- Imports BakkesMod SDK paths from registry
- AdditionalIncludeDirectories: `$(BakkesModPath)\bakkesmodsdk\include`
- AdditionalLibraryDirectories: `$(BakkesModPath)\bakkesmodsdk\lib`
- AdditionalDependencies: `pluginsdk.lib`

### 7. Build Script Validation

Verifies build scripts are present and valid:

**Pre-build**: `CompiledScripts/update_version.ps1`
- Auto-increments VERSION_BUILD
- Updates VERSION_BUILD_TIMESTAMP
- Modifies `version.h`

**Post-build**: Embedded in `.vcxproj`
- Runs bakkesmod-patch.exe
- Copies resources to %APPDATA%

**Validation**:
```bash
# Test update_version.ps1 can run
powershell.exe -ExecutionPolicy Bypass -NoProfile -NonInteractive -File CompiledScripts/update_version.ps1 -WhatIf
```

## Output

Returns JSON with validation results:

```json
{
  "ready": true,
  "environment": {
    "bakkesmodSdk": true,
    "sdkPath": "C:\\Users\\bmile\\AppData\\Roaming\\bakkesmod\\bakkesmod",
    "visualStudio2022": true,
    "vsPath": "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community",
    "vcpkg": true,
    "vcpkgPath": "C:\\Users\\bmile\\vcpkg",
    "powershellPolicy": "RemoteSigned"
  },
  "dependencies": {
    "nlohmann-json": true
  },
  "projectStructure": {
    "requiredFiles": true,
    "requiredDirs": true,
    "buildScripts": true
  },
  "configuration": {
    "vcxproj": "valid",
    "bakkesmodProps": "valid"
  },
  "warnings": [],
  "errors": [],
  "fixesApplied": []
}
```

## Common Setup Issues

### BakkesMod Not Installed

**Error**: Registry key not found or SDK missing

**Solution**:
1. Download BakkesMod from https://bakkesmod.com/
2. Run installer
3. Launch BakkesMod at least once (creates registry entries)
4. Re-run this hook: `npx claude-flow hook project-init --project bakkesmod`

### Visual Studio Wrong Version

**Error**: VS 2019 or 2017 detected instead of VS 2022

**Solution**:
1. Download VS 2022 Community: https://visualstudio.microsoft.com/
2. Select workload: "Desktop development with C++"
3. Include "MSVC v143 - VS 2022 C++ x64/x86 build tools"
4. Update `.vcxproj` if needed: `<PlatformToolset>v143</PlatformToolset>`

### vcpkg Not Found

**Error**: vcpkg directory doesn't exist

**Solution**:
```bash
# Install vcpkg
cd C:\Users\<user>
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install nlohmann-json:x64-windows

# Update .vcxproj AdditionalIncludeDirectories
# Add: C:\Users\<user>\vcpkg\installed\x64-windows\include
```

### PowerShell Execution Policy Blocked

**Error**: Cannot run PowerShell scripts

**Solution**:
```powershell
# As Administrator or CurrentUser scope
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Verify
Get-ExecutionPolicy -Scope CurrentUser
# Should show: RemoteSigned
```

### Wrong Project Structure

**Error**: Missing required directories or files

**Solution**:
- Clone correct repository structure
- Ensure all source files are present
- Verify IMGUI folder is complete
- Check Resources folder has PowerShell scripts

## Integration

This hook should be run:

- When first setting up the development environment
- After reinstalling BakkesMod or Visual Studio
- After cloning the repository on a new machine
- When onboarding new developers
- As part of CI/CD environment validation

Manual usage:

```bash
# Full validation with auto-fix
npx claude-flow hook project-init --project bakkesmod --check-all --auto-fix

# Specific checks
npx claude-flow hook project-init --check-sdk --check-vs

# Silent check (for scripts)
npx claude-flow hook project-init --project bakkesmod --quiet
```

## Workflow Example

```bash
# 1. Clone repository
git clone https://github.com/MilesAhead1023/SuiteSpot-Shipped.git
cd SuiteSpot-Shipped

# 2. Run project initialization check
npx claude-flow hook project-init --project bakkesmod --auto-fix

# 3. If all checks pass, build
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64

# 4. Test plugin in BakkesMod
# (Launch Rocket League with BakkesMod)
```

## See Also

- `hook pre-build` - Pre-compilation validation
- [CLAUDE.md](../../CLAUDE.md) - Build requirements
- [BakkesMod.props](../../BakkesMod.props) - SDK integration
- [SuiteSpot.vcxproj](../../SuiteSpot.vcxproj) - Project configuration
