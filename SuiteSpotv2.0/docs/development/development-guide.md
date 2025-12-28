# BakkesMod Plugin Development Guide

## Build System Overview

The BakkesMod plugin template uses the Visual Studio project system (`.vcxproj`) to manage the compilation process. Key aspects of the build configuration are defined in the project file and a custom property sheet.

### SuiteSpot Build Environment

**Development Setup:**
- Visual Studio 2022 Professional
- Platform Toolset: v143
- MSBuild Path: `C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe`

**Build Command (from project root):**
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64 /v:minimal
```

**Output:** `SuiteSpotv2.0\plugins\SuiteSpot.dll`

### `BakkesPluginTemplate.vcxproj` (Visual Studio Project File)

This file defines the core settings for building your plugin:

-   **Project Type:** Configured as a `DynamicLibrary` (`.dll`), which is the standard output for BakkesMod plugins.
-   **Platform:** Targets `x64` architecture.
-   **Toolchain:** Uses the MSVC build tools (e.g., `v143` for Visual Studio 2022).
-   **Output:** The compiled plugin (`.dll`) is placed in the `$(SolutionDir)plugins\` directory.
-   **C++ Standard:** Configured to use `stdcpp20` (C++20 standard) for Release builds.
-   **Precompiled Headers:** Utilizes `pch.h` for faster compilation.
-   **Source Files:** Lists all `.cpp` and `.h` files included in the project, including ImGui sources.

### `BakkesMod.props` (Property Sheet)

This file is crucial for integrating the BakkesMod SDK into the build process. It is imported by the `.vcxproj` file.

-   **SDK Path Discovery:** It automatically determines the BakkesMod installation path by reading the Windows Registry key `HKEY_CURRENT_USER\Software\BakkesMod\AppPath`. This eliminates the need to hardcode SDK paths.
-   **Include Directories:** It adds `$(BakkesModPath)\bakkesmodsdk\include` to the compiler's include paths, making all SDK headers available to your project.
-   **Library Directories & Dependencies:** It adds `$(BakkesModPath)\bakkesmodsdk\lib` to the linker's library search paths and specifies `pluginsdk.lib` as a required library for linking.
-   **Automatic Hot-Reloading (Post-Build Event):** After every successful build, `BakkesMod.props` configures a post-build event to execute `$(BakkesModPath)\bakkesmodsdk\bakkesmod-patch.exe` with the path to your newly compiled `.dll`. This tool automates the process of unloading the old plugin and loading the new one in BakkesMod, enabling instant iteration without restarting Rocket League.

This integrated build setup provides a highly efficient development workflow, automatically managing SDK dependencies and facilitating rapid testing of changes.

## 1. Plugin Setup

A BakkesMod plugin is a C++ project that is compiled into a DLL. The easiest way to start a new plugin is to use the [BakkesMod Plugin Template](https://github.com/bakkesmod/BakkesmodPluginTemplate).

1.  Download the template.
2.  Open the `.sln` file in Visual Studio.
3.  Rename the project to your desired plugin name.
4.  In `plugin.cpp`, change the `BAKKESMOD_PLUGIN` macro to reflect your plugin's name, description, and version.

## 2. Plugin Lifecycle

Your main plugin class must inherit from `BakkesMod::Plugin::BakkesModPlugin`. This class has two important functions that control the plugin's lifecycle:

-   `void onLoad()`: This function is called when your plugin is loaded. Use it to initialize your plugin, register CVars and notifiers, and hook game events.
-   `void onUnload()`: This function is called when your plugin is unloaded. Use it to clean up any resources your plugin has allocated. CVars and notifiers are automatically unregistered.

## 3. Interacting with the Game (`gameWrapper`)

The `gameWrapper` is your main tool for interacting with the game. It is a member of your main plugin class.

-   **Get Game State:** Use functions like `gameWrapper->IsInGame()`, `gameWrapper->IsInFreeplay()`, etc. to check the current game state.
-   **Get Game Objects:** Use functions like `gameWrapper->GetCurrentGameState()`, `gameWrapper->GetLocalCar()`, etc. to get wrappers for game objects.

## 4. CVars and Notifiers (`cvarManager`)

The `cvarManager` is used to interact with the BakkesMod console. It is also a member of your main plugin class.

### CVars (Console Variables)

CVars are variables that can be changed through the BakkesMod console.

-   **Registration:**
    ```cpp
    cvarManager->registerCvar("my_cvar", "default_value", "description");
    ```
-   **Getting/Setting Value:**
    ```cpp
    CVarWrapper cvar = cvarManager->getCvar("my_cvar");
    std::string value = cvar.getStringValue();
    cvar.setValue("new_value");
    ```
-   **Value Change Callback:**
    ```cpp
    cvar.addOnValueChanged([this](std::string cvarName, CVarWrapper newCvar) {
        // Do something when the cvar value changes
    });
    ```
-   **Binding to a variable:**
    ```cpp
    std::shared_ptr<bool> enabled = std::make_shared<bool>(false);
    cvarManager->registerCvar("my_plugin_enabled", "0", "Enable my plugin", true, true, 0, true, 1).bindTo(enabled);
    ```

### Notifiers

Notifiers are commands that can be executed from the console.

-   **Registration:**
    ```cpp
    cvarManager->registerNotifier("my_notifier", [this](std::vector<std::string> args) {
        // Do something when the notifier is executed
    }, "description", PERMISSION_ALL);
    ```

## 5. Event Hooking

You can hook into game events to execute your code when something happens in the game.

-   **Simple Hook:**
    ```cpp
    gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
        // This code will be executed right before the ball explodes
    });
    ```
-   **Post-Event Hook:**
    ```cpp
    gameWrapper->HookEventPost("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
        // This code will be executed right after the ball explodes
    });
    ```
-   **Hook with Caller:**
    ```cpp
    gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.OnCarTouch", [this](BallWrapper ball, void* params, std::string eventName) {
        // Get the car that touched the ball from the params
        struct CarTouchParams {
            uintptr_t car;
            unsigned char hitType;
        };
        CarTouchParams* touchParams = (CarTouchParams*)params;
        CarWrapper car = CarWrapper(touchParams->car);
        
        // Do something with the car and ball
    });
    ```

## 6. Creating a GUI

You can create two types of GUIs for your plugin: a settings tab in the BakkesMod settings window (F2), or a standalone plugin window.

### Settings Tab

1.  Inherit from `SettingsWindowBase` in your main plugin class.
2.  Uncomment `RenderSettings()` in `plugin.h` and `plugin.cpp`.
3.  Implement the `RenderSettings()` function to draw your settings using ImGui widgets.

```cpp
// plugin.h
class MyPlugin : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{
    // ...
    void RenderSettings() override;
    // ...
};

// plugin.cpp
void MyPlugin::RenderSettings()
{
    ImGui::TextUnformatted("My Plugin Settings");
    // Add your ImGui widgets here
}
```

### Plugin Window

1.  Inherit from `PluginWindowBase` in your main plugin class.
2.  Uncomment `RenderWindow()` in `plugin.h` and `plugin.cpp`.
3.  Implement the `RenderWindow()` function to draw your window's contents.
4.  To open the window, you need to register it as a togglable menu:
    ```cpp
    // In onLoad()
    cvarManager->registerNotifier("toggle_my_plugin_window", [this](std::vector<std::string> args) {
        cvarManager->executeCommand("togglemenu " + GetMenuName());
    }, "Toggle the My Plugin window", PERMISSION_ALL);
    ```

```cpp
// plugin.h
class MyPlugin : public BakkesMod::Plugin::BakkesModPlugin, public PluginWindowBase
{
    // ...
    void RenderWindow() override;
    // ...
};

// plugin.cpp
void MyPlugin::RenderWindow()
{
    ImGui::TextUnformatted("My Plugin Window");
    // Add your ImGui widgets here
}
```