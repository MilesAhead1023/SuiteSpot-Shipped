# Thread Safety Guide

This document explains threading patterns and safety requirements for BakkesMod plugin development.

## Table of Contents

1. [Thread Model Overview](#thread-model-overview)
2. [The Execute Pattern](#the-execute-pattern)
3. [The SetTimeout Pattern](#the-settimeout-pattern)
4. [RenderSettings Thread Safety](#rendersettings-thread-safety)
5. [LoadoutManager Example](#loadoutmanager-example)
6. [Common Threading Mistakes](#common-threading-mistakes)
7. [Synchronization Primitives](#synchronization-primitives)

---

## Thread Model Overview

BakkesMod plugins operate across multiple threads:

```
┌─────────────────────────────────────────────────────────┐
│                     Game Thread                          │
│  - onLoad(), onUnload()                                  │
│  - Event hooks (HookEvent callbacks)                     │
│  - SetTimeout callbacks                                  │
│  - Execute() callbacks                                   │
│  - All game object manipulation                          │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                    Render Thread                         │
│  - Render() (PluginWindow)                               │
│  - RenderSettings() (PluginSettingsWindow)               │
│  - RegisterDrawable callbacks                            │
│  - All ImGui operations                                  │
└─────────────────────────────────────────────────────────┘
```

### Critical Rule

**Never call game object methods (CarWrapper, BallWrapper, ServerWrapper, etc.) directly from the render thread. Always use `gameWrapper->Execute()` to marshal calls to the game thread.**

---

## The Execute Pattern

`gameWrapper->Execute()` runs a lambda on the game thread from any other thread.

### When to Use

- Inside `RenderSettings()` when you need to interact with game objects
- Inside `Render()` when you need to call game functions
- From any callback that might run on a non-game thread
- When spawning objects, modifying game state, or reading game data for mutations

### Basic Usage

```cpp
void MyPlugin::RenderSettings() {
    if (ImGui::Button("Spawn Ball")) {
        // Execute runs this lambda on the game thread
        gameWrapper->Execute([this](GameWrapper* gw) {
            auto server = gw->GetCurrentGameState();
            if (!server) return;

            server.SpawnBall(Vector{0, 0, 100}, true, false);
        });
    }
}
```

### Reading Game Data Safely

```cpp
void MyPlugin::RenderSettings() {
    // WRONG - Direct access from render thread
    // auto car = gameWrapper->GetLocalCar();
    // float speed = car.GetVelocity().magnitude();

    // CORRECT - Cache data via Execute, display cached value
    if (needsUpdate) {
        gameWrapper->Execute([this](GameWrapper* gw) {
            auto car = gw->GetLocalCar();
            if (car) {
                cachedSpeed = car.GetVelocity().magnitude();
            }
        });
        needsUpdate = false;
    }

    ImGui::Text("Speed: %.1f", cachedSpeed);
}
```

### Execute is Asynchronous

`Execute()` queues the lambda for execution - it doesn't block:

```cpp
gameWrapper->Execute([](GameWrapper* gw) {
    // This runs later, on the game thread
    LOG("This prints second");
});
LOG("This prints first");  // Execute returns immediately
```

---

## The SetTimeout Pattern

`gameWrapper->SetTimeout()` schedules a callback to run after a delay on the game thread.

### Basic Usage

```cpp
void AutoLoadFeature::OnMatchEnded() {
    float delaySeconds = 3.0f;

    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        // This runs 3 seconds later, on the game thread
        LoadNextMap();
    }, delaySeconds);
}
```

### Chaining Timeouts

```cpp
void LoadSequence() {
    // Step 1: Load training pack
    cvarManager->executeCommand("load_training ABCD-1234-5678");

    // Step 2: After 2 seconds, queue for match
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        if (autoQueueEnabled) {
            cvarManager->executeCommand("queue");
        }
    }, 2.0f);
}
```

### Cancellation Pattern

SetTimeout doesn't return a handle for cancellation. Use a flag:

```cpp
class MyPlugin {
    std::atomic<bool> shouldContinue{true};

    void StartDelayedAction() {
        shouldContinue = true;
        gameWrapper->SetTimeout([this](GameWrapper* gw) {
            if (shouldContinue) {
                DoAction();
            }
        }, 5.0f);
    }

    void CancelAction() {
        shouldContinue = false;
    }
};
```

---

## RenderSettings Thread Safety

`RenderSettings()` runs on the render thread. Special care is required.

### Safe Operations (Direct)

```cpp
void RenderSettings() {
    // ImGui operations - SAFE
    ImGui::Text("Hello");
    ImGui::Checkbox("Enable", &enabled);

    // CVarManager operations - SAFE (thread-safe internally)
    auto cvar = cvarManager->getCvar("suitespot_enabled");
    if (cvar) {
        bool val = cvar.getBoolValue();
        if (ImGui::Checkbox("Enabled", &val)) {
            cvar.setValue(val);
        }
    }

    // Logging - SAFE
    LOG("Rendering settings");
}
```

### Unsafe Operations (Need Execute)

```cpp
void RenderSettings() {
    // WRONG - Game object access on render thread
    auto car = gameWrapper->GetLocalCar();  // UNSAFE
    auto server = gameWrapper->GetCurrentGameState();  // UNSAFE

    // CORRECT - Use Execute for game operations
    if (ImGui::Button("Demolish Car")) {
        gameWrapper->Execute([](GameWrapper* gw) {
            auto car = gw->GetLocalCar();
            if (car) {
                car.Demolish();
            }
        });
    }
}
```

### Pattern: Cache and Display

```cpp
class SettingsUI {
    // Cached data (updated from game thread)
    std::string playerName;
    int currentScore = 0;
    std::mutex cacheMutex;

    void UpdateCache() {
        gameWrapper->Execute([this](GameWrapper* gw) {
            auto pri = gw->GetLocalCar().GetPRI();
            if (!pri) return;

            std::lock_guard<std::mutex> lock(cacheMutex);
            playerName = pri.GetPlayerName().ToString();
            currentScore = pri.GetMatchScore();
        });
    }

    void RenderSettings() {
        if (ImGui::Button("Refresh")) {
            UpdateCache();
        }

        std::lock_guard<std::mutex> lock(cacheMutex);
        ImGui::Text("Player: %s", playerName.c_str());
        ImGui::Text("Score: %d", currentScore);
    }
};
```

---

## LoadoutManager Example

SuiteSpot's `LoadoutManager` demonstrates proper thread-safe design:

```cpp
// LoadoutManager.h
class LoadoutManager {
public:
    void RefreshLoadoutCache();
    std::vector<std::string> GetLoadoutNames() const;
    void SwitchLoadout(int index);
    bool IsReady() const;

private:
    std::vector<std::string> loadoutNames;
    mutable std::mutex cacheMutex;
    std::atomic<bool> isReady{false};

    std::shared_ptr<GameWrapper> gameWrapper;
};
```

```cpp
// LoadoutManager.cpp
void LoadoutManager::RefreshLoadoutCache() {
    // This can be called from any thread
    gameWrapper->Execute([this](GameWrapper* gw) {
        // Now we're on the game thread - safe to access game objects
        auto items = gw->GetItemsWrapper();
        if (!items) return;

        std::vector<std::string> names;
        // ... populate names from game data ...

        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            loadoutNames = std::move(names);
        }

        isReady = true;
    });
}

std::vector<std::string> LoadoutManager::GetLoadoutNames() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return loadoutNames;  // Return copy, safe for any thread
}

void LoadoutManager::SwitchLoadout(int index) {
    // Must execute on game thread
    gameWrapper->Execute([this, index](GameWrapper* gw) {
        // Safe to call game functions here
        auto items = gw->GetItemsWrapper();
        if (!items) return;

        // ... switch loadout ...
    });
}
```

### Usage from RenderSettings

```cpp
void LoadoutUI::RenderSettings() {
    if (!loadoutManager->IsReady()) {
        ImGui::Text("Loading loadouts...");
        return;
    }

    auto names = loadoutManager->GetLoadoutNames();  // Thread-safe

    if (ImGui::BeginCombo("Loadout", currentLoadout.c_str())) {
        for (size_t i = 0; i < names.size(); i++) {
            if (ImGui::Selectable(names[i].c_str())) {
                loadoutManager->SwitchLoadout(i);  // Internally uses Execute
            }
        }
        ImGui::EndCombo();
    }
}
```

---

## Common Threading Mistakes

### Mistake 1: Direct Game Access from Render Thread

```cpp
// WRONG
void RenderSettings() {
    auto car = gameWrapper->GetLocalCar();
    ImGui::Text("Speed: %.1f", car.GetVelocity().magnitude());
}

// CORRECT
void RenderSettings() {
    // Use cached value, updated via Execute
    ImGui::Text("Speed: %.1f", cachedSpeed);
}
```

### Mistake 2: Blocking the Game Thread

```cpp
// WRONG - Never block the game thread
void onLoad() {
    std::this_thread::sleep_for(std::chrono::seconds(5));  // BLOCKS GAME
}

// CORRECT - Use SetTimeout for delays
void onLoad() {
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        // Runs after 5 seconds without blocking
    }, 5.0f);
}
```

### Mistake 3: Race Conditions with Shared State

```cpp
// WRONG - No synchronization
class MyPlugin {
    std::vector<std::string> data;

    void LoadData() {
        gameWrapper->Execute([this](GameWrapper*) {
            data.push_back("item");  // Race condition!
        });
    }

    void RenderSettings() {
        for (auto& item : data) {  // Race condition!
            ImGui::Text("%s", item.c_str());
        }
    }
};

// CORRECT - Use mutex
class MyPlugin {
    std::vector<std::string> data;
    std::mutex dataMutex;

    void LoadData() {
        gameWrapper->Execute([this](GameWrapper*) {
            std::lock_guard<std::mutex> lock(dataMutex);
            data.push_back("item");
        });
    }

    void RenderSettings() {
        std::lock_guard<std::mutex> lock(dataMutex);
        for (auto& item : data) {
            ImGui::Text("%s", item.c_str());
        }
    }
};
```

### Mistake 4: Assuming Execute is Synchronous

```cpp
// WRONG - Expecting immediate result
void DoSomething() {
    bool result = false;
    gameWrapper->Execute([&result](GameWrapper* gw) {
        result = gw->IsInGame();
    });
    // result is still false here! Execute is async
    if (result) { /* never reached */ }
}

// CORRECT - Use callback or atomic
void DoSomething() {
    gameWrapper->Execute([this](GameWrapper* gw) {
        if (gw->IsInGame()) {
            HandleInGame();  // Do work inside Execute
        }
    });
}
```

---

## Synchronization Primitives

### std::mutex

Use for protecting shared data:

```cpp
class ThreadSafeCache {
    std::vector<std::string> items;
    mutable std::mutex mutex;

public:
    void Add(const std::string& item) {
        std::lock_guard<std::mutex> lock(mutex);
        items.push_back(item);
    }

    std::vector<std::string> GetAll() const {
        std::lock_guard<std::mutex> lock(mutex);
        return items;  // Return copy
    }
};
```

### std::atomic

Use for simple flags and counters:

```cpp
class MyPlugin {
    std::atomic<bool> isLoading{false};
    std::atomic<int> processedCount{0};

    void StartLoading() {
        isLoading = true;
        gameWrapper->Execute([this](GameWrapper*) {
            // ... load data ...
            processedCount++;
            isLoading = false;
        });
    }

    void RenderSettings() {
        if (isLoading) {
            ImGui::Text("Loading... (%d processed)", processedCount.load());
        }
    }
};
```

### When to Use Which

| Scenario | Use |
|----------|-----|
| Simple bool/int flags | `std::atomic<T>` |
| Complex data structures | `std::mutex` |
| Read-heavy, rare writes | `std::shared_mutex` |
| One-time initialization | `std::call_once` |

---

## Quick Reference

### Safe on Render Thread

- ImGui calls
- CVarManager operations
- Logging
- Reading atomic/mutex-protected cached data

### Requires Execute()

- Any GameWrapper getter (GetLocalCar, GetCurrentGameState, etc.)
- Any game object method (CarWrapper, BallWrapper, etc.)
- Spawning or modifying game objects
- Reading game state for mutations

### Thread-Safe by Design

- `CVarManagerWrapper` - internal synchronization
- `LOG()` / `DEBUGLOG()` - safe to call anywhere
- `std::atomic<T>` variables
- Data protected by mutex

### Not Thread-Safe

- Direct game object access from render thread
- Unprotected shared containers
- Storing wrappers across threads
