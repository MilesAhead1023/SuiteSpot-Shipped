# Development Quick Reference

**Build locally:**
```bash
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

**Output:** `SuiteSpotv2.0\plugins\SuiteSpot.dll`

**Key files:**
- Plugin logic: `SuiteSpot.h/cpp`
- UI: `Source.cpp`
- Maps: `MapList.cpp`
- Event hooks registered in `SuiteSpot::LoadHooks()`

**Patterns to follow (see CLAUDE.md):**
- Thread safety: Use `gameWrapper->SetTimeout()` for game state
- Never store wrappers across frames
- Register CVars only in `onLoad()`
- Call `SaveTrainingMaps()` after modifying `RLTraining`

**Git workflow:**
```bash
git checkout -b feature/name
# ... make changes ...
git commit -m "description"
git push origin feature/name
```

**Release (automated on tag):**
```bash
git tag v1.0.1 -m "v1.0.1"
git push origin v1.0.1
```

**Workflow:** `.github/workflows/build.yml` - auto-builds on push, creates releases on tags

**Troubleshoot:** See `BUILD_TROUBLESHOOTING.md` or check workflow logs in GitHub Actions
