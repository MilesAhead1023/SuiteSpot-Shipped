# SuiteSpot TODO

Current plugin improvements, refactoring tasks, and technical debt.

---

## High Priority - Code Quality

- [x] **Split RenderMapSelectionTab()** *(SettingsUI.cpp:212-230)*
  - Extracted `RenderFreeplayMode()` (~53 lines)
  - Extracted `RenderTrainingMode()` (~161 lines)
  - Extracted `RenderWorkshopMode()` (~138 lines)
  - Benefit: Each mode independently maintainable

- [x] **Create UI Helper Functions** *(New: UIHelpers.h/cpp)*
  - Created UIHelpers.h with 7 helper function declarations
  - Created UIHelpers.cpp with all implementations
  - Refactored 11 UI patterns: 4 InputIntWithRange, 3 ComboWithTooltip, 2 CheckboxWithCVar, 2 StatusMessage
  - Moved SetCVarSafely template from SettingsUI to UIHelpers for reuse
  - Updated SettingsUI.cpp, LoadoutUI.cpp, TrainingPackUI.cpp to use helpers
  - Added files to SuiteSpot.vcxproj build configuration
  - Build verified: 0 errors, 0 warnings
  - Benefit: Eliminated ~48 lines of repetitive code, improved maintainability

- [x] **Centralize Configuration Constants** *(New: UIConstants.h)*
  - Created UIConstants.h with 100+ individual constants
  - Maximum granularity: Each UI element has its own named constant
  - Organized into nested namespaces: UI::SettingsUI, UI::TrainingPackUI, UI::LoadoutUI
  - All constants have descriptive comments explaining exact purpose
  - Updated SettingsUI.cpp, TrainingPackUI.cpp, LoadoutUI.cpp to use constants
  - Build verified: 0 errors, 0 warnings

---

## Medium Priority - Consistency

- [x] **Standardize Status Message System** *(New: StatusMessage.h/cpp)*
  - Created StatusMessage class with DisplayMode enum (Timer, TimerWithFade, ManualDismiss)
  - Created Type enum (Success, Error, Warning, Info) with automatic color mapping
  - Migrated LoadoutUI: Replaced 3 variables (text, color, timer) with single StatusMessage
  - Migrated SettingsUI: Replaced 2 variables (bool, timer) with StatusMessage
  - Migrated TrainingPackUI: Replaced 2 variables (string, bool) with StatusMessage
  - Added files to SuiteSpot.vcxproj build configuration
  - Build verified: 0 errors, 0 warnings
  - Benefit: Eliminated ~150 lines, unified 3 patterns into single API

- [x] **Add ListClipper to SettingsUI Dropdowns** *(SettingsUI.cpp)*
  - Optimized Training Packs dropdown (lines 279-296): Reduced from 500+ items to ~12 visible items per frame
  - Optimized Workshop Maps dropdown (lines 429-443): Applied same pattern for 50-200+ workshop maps
  - Optimized Freeplay Maps dropdown (lines 229-243): Applied for consistency (~30 maps)
  - Replaced simple for-loops with ImGuiListClipper pattern from TrainingPackUI
  - Build verified: 0 errors, 0 warnings
  - Benefit: 98% reduction in rendering work for large lists, eliminates lag when scrolling 500+ training packs

- [ ] **Extract Form Components**
  - Workshop path configuration section
  - Shuffle bag manager section

---

## Nice-to-Have

- [ ] **Validation Framework**
  - Shared `ValidatePackCode()`, `ValidatePath()` functions

- [ ] **Consistent State Management**
  - Choose "read all at start" vs "read on-demand" pattern

---

**Last Updated:** 2025-12-31
