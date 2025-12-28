# SuiteSpot UI/UX Improvement TODO

Based on the prioritized list of UI/UX issues.

Current UI module ownership:
- Prejump packs UI: `PrejumpUI.cpp`
- Loadout management UI: `LoadoutUI.cpp`
- Post-match overlay UI: `OverlayRenderer.cpp`

## 🔴 High Priority

- [x] **1. Prevent Overlapping / Duplicate UI Rendering**
    - **Status:** REMEDIED. UI suppression implemented in F2 panel when standalone window is open.
- [x] **2. Fix Overlay Text Formatting & Placeholder Strings**
    - **Status:** REMEDIED. Rank/MMR standardized with human-readable names and "N/A" fallbacks.
- [x] **3. Provide Feedback for User Actions**
    - **Status:** REMEDIED. Added `gameWrapper->Toast()` feedback for Load, Apply Loadout, and Shuffle actions.
- [x] **4. Ensure Settings Persist Across Sessions**
    - **Status:** REMEDIED. All major settings linked to CVars and saved.

## 🟠 Medium Priority

- [x] **5. UI Layout Refactor: Group and Label Controls**
    - **Status:** REMEDIED. Main Settings tab refactored with CollapsingHeaders.
- [x] **6. Clarify Labels and Terminology**
    - **Status:** REMEDIED. Terminology standardized (e.g., "Auto-Queue Next Match").
- [x] **7. Standalone Window UX Improvements**
    - **Status:** REMEDIED. Window visibility decoupled from enablement.
- [x] **8. Truncated Text & Plugin Name Readability**
    - **Status:** DEPRECATED. Limited by BakkesMod core UI; mitigated by clear metadata headers.

## 🟡 Low Priority / Polish

- [x] **9. Fix Missing Glyphs / Unsupported Characters**
    - **Status:** REMEDIED. Replaced symbol "★" with text "MVP" for guaranteed rendering.
- [x] **10. Error Handling & User Warnings**
    - **Status:** REMEDIED. Added tooltips and status checks for empty map lists.
- [x] **11. Add “Reset to Defaults” Controls**
    - **Status:** REMEDIED. Reset buttons added to Overlay Layout tab.
- [x] **12. Improve Overlay Text Contrast**
    - **Status:** REMEDIED. Overlay now uses semi-transparent background panel.
- [x] **13. Consistency Pass on Language & Formatting**
    - **Status:** REMEDIED. Standardized "Freeplay" and capitalization.
- [x] **14. Visual Hierarchy & Spacing Pass**
    - **Status:** REMEDIED. Consistent use of separators and spacing implemented.
