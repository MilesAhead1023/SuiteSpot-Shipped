# Developer Rule: Thread-Safe ImGui in BakkesMod

## The Rule
**Never** perform ImGui operations inside `RegisterDrawable`. All ImGui window and widget calls must happen on the **UI/Main Thread**.

## Why?
BakkesMod executes `RegisterDrawable` callbacks on the **GPU Rendering Thread**. The Dear ImGui library is not thread-safe. Attempting to manage windows or draw text from the rendering thread while BakkesMod is managing the UI context on the main thread will result in a **Fatal Rendering Thread Exception**.

## Implementation Pattern

### ❌ WRONG (Crashes)
```cpp
// onLoad
gameWrapper->RegisterDrawable([this](auto) {
    myWindow->Render(); // CRASH: Wrong thread!
});
```

### ✅ CORRECT (Stable)
```cpp
// Source.cpp
void SuiteSpot::RenderSettings() {
    if (myWindow) {
        myWindow->Render(); // SAFE: Executes on safe ImGui pass
    }
    
    if (settingsUI) {
        settingsUI->RenderMainSettingsWindow();
    }
}
```

## When does this run?
In current BakkesMod SDK versions, `RenderSettings` is the primary entry point for ImGui. While traditional "always-on" HUDs used `RegisterDrawable` with the legacy `CanvasWrapper`, modern ImGui-first plugins must bridge through `RenderSettings` or a compatible `Render()` override if provided by the SDK base class.

## CanvasWrapper Constraint
Per project requirements, **do not use `CanvasWrapper`** for any new rendering. Use the `ImDrawList` pattern from the `OverlayRenderer` class.
