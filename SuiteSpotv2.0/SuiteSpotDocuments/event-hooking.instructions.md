# Type-Safe Event Hooking Patterns

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

Complete reference for safe, type-checked event hooking using HookEventWithCaller - the recommended pattern for all plugin development.

## Why Type-Safe Hooking?

Type-safe event hooking provides:
- ✅ Compile-time type checking
- ✅ Guaranteed caller validity (no null-checks needed)
- ✅ IDE autocomplete support
- ✅ Clearer code intent
- ✅ Fewer runtime crashes from null pointers

## Type-Safe Hook Registration

### Basic Syntax

```cpp
// Hook that runs when function is called
template<typename CallerType>
void HookEventWithCaller(std::string eventName,
    std::function<void(CallerType caller, void* params, std::string eventname)> callback);

// Hook that runs after function executes
template<typename CallerType>
void HookEventWithCallerPost(std::string eventName,
    std::function<void(CallerType caller, void* params, std::string eventname)> callback);
```

### Registration in onLoad()

```cpp
void SuiteSpot::onLoad() {
    // Match end event with ServerWrapper caller
    gameWrapper->HookEventWithCallerPost<ServerWrapper>(
        "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        bind(&SuiteSpot::OnMatchEnded, this, 
             placeholders::_1,  // caller (ServerWrapper)
             placeholders::_2,  // params (void*)
             placeholders::_3)  // eventname (std::string)
    );
    
    // Car hit ball event with CarWrapper caller
    gameWrapper->HookEventWithCaller<CarWrapper>(
        "Function TAGame.Car_TA.EventHitBall",
        bind(&SuiteSpot::OnCarHitBall, this,
             placeholders::_1,  // caller (CarWrapper)
             placeholders::_2,  // params
             placeholders::_3)  // eventname
    );
}
```

## Handler Function Signature

### Standard Signature
```cpp
void HandlerName(CallerType caller, void* params, std::string eventname) {
    // caller is guaranteed non-null
    // params points to function parameters (type depends on event)
    // eventname is the hooked function path
}
```

### With std::bind

```cpp
// In class:
void SuiteSpot::OnSomeEvent(CarWrapper car, void* params, std::string eventname);

// Registration:
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.Jump",
    bind(&SuiteSpot::OnSomeEvent, this, 
         placeholders::_1, placeholders::_2, placeholders::_3)
);

// Implementation:
void SuiteSpot::OnSomeEvent(CarWrapper car, void* params, std::string eventname) {
    // car is GUARANTEED valid
    float boostAmount = car.GetBoostAmount();
    // No null-check needed!
}
```

## Common Event Hook Patterns

### Pattern 1: Match End Detection

```cpp
// Registration:
gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    bind(&SuiteSpot::OnMatchEnded, this, placeholders::_1, 
         placeholders::_2, placeholders::_3));

// Handler:
void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    // server is guaranteed valid here
    
    if (!enabled) return;  // Check plugin state
    
    // Get scores
    int blueScore = server.GetTeamNum(0).GetScore();
    int orangeScore = server.GetTeamNum(1).GetScore();
    LOG("Match ended: Blue " + std::to_string(blueScore) + 
        " Orange " + std::to_string(orangeScore));
    
    // Schedule map load
    std::string mapCode = GetSelectedMapCode();
    gameWrapper->SetTimeout([this, mapCode](GameWrapper* gw) {
        cvarManager->executeCommand("load_freeplay " + mapCode);
    }, delayFreeplaySec);
}
```

### Pattern 2: Ball Physics Tracking

```cpp
// Registration:
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.EventHitBall",
    bind(&SuiteSpot::OnBallHit, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

// Handler:
void SuiteSpot::OnBallHit(BallWrapper ball, void* params, std::string eventname) {
    // ball is guaranteed valid
    
    Vector hitLocation = ball.GetLocation();
    Vector hitVelocity = ball.GetVelocity();
    
    LOG("Ball hit at " + std::to_string(hitLocation.X) + 
        " with velocity " + std::to_string(hitVelocity.magnitude()));
}
```

### Pattern 3: Vehicle Input Monitoring

```cpp
// Registration:
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.SetVehicleInput",
    bind(&SuiteSpot::OnCarInput, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

// Handler:
void SuiteSpot::OnCarInput(CarWrapper car, void* params, std::string eventname) {
    // car is guaranteed valid - the car receiving input
    
    int teamNum = car.GetTeamNum();
    float boostAmount = car.GetBoostAmount();
    bool isInAir = car.IsInAir();
    
    LOG("Car team " + std::to_string(teamNum) + 
        " boost: " + std::to_string(boostAmount) +
        " inAir: " + std::string(isInAir ? "yes" : "no"));
}
```

### Pattern 4: Goal Scored Detection

```cpp
// Registration (POST hook - fires after goal is scored):
gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventGoalScored",
    bind(&SuiteSpot::OnGoalScored, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

// Handler:
void SuiteSpot::OnGoalScored(ServerWrapper server, void* params, std::string eventname) {
    // server is guaranteed valid - goal is already scored
    
    int blueScore = server.GetTeamNum(0).GetScore();
    int orangeScore = server.GetTeamNum(1).GetScore();
    
    LOG("GOAL! Score: Blue " + std::to_string(blueScore) + 
        " Orange " + std::to_string(orangeScore));
    
    // Could trigger celebration overlay, sound, etc.
    ShouldCelebrate = true;
}
```

### Pattern 5: Kickoff Detection

```cpp
// Registration:
gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventKickoff",
    bind(&SuiteSpot::OnKickoff, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

// Handler:
void SuiteSpot::OnKickoff(ServerWrapper server, void* params, std::string eventname) {
    // server is guaranteed valid - kickoff is starting
    
    LOG("Kickoff!");
    
    // Get cars count
    int carCount = server.GetCars().Count();
    LOG("Players ready: " + std::to_string(carCount));
}
```

## Hook Timing: Pre vs Post

```cpp
// PRE-hook: Runs BEFORE the function executes
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.Jump",
    bind(&MyPlugin::OnJumpStart, this, placeholders::_1, 
         placeholders::_2, placeholders::_3));

void OnJumpStart(CarWrapper car, void* params, std::string eventname) {
    LOG("Car jumping (boost before jump: " + 
        std::to_string(car.GetBoostAmount()) + ")");
}

// POST-hook: Runs AFTER the function executes
gameWrapper->HookEventWithCallerPost<CarWrapper>(
    "Function TAGame.Car_TA.Jump",
    bind(&MyPlugin::OnJumpEnd, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

void OnJumpEnd(CarWrapper car, void* params, std::string eventname) {
    LOG("Car jumped (boost after jump: " + 
        std::to_string(car.GetBoostAmount()) + ")");
}
```

## Common Caller Types

```cpp
// GameWrapper-level events
gameWrapper->HookEventWithCaller<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventGoalScored", ...);

// Vehicle events
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.EventHitBall", ...);

// Ball events
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.EventHitBall", ...);

// Player info events
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.ConstructLoadout", ...);

// Team events
gameWrapper->HookEventWithCaller<TeamWrapper>(
    "Function TAGame.Team_TA.SetTeamColor", ...);

// Controller events
gameWrapper->HookEventWithCaller<PlayerControllerWrapper>(
    "Function Engine.PlayerController.IsLocalPlayerController", ...);
```

## Parameters (void* params)

The void* params pointer contains event-specific data. Accessing it requires casting:

```cpp
// Example: Car physics parameters
void SuiteSpot::OnCarInput(CarWrapper car, void* params, std::string eventname) {
    // params usually points to a struct with input data
    // Exact type depends on the hooked function
    // For SetVehicleInput, it contains ControllerInput struct
    
    // Usually, use the wrapper object instead of params
    // The wrapper provides higher-level access
}
```

## Lambda Alternative (Not Recommended for Plugins)

```cpp
// While you CAN use lambdas, std::bind is preferred in plugins
// because plugin lifecycle is better controlled

// Lambda approach (less safe):
gameWrapper->HookEventWithCaller<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    [this](ServerWrapper server, void* params, std::string eventname) {
        // Handle event
    });

// Bind approach (recommended):
gameWrapper->HookEventWithCaller<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    bind(&SuiteSpot::OnMatchEnded, this, 
         placeholders::_1, placeholders::_2, placeholders::_3));
```

## Multiple Hooks on Same Event

```cpp
// Multiple handlers can hook the same event
gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    bind(&SuiteSpot::OnMatchEnded1, this, 
         placeholders::_1, placeholders::_2, placeholders::_3));

gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    bind(&SuiteSpot::OnMatchEnded2, this,
         placeholders::_1, placeholders::_2, placeholders::_3));

// Both handlers execute
void SuiteSpot::OnMatchEnded1(ServerWrapper server, void* params, std::string eventname) {
    // Handle part 1
}

void SuiteSpot::OnMatchEnded2(ServerWrapper server, void* params, std::string eventname) {
    // Handle part 2
}
```

## Best Practices for AI Agents

1. **Always use type-safe hooking** - HookEventWithCaller, not HookEvent
2. **No null-checks for caller** - Type-safe guarantee means it's valid
3. **Check plugin state** - Even if caller is valid, check your enabled flags
4. **Keep handlers lightweight** - Don't block, defer heavy work to SetTimeout
5. **Use bind with placeholders** - Explicit parameter mapping is clearer than lambdas
6. **Cache event handlers** - Register once in onLoad, not repeatedly
7. **Log event names during development** - Helps verify hooks are firing
8. **Test with different game modes** - Some events only fire in specific contexts

## SuiteSpot Pattern Summary

```cpp
// Standard SuiteSpot event pattern:

// 1. In onLoad(), register the hook:
gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    bind(&SuiteSpot::OnMatchEnded, this, placeholders::_1,
         placeholders::_2, placeholders::_3));

// 2. Declare in header:
void OnMatchEnded(ServerWrapper server, void* params, std::string eventname);

// 3. Implement in cpp:
void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    // Check enable state
    if (!enabled) return;
    
    // Check caller validity (guaranteed, but good practice)
    if (!server) return;
    
    // Get game state
    // Schedule delayed actions via SetTimeout
    
    // NO DIRECT game state modification here!
    // Use SetTimeout for safe execution
}
```

## Debugging Hook Registration

```cpp
// Add debug logging to verify hooks fire:
void SuiteSpot::onLoad() {
    LOG("SuiteSpot: Registering hooks...");
    
    gameWrapper->HookEventWithCallerPost<ServerWrapper>(
        "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        bind(&SuiteSpot::OnMatchEnded, this, placeholders::_1,
             placeholders::_2, placeholders::_3));
    
    LOG("SuiteSpot: Hooks registered");
}

void SuiteSpot::OnMatchEnded(ServerWrapper server, void* params, std::string eventname) {
    LOG("SuiteSpot: OnMatchEnded fired!");  // Verify this logs
    // ...
}

// Check BakkesMod console for logs
```

## Important Notes for AI Agents

- **Automatic cleanup** - BakkesMod unhooks everything on plugin unload
- **No UnhookEvent needed** - Usually handled automatically
- **CallerType must match** - Mismatch will cause runtime issues
- **Bind parameters must match** - 3 placeholders for (caller, params, eventname)
- **Post-hooks fire after side effects** - Better for reacting to completed actions
- **Pre-hooks fire before side effects** - Good for preventing/modifying behavior

---

**Last Updated:** 2025-12-14  
**For:** AI Coding Agents (Copilot CLI, Claude, Gemini)  
**Format:** Strictly `.instructions.md` for auto-loading
