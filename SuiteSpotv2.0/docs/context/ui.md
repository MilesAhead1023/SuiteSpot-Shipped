# UI Subsystem Context

## Purpose
Provides the ImGui interface for configuring SuiteSpot, managing loadouts, and selecting training packs.

## Key Components
- **SettingsUI**: The main entry point for plugin configuration.
- **TrainingPackUI**: Specialized view for filtering and managing the training pack library.
- **LoadoutUI**: Interface for the LoadoutManager.

## Logic
- Renders entirely within the BakkesMod F2 → Plugins → SuiteSpot tab.
- Uses `SettingsWindowBase` to hook into the SDK's rendering pipeline.

## Files
- `SettingsUI.h/cpp`
- `TrainingPackUI.h/cpp`
- `LoadoutUI.h/cpp`
- `Source.cpp` (Settings entry point)

## Triggers
Keywords: ImGui, Tab, Button, UI, Render.
Files: `*UI.*`, `Source.cpp`.
