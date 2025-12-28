# Release Procedure

**Automated on git tag:** `git tag v*.*.* && git push origin <tag>`

## Steps

1. **Update version.h**
   ```cpp
   #define VERSION_MAJOR 1
   #define VERSION_MINOR 0
   #define VERSION_PATCH 1
   #define VERSION_BUILD 313
   ```

2. **Commit & Tag**
   ```bash
   git add SuiteSpotv2.0/version.h
   git commit -m "Bump version to 1.0.1"
   git tag -a v1.0.1 -m "v1.0.1"
   git push origin master v1.0.1
   ```

3. **Workflow executes automatically:**
   - Detects `v1.0.1` tag
   - Builds Release|x64
   - Creates GitHub Release with DLL asset

**Workflow file:** `.github/workflows/build.yml`

**Access:** `https://github.com/ORG/SuiteSpotProject/releases`
