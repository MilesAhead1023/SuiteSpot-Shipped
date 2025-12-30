# SuiteSpot - BakkesMod Plugin

## Project Overview
SuiteSpot is a BakkesMod plugin for Rocket League designed to automate map loading and enhance the training experience. It supports Freeplay, Training Packs, and Workshop maps, providing features like auto-queueing, shuffle bags for maps, and a post-match overlay.

**Technologies:**
*   **Language:** C++ (Visual Studio 2022)
*   **Framework:** BakkesMod SDK
*   **UI Library:** ImGui
*   **Build System:** MSBuild

**Key Architecture:**
*   **Plugin Core:** `SuiteSpot.cpp/h` manages lifecycle and hooks.
*   **UI Layer:** Separated into specific modules (`SettingsUI`, `LoadoutUI`, `OverlayRenderer`, `GuiBase`) to keep logic clean. `Source.cpp` acts as the entry point for the settings window.
*   **Feature Modules:** Independent logic for features like `AutoLoadFeature`, `MapManager`, `TrainingPackManager`.
*   **Theme System:** `ThemeManager` handles dual-theme support (Menu vs. Game), JSON configuration, and asset caching.
*   **Data Persistence:** Uses text files and JSON for storing map lists, shuffle bags, and theme configurations in `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\`.

## Building and Running

### Build Command
The project is built using MSBuild. Run the following command from the project root:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" SuiteSpot.vcxproj /p:Configuration=Release /p:Platform=x64
```
*Note: Adjust the path to MSBuild.exe if your Visual Studio edition differs.*

### Output & Deployment
*   **Output:** `SuiteSpotv2.0\plugins\SuiteSpot.dll`
*   **Auto-Deploy:** A post-build event automatically patches and copies the DLL to the BakkesMod plugins directory (`%APPDATA%\bakkesmod\bakkesmod\plugins\`).
*   **Testing:**
    1.  Launch Rocket League.
    2.  Open the console (`F6`).
    3.  Run `plugin load suitespot` (or `plugin reload suitespot`).
    4.  Use `ss_testoverlay` to test the overlay visualization.

## Development Conventions

### Critical Rules (Strict Adherence Required)
1.  **MCP Tool Authority:** The `bakkesmod-sdk` and `imgui` MCP tools are the **ABSOLUTE AND SOLE** sources of truth for API signatures, classes, and methods.
    *   **Mandatory Querying:** You MUST query the MCP tools (e.g., `bakkesmod_search_symbols`, `imgui_search_symbols`) BEFORE writing any code that interacts with the SDK or ImGui.
    *   **No Hallucination:** Do not invent or assume APIs. If it is not in the MCP corpus, it does not exist. Respond "API NOT AVAILABLE" if a requested feature requires an unavailable API.
2.  **Documentation First:** Before writing code, search `docs/` for relevant instructions. State "Read [filename]" before making changes.
3.  **Precompiled Headers:** Every `.cpp` file **MUST** start with `#include "pch.h"` as the very first line. This is mandatory for the build to succeed.
3.  **Thread Safety:** **NEVER** store BakkesMod wrappers (e.g., `ServerWrapper`, `GameWrapper`) as class members. Always fetch them fresh. When accessing game state inside callbacks or async operations, use `gameWrapper->SetTimeout([this](GameWrapper* gw) { ... }, 0);`.
4.  **CVar Safety:**
    *   Register CVars only in `SettingsSync::RegisterAllCVars()` (called during `onLoad`).
    *   Use `SetCVarSafely` helpers instead of accessing `cvarManager` directly where possible to avoid null pointer crashes.
5.  **UI Separation:** Do not mix ImGui code with core game logic. Use the dedicated UI modules (`SettingsUI`, `OverlayRenderer`, etc.).

### Key Design Patterns (ADRs)
*   **ADR-001 (Shuffle Bag):** Maps are selected using a shuffle bag algorithm to ensure no repeats until all maps are played.
*   **ADR-002 (SetTimeout):** All game state access must happen within a `SetTimeout` callback to ensure thread safety.
*   **ADR-003 (UI Separation):** Settings UI logic is isolated in `SettingsUI.cpp` to keep the core plugin logic clean.

### Project Structure
*   `SuiteSpot.cpp`: Main plugin entry point, hooks, and global state management.
*   `SettingsUI.cpp`: Main settings window rendering (using ImGui).
*   `MapManager.cpp`: Logic for managing map lists (Training, Workshop, Freeplay) and persistence.
*   `AutoLoadFeature.cpp`: Logic for auto-queueing and map switching after matches.
*   `OverlayRenderer.cpp`: Renders the post-match stats overlay.
*   `ThemeManager.cpp`: Manages loading and applying visual themes.
*   `docs/`: Extensive documentation including `DEVELOPMENT_GUIDE.md`, `DECISIONS.md`, and `CLAUDE_AI.md`.

## Key Documentation
*   **`docs/development/DEVELOPMENT_GUIDE.md`**: Detailed guide on workflows, common patterns, and troubleshooting.
*   **`docs/architecture/DECISIONS.md`**: Architectural Decision Records (ADR) explaining the "why" behind design choices.
*   **`docs/architecture/CLAUDE_AI.md`**: High-level architectural context and constraints.
*   **`AGENT.md`**: Authoritative contract for BakkesModSDK API usage and tool interaction.
*   **`ThemeManager.h`**: Reference for the theming system capabilities.
