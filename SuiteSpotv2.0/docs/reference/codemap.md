# Project Codemap

This document provides a high-level overview of the `SuiteSpotv2.0` directory structure for an AI coding agent. Its purpose is to map out the key files and directories to help you navigate the codebase and locate relevant information.

## Root Directory

The SuiteSpot plugin project is located at:

-   `C:\Users\bmile\suiteSpotProject\SuiteSpotv2.0\`

## SuiteSpot Project Structure

### Main Plugin Files (`SuiteSpotv2.0/`)

-   `SuiteSpot.h` & `SuiteSpot.cpp`: The core plugin class. Contains lifecycle methods (`onLoad`, `onUnload`), event hooks, CVar registration, and main plugin logic.
-   `Source.cpp`: Settings window entry point. Delegates `RenderSettings()` to `SettingsUI`.
-   `SettingsUI.h` & `SettingsUI.cpp`: Main ImGui settings window rendering and tab content.
-   `PrejumpUI.h` & `PrejumpUI.cpp`: Prejump pack tab rendering and filtering UI.
-   `LoadoutUI.h` & `LoadoutUI.cpp`: Loadout management tab UI and selection state.
-   `OverlayRenderer.h` & `OverlayRenderer.cpp`: Post-match overlay rendering and visual layout parameters.
-   `MapList.h` & `MapList.cpp`: Map data storage. Contains `RLMaps`, `RLTraining`, and `RLWorkshop` vectors with map information.
-   `MapManager.h` & `MapManager.cpp`: Map persistence, shuffle bag, and workshop discovery logic.
-   `SettingsSync.h` & `SettingsSync.cpp`: Centralized CVar registration and settings state synchronization.
-   `AutoLoadFeature.h` & `AutoLoadFeature.cpp`: Match-end auto-load and auto-queue logic (uses SetTimeout).
-   `PrejumpPackManager.h` & `PrejumpPackManager.cpp`: Prejump pack loading, caching, and filtering logic.
-   `GuiBase.h` & `GuiBase.cpp`: Base class for ImGui settings windows. Handles window lifecycle and rendering.
-   `LoadoutManager.h` & `LoadoutManager.cpp`: Car loadout management functionality.
-   `pch.h` & `pch.cpp`: Precompiled header files. **CRITICAL**: Every `.cpp` must include `pch.h` as the first line.
-   `logging.h`: Logging utilities for debug output.
-   `version.h`: Plugin version information (auto-updated by `update_version.ps1`).
-   `resource.h`: Windows resource definitions.

### Build Configuration

-   `SuiteSpot.sln`: Visual Studio solution file.
-   `SuiteSpot.vcxproj`: Project file with build settings, file references, and configurations.
-   `BakkesMod.props`: Property sheet that configures BakkesMod SDK paths via registry lookup.
    -   Uses `$(BakkesModPath)` from Windows registry to find SDK dynamically
    -   No local SDK copy needed - references installed BakkesMod SDK

### ImGui (UI Framework)

-   `IMGUI/`: ImGui library files for rendering the settings interface.
    -   `imgui.cpp`, `imgui_widgets.cpp`, `imgui_draw.cpp`: Core ImGui implementation.
    -   `imgui_impl_dx11.cpp`, `imgui_impl_win32.cpp`: DirectX 11 and Win32 platform backends.
    -   Custom additions: `imgui_rangeslider.cpp`, `imgui_searchablecombo.cpp`, `imgui_timeline.cpp`

### Build Artifacts

-   `build/`: Intermediate build files (.obj, etc.).
-   `plugins/`: Output directory for compiled `SuiteSpot.dll`.

### Scripts

-   `update_version.ps1`: Pre-build script that updates version numbers.
-   `generate_settings.ps1`: Post-build script that generates settings files.
-   `scrape_prejump.ps1`: Utility for scraping training pack data.

### Documentation

-   `SuiteSpotDocuments/`: Contains all instructional and reference documentation for AI agent development.
    -   `instructions/context/`: API-specific documentation (e.g., `gamewrapper-api.md`, `serverwrapper-api.md`).
    -   `instructions/development/`: Development guides (e.g., `development-guide.md`, `deployment-guide.md`).
    -   `instructions/reference/`: High-level reference material, including this codemap.

## External Dependencies

### BakkesMod SDK (Registry-based)

The BakkesMod SDK is **not** stored in the project directory. It's referenced via Windows registry:

-   Registry Key: `HKEY_CURRENT_USER\Software\BakkesMod\AppPath@BakkesModPath`
-   Typical Location: `%APPDATA%\bakkesmod\bakkesmod\`
-   SDK Subdirectory: `bakkesmodsdk/`
    -   `include/`: C++ header files (`.h`) defining the BakkesMod API.
        -   `bakkesmod/plugin/`: Plugin structure headers (`BakkesModPlugin`, `PluginWindow`).
        -   `bakkesmod/wrappers/`: Wrapper classes for game objects (`GameWrapper`, `ServerWrapper`, `CarWrapper`, `BallWrapper`, etc.).
    -   `lib/`: Pre-compiled library files (`pluginsdk.lib`).
    -   `bakkesmod-patch.exe`: Post-build patching tool.

### Reference Template

-   `C:\Users\bmile\suiteSpotProject\BakkesmodPluginTemplate\`: A template project for reference. Shows basic plugin structure and implementation patterns.

## Key File Locations Quick Reference

| Task | File to Modify |
|------|---------------|
| Add UI elements | `SettingsUI.cpp` (RenderMainSettingsWindow and tab renderers) |
| Add game logic | `SuiteSpot.cpp` |
| Add event hook | `SuiteSpot.cpp` (register in onLoad) |
| Add CVar | `SuiteSpot.cpp` (register in onLoad) |
| Add map data | `MapList.cpp` (RLMaps/RLTraining/RLWorkshop vectors) |
| Add loadout feature | `LoadoutManager.cpp` |
| Build configuration | `SuiteSpot.vcxproj` or `BakkesMod.props` |
| SDK path issues | `BakkesMod.props` (should use registry path) |
