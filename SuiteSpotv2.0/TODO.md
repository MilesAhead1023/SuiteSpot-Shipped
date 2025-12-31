# SuiteSpot TODO

Current plugin improvements, refactoring tasks, and technical debt.

---

## High Priority - Code Quality

- [x] **Split RenderMapSelectionTab()** *(SettingsUI.cpp:212-230)*
  - Extracted `RenderFreeplayMode()` (~53 lines)
  - Extracted `RenderTrainingMode()` (~161 lines)
  - Extracted `RenderWorkshopMode()` (~138 lines)
  - Benefit: Each mode independently maintainable

- [ ] **Create UI Helper Functions** *(New: UIHelpers.h/cpp)*
  - `InputIntWithRange()` - input with clamping, cvar, tooltip
  - `ComboWithTooltip()` - dropdown with tooltip
  - `StatusMessage()` - auto-fade status display
  - Benefit: Reduce 20+ repeated patterns to single-line calls

- [ ] **Centralize Configuration Constants** *(New: UIConstants.h)*
  - Widget widths (220, 300, etc.)
  - Delay ranges (0-300)
  - Difficulty levels array
  - Map mode labels

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
