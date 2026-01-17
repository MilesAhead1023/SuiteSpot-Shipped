# Plan: Feature - "The Coach" (Session Stats & Analytics)

**Goal:** Implement a comprehensive performance tracking system that provides real-time feedback and session-long analytics to help players improve scientifically.

## 1. Plugin Scope & Feature Set

### Phase 2a: Session Stats Tracker (Immediate)
*   **Live Metrics:** Track Wins, Losses, MMR Delta (+/-), Goals, Saves, Assists, and Shots per session.
*   **Session View:** A new UI tab "The Coach" showing these totals.
*   **Toast Notifications:** Status messages when MMR changes (e.g., "+12 MMR").

### Phase 2b: Warmup Efficiency (Analytics)
*   **Metric:** "Shots Scored / Shots Attempted" in Custom Training.
*   **Persistence:** Save daily totals to a JSON file to show "Improvement Over Time."

### Phase 2c: The "Simon Says" Warmup (Intelligent Logic)
*   **Goal:** Mandatory mechanic check before queuing (to be implemented after stats foundation is solid).

## 2. Documented APIs (MCP Verified)

### BakkesMod SDK
*   `GameWrapper::GetMMRWrapper()` -> `MMRWrapper`
*   `MMRWrapper::GetPlayerMMR(SteamID, PlaylistID)`: Get current rating.
*   `PriWrapper::GetMatchGoals()`, `GetMatchSaves()`, `GetMatchAssists()`, `GetMatchShots()`: Match-specific stats.
*   `gameWrapper->HookEvent("Function TAGame.PRI_TA.OnStatGoalsChanged", ...)`: Live goal detection.

### ImGui
*   `ImGui::Columns()` or `ImGui::Table()`: For clean stats display.
*   `ImGui::ProgressBar()`: For Warmup Efficiency visualization.

## 3. Data Structures

### `struct SessionStats`
*   `int wins = 0`, `int losses = 0`.
*   `float startMMR = 0.0f`, `float currentMMR = 0.0f`.
*   `int totalGoals = 0`, `totalSaves = 0`, etc.

### `class SessionManager`
*   Responsible for event hooks and state updates.
*   Methods: `OnMatchEnded()`, `UpdateMMR()`, `ResetSession()`.

## 4. Error Handling
*   **MMR Failure:** Handle cases where the MMR API returns 0 or fails (common during server issues).
*   **Null Wrappers:** Standard SDK safety (SetTimeout + Null checks).

## 5. Testing Strategy
*   **Unit Tests:** Use `mock_sdk` to verify `SessionManager` correctly increments stats when events are triggered.
*   **Manual Test:** Play a match (or simulate match end via console) and verify UI updates.

## 6. Implementation Checklist

### Step 1: Foundation
- [ ] Create `SessionManager.h` and `SessionManager.cpp`.
- [ ] Implement `SessionStats` struct.
- [ ] Register `SessionManager` in `SuiteSpot.h/cpp`.

### Step 2: Hooking & Logic
- [ ] Hook `EventMatchEnded` to trigger `UpdateMMR`.
- [ ] Hook `Stat` events to track live performance.
- [ ] Implement logic to calculate MMR delta.

### Step 3: UI Integration
- [ ] Create `CoachUI.h` and `CoachUI.cpp`.
- [ ] Add "The Coach" tab to `SettingsUI`.
- [ ] Implement stats table rendering using `ModernUI` styled components.

### Step 4: Verification
- [ ] Run `run_guardrails.ps1`.
- [ ] Run `run_pioneer_tests.cpp` (Update with Session tests).
- [ ] Build and Verify.
