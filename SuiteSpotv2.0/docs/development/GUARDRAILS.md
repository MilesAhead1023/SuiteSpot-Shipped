# Project Guardrails

This document outlines the automated and behavioral guardrails enforced to ensure code quality, stability, and adherence to project conventions.

## 1. API Verification & No Hallucinations
*   **Rule:** You must query the MCP tool (`bakkesmod-sdk`, `imgui`) BEFORE writing any code that interacts with these libraries.
*   **Enforcement:** Behavioral. The agent must verify symbol existence.
*   **Constraint:** If an API is not in the MCP corpus, it does not exist. Do not invent it.

## 2. Precompiled Headers (PCH)
*   **Rule:** Every `.cpp` file must include `pch.h` as its first substantive line of code.
*   **Why:** Ensures build stability and faster compilation.
*   **Enforcement:** `tools/guardrails/check_pch.ps1`

## 3. Thread Safety & State Management
*   **Rule:** Do not store BakkesMod Wrappers (e.g., `CarWrapper`, `BallWrapper`) as persistent class members in sub-systems. Fetch them fresh or pass them into functions.
*   **Rule:** Access game state only on the game thread. Use `gameWrapper->SetTimeout` for async callbacks.
*   **Enforcement:** `tools/guardrails/check_wrapper_safety.ps1` (Warning level)

## 4. CVar Registration
*   **Rule:** All Console Variables (CVars) must be registered in `SettingsSync::RegisterAllCVars`.
*   **Why:** Centralizes configuration logic and prevents race conditions or "magic string" distribution.
*   **Enforcement:** `tools/guardrails/check_cvars.ps1`

## 5. UI Separation
*   **Rule:** ImGui code (including headers) should only be present in UI-specific files (`*UI.cpp`, `Source.cpp`, `OverlayRenderer.cpp`).
*   **Why:** Keeps core logic testable and decoupled from the rendering layer.
*   **Enforcement:** `tools/guardrails/check_ui_separation.ps1`

## 6. Documentation
*   **Rule:** Check `docs/` before making changes.
*   **Rule:** Update documentation if architectural changes are made.

## 7. Library Integrity (IMGUI)
*   **Rule:** Do not modify any files within the `IMGUI/` folder.
*   **Why:** These are third-party library files. Modifications make updates difficult and introduce non-standard behavior.
*   **Enforcement:** `tools/guardrails/check_imgui_integrity.ps1`

## How to Run Checks
Execute the master script to run all verifications:
```powershell
.\tools\guardrails\run_guardrails.ps1
```
