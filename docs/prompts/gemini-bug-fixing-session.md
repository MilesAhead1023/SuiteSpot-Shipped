# Bug Fixing Session - SuiteSpot BakkesMod Plugin

## Your Mission

You are a Gemini CLI coding agent working on **SuiteSpot**, a BakkesMod plugin for Rocket League. Your immediate task is to **identify and fix all current bugs** in the plugin before we proceed with planned feature work.

## Critical First Steps

1. **Read the comprehensive onboarding document:**
   ```
   GEMINI.md
   ```
   This file contains everything you need: architecture, build commands, patterns, gotchas, workflows, and code examples.

2. **Understand the project context:**
   - **Type:** C++20 BakkesMod plugin (DLL)
   - **Platform:** Windows x64, Visual Studio 2022
   - **Build command:** `msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64`
   - **Dependencies:** BakkesMod SDK, ImGui v1.75, nlohmann/json (via vcpkg)

3. **Check current git status:**
   ```bash
   git status
   ```

   Currently modified files:
   - `WorkshopDownloader.cpp` - Workshop download implementation
   - `version.h` - Auto-generated build version

   Untracked:
   - `.clangd` - clangd config
   - `.vscode/` - VS Code settings

## Your Task Breakdown

### Phase 1: Bug Discovery

1. **Read recent work context:**
   - Recent PRs merged: Workshop map loader/downloader (#13), race condition fixes (#14)
   - Recent feature: Pack healing (auto-corrects shot counts)
   - Removed feature: Bag rotation system

2. **Identify current issues:**
   - Check modified files (`WorkshopDownloader.cpp`) for incomplete work
   - Look for TODO comments, compile warnings, or error handling gaps
   - Review race conditions in search/render paths (per PR #14)
   - Test pack healing logic for edge cases

3. **Build the plugin:**
   ```bash
   msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
   ```

   **Expected output:** `plugins/SuiteSpot.dll`

   **Look for:**
   - Compiler errors
   - Warnings (already suppressing 4244, 4267, 4305, 4099)
   - Linker issues

4. **Analyze logs and code for patterns:**
   - Search for `TODO`, `FIXME`, `BUG`, `HACK` comments
   - Look for missing null checks (CVars, GameWrapper, ServerWrapper)
   - Check thread safety (mutex protection for shared data)
   - Verify `SetTimeout()` usage (never load maps immediately after match-end)

### Phase 2: Bug Classification

For each bug found, document:
- **Severity:** Critical (crashes), High (breaks feature), Medium (degraded UX), Low (cosmetic)
- **Location:** File path and line number
- **Root cause:** Technical explanation
- **Impact:** What breaks when this bug triggers
- **Proposed fix:** Brief implementation approach

Create a bug inventory before fixing anything.

### Phase 3: Bug Fixing

**Priority order:**
1. Critical bugs (crashes, data corruption)
2. High severity (broken features, race conditions)
3. Medium severity (poor UX, error handling)
4. Low severity (cosmetic, logging)

**For each fix:**
1. **Verify the bug** - Read the code, understand the data flow
2. **Plan the fix** - Don't just patch symptoms, fix root causes
3. **Implement** - Make minimal, focused changes
4. **Build** - Ensure compilation succeeds
5. **Log your fix** - Add `LOG()` statements for debugging
6. **Document** - Update comments if logic is non-obvious

**Critical constraints from GEMINI.md:**
- ❌ **DO NOT** load maps immediately after match-end (always use `SetTimeout()`)
- ❌ **DO NOT** assume CVars exist (always null-check)
- ❌ **DO NOT** block the render thread (background threads for long operations)
- ❌ **DO NOT** guess APIs (verify in `IMGUI/imgui.h` and `docs/bakkesmod-sdk-reference.md`)
- ✅ **DO** use mutex for shared data (training pack lists, workshop maps)
- ✅ **DO** preserve all existing features (check before removing code)
- ✅ **DO** log important state changes (`LOG("Component: Action - variable: {}", value)`)

### Phase 4: Testing

**Build verification:**
```bash
# Clean build
msbuild SuiteSpot.sln /t:Clean /p:Configuration=Release /p:Platform=x64
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

**Manual testing workflow:**
1. Launch Rocket League
2. Press **F6** → BakkesMod console → type `plugin list` → verify SuiteSpot loaded
3. Press **F2** → Settings → SuiteSpot tab
4. Test each fixed bug scenario
5. Check console logs for errors (press F6)

**Regression tests:**
- Match ends → auto-load works (with delay)
- Training pack browser opens (togglemenu)
- Filters work (search, difficulty, tags)
- Pack loads (LOAD NOW button)
- Workshop download works
- Custom pack add/delete works
- Pack healing works (shot count correction)
- Usage tracking persists (favorites sorting)

### Phase 5: Commit Strategy

**Only commit when all bugs are fixed and tested.**

**Commit message format:**
```
Fix [number] critical bugs in [component]

- [Brief description of bug 1 and fix]
- [Brief description of bug 2 and fix]
- [Brief description of bug 3 and fix]

Verified: [what you tested]

Co-Authored-By: Gemini [model-version] <noreply@google.com>
```

**Git workflow:**
```bash
git status
git diff  # Review all changes
git add WorkshopDownloader.cpp [other-files]
git commit -m "[message from above]"
# DO NOT push unless explicitly requested
```

---

## Important Context: What's On Hold

There is a **completed implementation plan** for a two-panel Training Browser redesign at:
```
docs/plans/training-browser-two-panel-layout.md
```

**DO NOT implement this plan yet!** Your task is **bug fixing only**. The two-panel redesign will happen after you've stabilized the current codebase.

---

## Code Style Guidelines

### Formatting
- **Indentation:** Tabs (existing codebase uses tabs)
- **Braces:** Opening brace on same line (K&R style)
- **Naming:**
  - Classes: `PascalCase` (e.g., `TrainingPackManager`)
  - Methods: `PascalCase` (e.g., `LoadPackImmediately()`)
  - Member variables: `camelCase_` with trailing underscore (e.g., `selectedPackCode_`)
  - Local variables: `camelCase` (e.g., `packCount`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `LEFT_PANEL_WIDTH`)

### Comments
- Only add comments where logic isn't self-evident
- Don't add docstrings to existing code you didn't change
- Use `//` for single-line, `/* */` for multi-line
- Detailed comment blocks use the `#detailed comments: FunctionName` pattern (see `SuiteSpot.cpp:124`)

### Error Handling
- Validate at system boundaries (user input, external APIs, file I/O)
- Trust internal code and BakkesMod framework guarantees
- Always null-check CVars, GameWrapper, ServerWrapper before dereferencing
- Use early returns for error conditions

---

## Resources Available to You

### Documentation Files
- **`GEMINI.md`** (START HERE) - Your comprehensive onboarding guide
- **`CLAUDE.md`** - Condensed project overview
- **`docs/architecture.md`** - Architecture diagrams and technical reference
- **`docs/bakkesmod-sdk-reference.md`** - BakkesMod API reference
- **`docs/plans/training-browser-two-panel-layout.md`** - Two-panel plan (on hold)

### Code Reference Files
- **`ConstantsUI.h`** - All UI colors, dimensions, difficulty levels
- **`MapList.h`** - Data structure definitions (TrainingEntry, WorkshopEntry)
- **`HelpersUI.h`** - UI utility functions
- **`StatusMessageUI.h`** - Toast notification API

### Build Files
- **`SuiteSpot.vcxproj`** - MSBuild configuration
- **`BakkesMod.props`** - BakkesMod SDK integration
- **`CompiledScripts/update_version.ps1`** - Version auto-incrementer

---

## Success Criteria

You'll know you're done when:

✅ **Build succeeds** with 0 errors, 0 new warnings
✅ **All critical bugs fixed** (crashes, data corruption)
✅ **All high-severity bugs fixed** (broken features)
✅ **Manual testing passed** (all workflows work in-game)
✅ **Logs are clean** (no error messages in F6 console)
✅ **Changes committed** with descriptive message
✅ **Ready for feature work** (two-panel browser can be implemented next)

---

## Communication Protocol

**When reporting progress:**
1. List bugs found (with severity, location, root cause)
2. Show fixes applied (file paths, brief description)
3. Report build status (success/failure, warnings)
4. Share test results (what you verified)

**When blocked:**
- Clearly state what's blocking you
- What you've tried
- What you need (user input, clarification, external resource)

**When asking questions:**
- Provide context (file, line number, current state)
- Explain what you're trying to accomplish
- Offer 2-3 alternative approaches if applicable

---

## Ready to Start!

Begin by running:

```bash
# Read your onboarding guide
cat GEMINI.md

# Check build status
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64

# Review modified files
git diff WorkshopDownloader.cpp
git diff version.h

# Search for known issue markers
grep -r "TODO" --include="*.cpp" --include="*.h"
grep -r "FIXME" --include="*.cpp" --include="*.h"
grep -r "BUG" --include="*.cpp" --include="*.h"
```

After completing your bug discovery and fixes, report back with:
1. **Bug inventory** (what you found)
2. **Fixes applied** (what you changed)
3. **Test results** (what you verified)

Good luck! The codebase is well-documented and you have everything you need. 🎯
