# SuiteSpot Coding Standards

This document defines coding patterns and best practices for BakkesMod plugin development in the SuiteSpot project.

## Table of Contents

1. [Project Architecture](#project-architecture)
2. [Null Safety](#null-safety)
3. [CVar Registration](#cvar-registration)
4. [Event Hooking](#event-hooking)
5. [Canvas Rendering](#canvas-rendering)
6. [Memory Management](#memory-management)
7. [Error Handling](#error-handling)
8. [Naming Conventions](#naming-conventions)
9. [Code Organization](#code-organization)
10. [ImGui Patterns](#imgui-patterns)

---

## Project Architecture

### Manager Pattern

SuiteSpot uses a hub-and-spoke architecture with managers:

```cpp
// SuiteSpot.h - Central hub owns all managers
class SuiteSpot : public BakkesMod::Plugin::BakkesModPlugin {
private:
    std::unique_ptr<MapManager> mapManager;
    std::unique_ptr<TrainingPackManager> trainingPackMgr;
    std::unique_ptr<SettingsSync> settingsSync;
    std::unique_ptr<AutoLoadFeature> autoLoadFeature;
};
```

**Guidelines:**
- Each manager has a single responsibility
- Managers are owned by the plugin hub via `std::unique_ptr`
- Cross-manager communication goes through the hub
- UI classes receive data from managers, don't own business logic

### Dependency Flow

```
SuiteSpot (Plugin Hub)
    │
    ├── Managers (Business Logic)
    │   ├── MapManager
    │   ├── TrainingPackManager
    │   ├── SettingsSync
    │   └── AutoLoadFeature
    │
    └── UI Layer (Presentation)
        ├── SettingsUI
        ├── TrainingPackUI
        └── LoadoutUI
```

---

## Null Safety

### Always Check Wrappers Before Use

```cpp
// CORRECT - Check before use
CarWrapper car = gameWrapper->GetLocalCar();
if (!car) return;  // or if (car.IsNull())

Vector velocity = car.GetVelocity();
```

```cpp
// WRONG - No null check
CarWrapper car = gameWrapper->GetLocalCar();
Vector velocity = car.GetVelocity();  // CRASH if car is null
```

### Chain Null Checks

```cpp
// CORRECT - Check each step
ServerWrapper server = gameWrapper->GetCurrentGameState();
if (!server) return;

BallWrapper ball = server.GetBall();
if (!ball) return;

Vector location = ball.GetLocation();
```

### Use Early Returns

```cpp
void ProcessCar() {
    if (!gameWrapper->IsInGame()) return;

    CarWrapper car = gameWrapper->GetLocalCar();
    if (!car) return;

    // Safe to use car here
}
```

---

## CVar Registration

### Standard CVar Pattern

```cpp
// In onLoad() or SettingsSync constructor
auto enabledCvar = cvarManager->registerCvar(
    "suitespot_enabled",           // Name (prefix with plugin name)
    "1",                            // Default value
    "Enable SuiteSpot auto-load",   // Description
    true,                           // Searchable
    false, 0,                       // No minimum
    false, 0,                       // No maximum
    true                            // Save to config
);
```

### CVar with Range

```cpp
auto delayCvar = cvarManager->registerCvar(
    "suitespot_delay_seconds",
    "3",
    "Delay before auto-loading",
    true,
    true, 0.0f,    // Has minimum: 0
    true, 30.0f,   // Has maximum: 30
    true
);
```

### Change Listeners

```cpp
enabledCvar.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
    bool isEnabled = cvar.getBoolValue();
    LOG("SuiteSpot enabled: {}", isEnabled);
});
```

### Naming Convention

Always prefix CVars with `suitespot_`:
- `suitespot_enabled`
- `suitespot_auto_queue`
- `suitespot_delay_freeplay_sec`
- `suitespot_training_shuffle_enabled`

---

## Event Hooking

### Basic Event Hook

```cpp
void SuiteSpot::LoadHooks() {
    gameWrapper->HookEvent(
        "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        [this](std::string eventName) {
            GameEndedEvent(eventName);
        }
    );
}
```

### Hook with Caller

```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.SetVehicleInput",
    [this](CarWrapper car, void* params, std::string eventName) {
        if (!car) return;
        // Process car input
    }
);
```

### Unhook on Unload

```cpp
void SuiteSpot::onUnload() {
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
}
```

### Common Events

```cpp
// Match lifecycle
"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"
"Function TAGame.AchievementManager_TA.HandleMatchEnded"
"Function TAGame.GameEvent_TA.OnAllTeamsCreated"

// Ball events
"Function TAGame.Ball_TA.OnCarTouch"
"Function TAGame.Ball_TA.OnHitGoal"

// Car events
"Function TAGame.Car_TA.SetVehicleInput"
"Function TAGame.Car_TA.OnGroundChanged"
```

---

## Canvas Rendering

### Register Drawable

```cpp
void SuiteSpot::onLoad() {
    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        RenderOverlay(canvas);
    });
}

void SuiteSpot::RenderOverlay(CanvasWrapper canvas) {
    if (!gameWrapper->IsInGame()) return;

    canvas.SetPosition(Vector2F{100, 100});
    canvas.SetColor(255, 255, 255, 255);
    canvas.DrawString("SuiteSpot Active");
}
```

### World-to-Screen Projection

```cpp
void DrawMarkerAtBall(CanvasWrapper canvas) {
    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) return;

    BallWrapper ball = server.GetBall();
    if (!ball) return;

    Vector2F screenPos = canvas.ProjectF(ball.GetLocation());
    canvas.SetPosition(screenPos);
    canvas.SetColor(255, 0, 0, 255);
    canvas.FillBox(Vector2F{10, 10});
}
```

### Unregister on Unload

```cpp
void SuiteSpot::onUnload() {
    gameWrapper->UnregisterDrawables();
}
```

---

## Memory Management

### Use Smart Pointers for Managers

```cpp
// CORRECT - unique_ptr for owned objects
std::unique_ptr<MapManager> mapManager;

void onLoad() {
    mapManager = std::make_unique<MapManager>(gameWrapper, cvarManager);
}
```

### Shared Pointers for Wrappers

```cpp
// BakkesMod provides shared_ptr wrappers
std::shared_ptr<CVarManagerWrapper> cvarManager;  // From BakkesModPlugin
std::shared_ptr<GameWrapper> gameWrapper;          // From BakkesModPlugin
```

### Don't Store Wrapper Objects Long-Term

```cpp
// WRONG - Storing wrapper that may become invalid
class MyClass {
    CarWrapper storedCar;  // DANGEROUS - may become stale
};

// CORRECT - Fetch fresh each time
CarWrapper GetCurrentCar() {
    return gameWrapper->GetLocalCar();  // Always fresh
}
```

---

## Error Handling

### Use Logging for Debugging

```cpp
#include "logging.h"

// Standard logging
LOG("Loading training packs from: {}", filePath.string());

// Debug logging (only when DEBUG_LOG = true)
DEBUGLOG("Shuffle bag contains {} packs", shuffleBag.size());
```

### Graceful Degradation

```cpp
void LoadTrainingPacks() {
    auto path = GetTrainingPacksPath();

    if (!std::filesystem::exists(path)) {
        LOG("Training packs file not found, using defaults");
        LoadDefaultPacks();
        return;
    }

    try {
        LoadPacksFromFile(path);
    } catch (const std::exception& e) {
        LOG("Failed to load training packs: {}", e.what());
        LoadDefaultPacks();
    }
}
```

### Status Messages for UI

```cpp
// Use StatusMessageUI for user-facing feedback
statusMessage.ShowSuccess("Training packs loaded successfully");
statusMessage.ShowError("Failed to save settings");
```

---

## Naming Conventions

### Files

- Header files: `PascalCase.h` (e.g., `TrainingPackManager.h`)
- Source files: `PascalCase.cpp` (e.g., `TrainingPackManager.cpp`)
- UI files: `*UI.h/cpp` suffix (e.g., `SettingsUI.h`)

### Classes

```cpp
class TrainingPackManager;  // PascalCase
class SettingsUI;
class AutoLoadFeature;
```

### Methods

```cpp
void LoadPacksFromFile();   // PascalCase for public methods
void onLoad();              // camelCase for BakkesMod overrides
void RenderSettings();      // PascalCase for BakkesMod overrides
```

### Variables

```cpp
// Member variables
std::unique_ptr<MapManager> mapManager;      // camelCase
bool isEnabled;
int currentIndex;

// Local variables
auto trainingPacks = GetTrainingPacks();     // camelCase
float delaySeconds = 3.0f;
```

### CVars

```cpp
"suitespot_enabled"              // lowercase with underscores
"suitespot_delay_freeplay_sec"   // plugin prefix
"suitespot_training_shuffle"
```

### Constants

```cpp
constexpr float DEFAULT_DELAY = 3.0f;        // UPPER_SNAKE_CASE
constexpr int MAX_TRAINING_PACKS = 2500;
static const std::string PLUGIN_NAME = "SuiteSpot";
```

---

## Code Organization

### Header Structure

```cpp
#pragma once

// System includes
#include <string>
#include <vector>
#include <memory>

// BakkesMod includes
#include "bakkesmod/plugin/bakkesmodplugin.h"

// Project includes
#include "MapList.h"

// Forward declarations
class SettingsSync;

class MyClass {
public:
    // Public interface first

private:
    // Private members last
};
```

### Include Order

1. Precompiled header (`pch.h`) if used
2. Corresponding header file
3. System/STL headers
4. BakkesMod SDK headers
5. Third-party headers (ImGui, nlohmann/json)
6. Project headers

### Source File Structure

```cpp
#include "pch.h"
#include "MyClass.h"

// Anonymous namespace for file-local helpers
namespace {
    bool IsValidIndex(int index) {
        return index >= 0;
    }
}

// Constructor
MyClass::MyClass() { }

// Public methods
void MyClass::PublicMethod() { }

// Private methods
void MyClass::PrivateHelper() { }
```

---

## ImGui Patterns

### Settings Window Rendering

```cpp
void SettingsUI::RenderSettings() {
    // Always check ImGui context
    if (!ImGui::GetCurrentContext()) return;

    ImGui::Text("SuiteSpot Settings");
    ImGui::Separator();

    // Use CVars for persistence
    auto enabledCvar = cvarManager->getCvar("suitespot_enabled");
    if (enabledCvar) {
        bool enabled = enabledCvar.getBoolValue();
        if (ImGui::Checkbox("Enable Auto-Load", &enabled)) {
            enabledCvar.setValue(enabled);
        }
    }
}
```

### Virtual Scrolling for Large Lists

```cpp
void TrainingPackUI::RenderPackList() {
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(packs.size()));

    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            RenderPackRow(packs[i]);
        }
    }
}
```

### Modal Dialogs

```cpp
if (ImGui::Button("Delete Pack")) {
    ImGui::OpenPopup("Confirm Delete");
}

if (ImGui::BeginPopupModal("Confirm Delete", nullptr,
    ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Are you sure?");

    if (ImGui::Button("Yes")) {
        DeleteSelectedPack();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("No")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
```

### Thread Safety in RenderSettings

```cpp
void SettingsUI::RenderSettings() {
    // RenderSettings runs on render thread, NOT game thread
    // Use gameWrapper->Execute() for game operations

    if (ImGui::Button("Spawn Ball")) {
        gameWrapper->Execute([this](GameWrapper* gw) {
            auto server = gw->GetCurrentGameState();
            if (server) {
                server.SpawnBall(Vector{0, 0, 100}, true, false);
            }
        });
    }
}
```

---

## Anti-Patterns to Avoid

### Don't Store Stale Wrappers

```cpp
// WRONG
class MyPlugin {
    CarWrapper savedCar;  // Will become stale

    void onLoad() {
        savedCar = gameWrapper->GetLocalCar();  // Only valid now
    }
};
```

### Don't Block the Game Thread

```cpp
// WRONG - Blocks game thread
void LoadData() {
    std::this_thread::sleep_for(std::chrono::seconds(5));  // NEVER
}

// CORRECT - Use SetTimeout for delays
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    // Runs after delay without blocking
}, 5.0f);
```

### Don't Forget Null Checks

```cpp
// WRONG
auto car = gameWrapper->GetLocalCar();
car.GetVelocity();  // Crash if null

// CORRECT
auto car = gameWrapper->GetLocalCar();
if (!car) return;
car.GetVelocity();
```

### Don't Mix Thread Contexts

```cpp
// WRONG - Calling game functions from render thread
void RenderSettings() {
    auto server = gameWrapper->GetCurrentGameState();
    server.SpawnBall(...);  // CRASH - wrong thread
}

// CORRECT - Execute on game thread
void RenderSettings() {
    gameWrapper->Execute([](GameWrapper* gw) {
        auto server = gw->GetCurrentGameState();
        if (server) server.SpawnBall(...);
    });
}
```
