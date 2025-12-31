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

- [ ] **Standardize Status Message System**
  - Create `StatusMessage` class with auto-fade
  - Replace 3 different timer implementations

- [ ] **Add ListClipper to SettingsUI Dropdowns**
  - Apply pattern from TrainingPackUI
  - Better performance with 500+ training packs

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
