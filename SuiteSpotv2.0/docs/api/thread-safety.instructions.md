# Thread Safety and Advanced Patterns

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

Critical patterns for thread-safe game state access and advanced plugin development.

## Thread Safety Fundamentals

BakkesMod runs the game on the **game thread**. Your plugin code runs on **multiple threads**:
- Event handlers: game thread
- ImGui rendering: game thread
- SetTimeout callbacks: game thread
- Direct code: may be main thread

**RULE:** Never access game state from unknown thread context.

## Safe vs Unsafe Access Patterns

### ❌ UNSAFE: Direct Access (Unknown Thread)

```cpp
// BAD - may crash if called from wrong thread
ServerWrapper server = gameWrapper->GetCurrentGameState();
if (server) {
    ArrayWrapper<CarWrapper> cars = server.GetCars();  // CRASH
}
```

### ✅ SAFE: SetTimeout Pattern

```cpp
// GOOD - guaranteed game thread execution
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    ServerWrapper server = gw->GetCurrentGameState();
    if (server) {
        ArrayWrapper<CarWrapper> cars = server.GetCars();  // SAFE
    }
}, 0.0f);  // 0 delay = as soon as possible
```

### ✅ SAFE: Execute Pattern

```cpp
// GOOD - immediate game thread execution
gameWrapper->Execute([this](GameWrapper* gw) {
    ServerWrapper server = gw->GetCurrentGameState();
    if (server) {
        server.GetCars();  // SAFE
    }
});
```

### ✅ SAFE: Event Hook Pattern

```cpp
// GOOD - event handlers run on game thread
gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    bind(&SuiteSpot::OnMatchEnded, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    // Already on game thread
    server.GetCars();  // SAFE
}
```

## SetTimeout vs Execute

### SetTimeout - Delayed Execution

```cpp
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    // Runs after delay
    cvarManager->executeCommand("load_freeplay Stadium_P");
}, 2.0f);  // Runs in 2 seconds
```

**Use SetTimeout when:**
- ✅ You need a delay (match loading, etc.)
- ✅ You're in an event handler and want deferred execution
- ✅ You want lightweight timing control
- ✅ SuiteSpot's primary pattern for auto-load delays

### Execute - Immediate Execution

```cpp
gameWrapper->Execute([this](GameWrapper* gw) {
    // Runs immediately on game thread
    ServerWrapper server = gw->GetCurrentGameState();
});
```

**Use Execute when:**
- ✅ You need immediate thread-safe access
- ✅ You're in ImGui callback and need to query state
- ✅ You can't afford to wait for a delay

## SuiteSpot's Thread-Safe Pattern

```cpp
// Example: Auto-load with delays

void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    // Already on game thread - but use SetTimeout for safety and timing
    
    if (!enabled) return;
    
    std::string mapCode = GetSelectedMapCode();
    
    // Schedule load with delay
    gameWrapper->SetTimeout([this, mapCode](GameWrapper* gw) {
        // Now: on game thread + after delayFreeplaySec
        cvarManager->executeCommand("load_freeplay " + mapCode);
        
        // If auto-queue, schedule queue
        if (autoQueue) {
            gameWrapper->SetTimeout([this](GameWrapper* gw) {
                // Now: on game thread + after delayQueueSec more
                cvarManager->executeCommand("queue");
            }, delayQueueSec);
        }
    }, delayFreeplaySec);
}
```

## Global Vector Thread Safety

SuiteSpot uses global vectors (RLMaps, RLTraining, RLWorkshop). These are shared state:

```cpp
// Global (MapList.h)
extern std::vector<TrainingEntry> RLTraining;

// Usage in SuiteSpot - safe pattern
void SuiteSpot::SaveTrainingMaps() {
    // Write global vector to file
    std::ofstream file(GetTrainingFilePath());
    
    // Safe because called from SetTimeout or onLoad
    for (const auto& entry : RLTraining) {
        file << entry.code << "," << entry.name << "\n";
    }
}

// Loading from file
void SuiteSpot::LoadTrainingMaps() {
    RLTraining.clear();  // Clear before loading
    
    std::ifstream file(GetTrainingFilePath());
    std::string line;
    while (std::getline(file, line)) {
        TrainingEntry entry;
        // Parse line
        RLTraining.push_back(entry);  // Add to vector
    }
}
```

**Thread-safety considerations:**
- ✅ Safe when: called from plugin startup (onLoad)
- ✅ Safe when: called from event handlers
- ✅ Safe when: called from SetTimeout/Execute
- ❌ Unsafe when: called from ImGui rendering during load
- ❌ Unsafe when: accessed simultaneously from multiple threads

**Best practice:** Load/save on game thread, cache locally in settings UI if needed.

## CVar Value Changes (Thread Safety)

```cpp
// Registration in onLoad - SAFE
cvarManager->registerCvar("suitespot_enabled", "1", "Enable SuiteSpot", ...)
    .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        // This callback runs on game thread
        enabled = cvar.getBoolValue();  // Safe to update member
        
        // If you need to access game state, use SetTimeout
        gameWrapper->SetTimeout([this](GameWrapper* gw) {
            ServerWrapper server = gw->GetCurrentGameState();
            // Safe now
        }, 0.0f);
    });
```

## ImGui and Game State

```cpp
// In RenderSettings - ImGui runs on game thread, but be careful

void SuiteSpot::RenderSettings() {
    // ❌ NOT SAFE - direct access
    // ServerWrapper server = gameWrapper->GetCurrentGameState();
    
    // ✅ SAFE - query cached state
    if (ImGui::Button("Get Game Info")) {
        gameWrapper->Execute([this](GameWrapper* gw) {
            ServerWrapper server = gw->GetCurrentGameState();
            if (server) {
                currentGameInfo = server.GetTeamNum(0).GetScore();
            }
        });
    }
    
    // Display cached info
    ImGui::Text("Score: %d", currentGameInfo);
}
```

## Lambda Capture Thread Safety

```cpp
// ❌ CAREFUL - Capturing raw pointers/references
gameWrapper->SetTimeout([this, &mapCode](GameWrapper* gw) {
    // mapCode reference might be invalid!
}, 5.0f);

// ✅ SAFE - Capture by value
std::string mapCode = "Stadium_P";
gameWrapper->SetTimeout([this, mapCode](GameWrapper* gw) {
    // mapCode is copied - safe
    cvarManager->executeCommand("load_freeplay " + mapCode);
}, 5.0f);

// ✅ SAFE - Capture shared_ptr
auto window = std::make_shared<MyWindow>();
gameWrapper->SetTimeout([this, window](GameWrapper* gw) {
    // window.get() is safe
}, 5.0f);

// ❌ DANGEROUS - Capturing raw this pointer across unload
// If plugin unloads while SetTimeout is pending, 'this' is invalid!
// Solution: Use shared_ptr if timeout spans plugin lifecycle
```

## Event Handler Thread Safety

```cpp
// Type-safe event handlers run on game thread - SAFE

gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.EventHitBall",
    bind(&SuiteSpot::OnCarHitBall, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

void SuiteSpot::OnCarHitBall(CarWrapper car, void* params, std::string eventname) {
    // Already on game thread - safe to:
    Vector pos = car.GetLocation();           // Direct access
    
    // Access global vectors
    if (RLTraining.size() > 0) { ... }        // Safe
    
    // Access member variables
    if (enabled) { ... }                      // Safe
    
    // For delayed operations, use SetTimeout
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        // More safe access after delay
    }, 0.5f);
}
```

## File I/O Thread Safety

```cpp
// File I/O is slow - should be thread-safe but avoid on game thread

// ✅ SAFE in onLoad (startup)
void SuiteSpot::onLoad() {
    LoadTrainingMaps();  // OK, game not running yet
}

// ✅ SAFE in SetTimeout (deferred)
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    SaveTrainingMaps();  // OK, deferred execution
}, 1.0f);

// ❌ PROBLEMATIC in event handlers (blocks game)
void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    SaveTrainingMaps();  // SLOW - blocks game momentarily
    // Better: defer to SetTimeout
}

// ✅ BETTER - Defer file I/O
void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
        SaveTrainingMaps();  // OK, won't block as much
    }, 0.1f);
}
```

## Wrapper Lifetime Management

Wrappers can become invalid:

```cpp
// ❌ UNSAFE - Storing wrapper across time
class BadPattern {
    ServerWrapper cachedServer;  // DANGEROUS!
    
    void StoreServer() {
        cachedServer = gameWrapper->GetCurrentGameState();
    }
    
    void UseLater() {
        if (!cachedServer) return;  // Might be invalid!
        cachedServer.GetCars();     // CRASH possible
    }
};

// ✅ SAFE - Get fresh wrapper each use
class GoodPattern {
    void OnUpdate() {
        ServerWrapper server = gameWrapper->GetCurrentGameState();
        if (!server) return;
        server.GetCars();  // Fresh, safe
    }
};

// ✅ SAFE - Get wrapper fresh in SetTimeout
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    ServerWrapper server = gw->GetCurrentGameState();
    if (!server) return;
    // server is fresh and valid
}, 0.0f);
```

## Multiple Concurrent SetTimeouts

```cpp
// Safe to have multiple pending SetTimeouts

gameWrapper->SetTimeout([this](GameWrapper* gw) {
    LOG("First timeout fired");
}, 1.0f);

gameWrapper->SetTimeout([this](GameWrapper* gw) {
    LOG("Second timeout fired");
}, 2.0f);

gameWrapper->SetTimeout([this](GameWrapper* gw) {
    LOG("Third timeout fired");
}, 0.5f);

// They execute in time order:
// 0.5s - Third
// 1.0s - First
// 2.0s - Second
```

## Plugin Unload Safety

```cpp
// BakkesMod automatically unhooks events on unload

void SuiteSpot::onUnload() {
    // Event hooks automatically removed
    // CVars automatically unregistered
    // Drawables automatically removed
    
    // You typically don't need to do cleanup
    // but you can add explicit cleanup if needed:
    
    SaveTrainingMaps();  // Safe - still on game thread
}

// ⚠️ DANGER: SetTimeout pending after unload
// If a SetTimeout is still pending when plugin unloads:
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    // 'this' is invalid if plugin unloaded!
    // CRASH
}, 5.0f);

// Solution: Use shared_ptr or ensure no pending SetTimeouts
```

## SuiteSpot Thread-Safety Checklist

When modifying SuiteSpot, ensure:

- ✅ Event handlers access game state safely
- ✅ SetTimeout used for all delayed operations
- ✅ Global vectors not accessed from unknown thread
- ✅ File I/O deferred with SetTimeout
- ✅ Lambda captures are by-value or shared_ptr
- ✅ CVars registered in onLoad, not other threads
- ✅ Member variables updated in callbacks, not from arbitrary thread
- ✅ Wrappers not stored across frames
- ✅ SetTimeout doesn't hold game state too long
- ✅ ImGui doesn't block on game state queries

## Anti-Patterns to Avoid

```cpp
// ❌ 1: Direct access from unknown thread
ServerWrapper server = gameWrapper->GetCurrentGameState();
if (server) { ... }

// ✅ Use SetTimeout or Execute instead

// ❌ 2: Storing wrapper references
ServerWrapper cachedServer = gameWrapper->GetCurrentGameState();
// Use later...

// ✅ Get fresh wrapper each time

// ❌ 3: Long-running operations in event handler
void OnMatchEnded(ServerWrapper server, ...) {
    for (int i = 0; i < 1000000; ++i) {  // BLOCKS GAME
        // slow work
    }
}

// ✅ Defer with SetTimeout

// ❌ 4: Accessing global vectors without synchronization
std::thread t([this] {
    RLTraining.push_back(entry);  // RACE CONDITION
});

// ✅ Only modify from game thread

// ❌ 5: Circular references with shared_ptr
auto handler = std::make_shared<MyHandler>();
handler->ptr = handler;  // MEMORY LEAK - circular reference

// ✅ Use std::weak_ptr if needed, or avoid
```

## Debugging Thread Issues

```cpp
// Add logging to understand thread context

void SuiteSpot::LogThreadInfo() {
    auto threadId = std::this_thread::get_id();
    LOG("Running on thread: " + std::to_string(
        std::hash<std::thread::id>{}(threadId)));
}

// Log at key points:
void SuiteSpot::onLoad() {
    LOG("onLoad thread ID: ");
    LogThreadInfo();
}

void SuiteSpot::RenderSettings() {
    // LOG("RenderSettings thread ID: ");
    // LogThreadInfo();  // Don't log every frame!
}

void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    LOG("OnMatchEnded thread ID: ");
    LogThreadInfo();
}
```

## Performance Notes

```cpp
// SetTimeout has slight overhead but worth it

// Fast path (0 delay)
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    // Runs as soon as game thread available
}, 0.0f);  // Almost immediate

// Reasonable delay
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    // Runs after delay
}, 1.0f);  // 1 second

// For SuiteSpot:
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    cvarManager->executeCommand("load_freeplay " + mapCode);
}, delayFreeplaySec);  // User-configured delay (0-300s)
```

## Summary for AI Agents

1. **Always use SetTimeout or Execute** for game state access
2. **Event handlers run on game thread** - direct access OK there
3. **Store SetTimeout delays in variables** - configurable timing
4. **Capture by value** - avoid reference invalidation
5. **Get fresh wrappers** - don't store across frames
6. **Check null always** - wrappers can become invalid
7. **Defer file I/O** - use SetTimeout to avoid game thread blocking
8. **Test with SetTimeout** - verify timing and state access

---

**Last Updated:** 2025-12-14  
**For:** AI Coding Agents (Copilot CLI, Claude, Gemini)  
**Format:** Strictly `.instructions.md` for auto-loading
