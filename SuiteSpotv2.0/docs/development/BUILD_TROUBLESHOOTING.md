# Build Troubleshooting

## Local Build

**DLL not found:** Verify output path in `.vcxproj`: `<OutDir>$(SolutionDir)plugins\</OutDir>`

**MSBuild not found:** Use full path:
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' `
  SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

**SDK not found:** Check `BakkesModSDK/include/bakkesmod/` exists. Verify `BakkesMod.props` includes correct paths.

**Platform error:** Use exact case: `/p:Platform=x64` (not `x86`)

**Clean rebuild:**
```powershell
Remove-Item SuiteSpotv2.0\build -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item SuiteSpotv2.0\plugins -Recurse -Force -ErrorAction SilentlyContinue
msbuild SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

## CI/CD Workflow

**Workflow not triggering:**
- Verify `.github/workflows/build.yml` syntax
- Check branch is `master` or `main`
- Confirm push triggers on correct branches

**Build fails in CI/CD:**
- Check workflow logs: GitHub Actions tab
- Verify solution file at `SuiteSpotv2.0/SuiteSpot.sln`
- Check BakkesMod SDK path configuration

**No artifacts generated:**
- Verify DLL created at `SuiteSpotv2.0/plugins/SuiteSpot.dll`
- Check build succeeded (not just compiled)
- Review "Verify build output" step in workflow

**Release not created:**
- Tag format must be `v*.*.*` (e.g., `v1.0.1`)
- Verify tag pushed: `git push origin v1.0.1`
- Check workflow runs successfully before release step

## Debugging

**Verbose output:**
```bash
msbuild SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64 /v:detailed
```

**Check file structure:**
```powershell
Test-Path "SuiteSpotv2.0/SuiteSpot.sln"
Test-Path "SuiteSpotv2.0/SuiteSpot.vcxproj"
Test-Path ".github/workflows/build.yml"
Test-Path "BakkesModSDK/include/bakkesmod/"
```

**Workflow logs:** GitHub → Actions tab → Failed run → Expand steps
