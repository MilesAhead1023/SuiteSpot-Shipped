# hook post-build (BakkesMod)

Execute post-build validation and deployment for BakkesMod plugins.

## Usage

```bash
npx claude-flow hook post-build --project bakkesmod [options]
```

## Options

- `--verify-dll` - Check that DLL was built successfully (default: true)
- `--check-patch` - Verify bakkesmod-patch.exe ran (default: true)
- `--validate-resources` - Ensure resources copied to %APPDATA% (default: true)
- `--test-load` - Attempt to validate plugin loads in BakkesMod (default: false)
- `--log-metrics` - Record build metrics (time, size, warnings) (default: true)

## Validation Checks

### 1. DLL Build Verification

Confirms plugin DLL was created:

**Expected path**: `plugins/SuiteSpot.dll`

**Checks**:
- File exists
- File size > 0 bytes
- Created/modified timestamp is recent (within last 5 minutes)
- Platform is x64

### 2. BakkesMod Patch Verification

Verifies DLL was patched by `bakkesmod-patch.exe`:

```bash
# Post-build event from .vcxproj:
# bakkesmod-patch.exe "$(TargetPath)"
```

**Validation**:
- Check DLL headers for BakkesMod signatures
- Verify no corruption after patching
- Confirm patched timestamp matches build time

### 3. Resource Deployment

Checks resource files were copied to BakkesMod data directory:

**Target**: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\`

**Expected structure**:
```
SuiteSpot/
├── TrainingSuite/
│   ├── training_packs.json
│   ├── pack_usage_stats.json
│   └── workshop_loader_config.json
└── Resources/
    ├── SuitePackGrabber.ps1
    └── icons/
```

**Validation**:
- All directories exist
- PowerShell scripts are present
- Icon files are accessible

### 4. Plugin Load Test (Optional)

If `--test-load` is enabled, attempts to validate plugin:

```bash
# This requires BakkesMod to be running
# Checks BakkesMod console output for load errors
```

**Note**: This is typically skipped in CI/CD environments.

### 5. Build Metrics Logging

Records build statistics:

```json
{
  "buildTime": "2024-01-31T13:22:00Z",
  "dllSize": 245760,
  "buildDuration": "12.3s",
  "warningCount": 0,
  "configuration": "Release",
  "platform": "x64",
  "versionBuilt": "1.0.0.42"
}
```

## Post-Build Script Execution

Verifies post-build event completed successfully:

```batch
@echo off
setlocal
set "BM=%AppData%\bakkesmod\bakkesmod"
set "PATCH=%BM%\bakkesmodsdk\bakkesmod-patch.exe"
set "DLL=$(TargetPath)"

if not exist "%PATCH%" (
    echo Error: bakkesmod-patch.exe not found
    exit /b 1
)

"%PATCH%" "%DLL%"
if errorlevel 1 (
    echo Error: Failed to patch DLL
    exit /b 1
)

# Copy resources...
xcopy /Y /I "Resources\*" "%BM%\data\SuiteSpot\Resources\"
```

## Output

Returns JSON with validation results:

```json
{
  "success": true,
  "dllExists": true,
  "dllPath": "plugins/SuiteSpot.dll",
  "dllSize": 245760,
  "patchVerified": true,
  "resourcesCopied": true,
  "resourcePath": "C:\\Users\\bmile\\AppData\\Roaming\\bakkesmod\\bakkesmod\\data\\SuiteSpot",
  "metrics": {
    "buildTime": "12.3s",
    "warnings": 0
  },
  "warnings": [],
  "errors": []
}
```

## Common Issues

### bakkesmod-patch.exe Not Found

**Error**: Patch executable missing

**Solution**:
1. Ensure BakkesMod is installed
2. Check SDK path: `%APPDATA%\bakkesmod\bakkesmod\bakkesmodsdk\bakkesmod-patch.exe`
3. Reinstall BakkesMod if necessary

### Resource Copy Failed

**Error**: xcopy command failed or files missing

**Solution**:
1. Verify source files exist in `Resources/` directory
2. Check write permissions to `%APPDATA%`
3. Manually create target directories if needed

### DLL Size Suspicious

**Warning**: DLL is significantly larger/smaller than expected

**Investigation**:
- Check for debug symbols (should be stripped in Release)
- Verify optimization settings
- Compare with previous builds

## Integration

This hook is automatically called by Claude Code when:

- After successful compilation
- Following msbuild or Visual Studio build
- Before testing or deployment

Manual usage:

```bash
# After building
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64

# Validate build output
npx claude-flow hook post-build --project bakkesmod --verify-dll --check-patch
```

## Deployment Workflow

1. **Build** → DLL created in `plugins/`
2. **Patch** → BakkesMod-specific headers injected
3. **Copy Resources** → Files deployed to %APPDATA%
4. **Validate** → This hook verifies all steps
5. **Load** → Plugin ready for BakkesMod

## See Also

- `hook pre-build` - Pre-build validation
- [SuiteSpot.vcxproj](../../SuiteSpot.vcxproj) - Build configuration
- [CLAUDE.md](../../CLAUDE.md) - Build process documentation
