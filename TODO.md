# SuiteSpot Critical Fixes & Improvements

## 🚨 Critical Fix: End-of-Match Auto-Load Logic

**Issue:** The auto-load feature frequently fails or behaves erratically (double loads) at the end of a match.

**Root Causes (Verified via Static Analysis & SDK Docs):**
1.  **Double Hooking:** The plugin hooks both `Function TAGame.GameEvent_Soccar_TA.EventMatchEnded` AND `Function TAGame.AchievementManager_TA.HandleMatchEnded` to the same handler (`GameEndedEvent`). This causes the logic to fire twice in rapid succession, leading to race conditions where the bag rotation advances twice or multiple load commands conflict.
2.  **Pre-Hook Execution:** The current implementation uses `HookEvent` (Pre-hook). This triggers `GameEndedEvent` *before* the game engine has finished its internal match-end processing.
3.  **Synchronous Execution:** When the user-configured delay is 0s, the `load_training` command is sent immediately within the event stack. The game engine often rejects map load commands while still in the "Match Ended" state transition.

**Required Actions:**
- [x] **Remove Redundant Hook:** Delete the `HookEvent` call for `Function TAGame.AchievementManager_TA.HandleMatchEnded` in `SuiteSpot.cpp`.
- [x] **Switch to Post-Hook:** Change the remaining hook for `Function TAGame.GameEvent_Soccar_TA.EventMatchEnded` from `HookEvent` to `HookEventPost`. This ensures our logic runs only *after* the game has stabilized its state.
- [x] **Enforce Async Execution:** In `AutoLoadFeature.cpp` (or the `GameEndedEvent` handler), ensure that even a 0s delay forces a context switch via `gameWrapper->SetTimeout(..., 0.1f)`. This guarantees the command runs on the next frame, completely outside the event stack.

---

## 🛠️ Codebase Health & Standards

**Prioritized Improvements:**
- [ ] **Memory Safety:** Replace raw pointers (`mapManager`, `settingsSync`, etc.) in `SuiteSpot.h` with `std::unique_ptr` to ensure RAII compliance and prevent leaks on unload/crash.
- [ ] **Modern C++:** Replace `std::bind` in event hooks with C++20 lambdas for better type safety and readability.
- [ ] **Input Blocking:** Improve `ShouldBlockInput` in `SuiteSpot.cpp` to check if the ImGui window is actually visible/focused, preventing input blocking when the browser is open but hidden.
