# hook-patterns.md (BakkesMod)

BakkesMod event hooking patterns and best practices.

## Overview

Function hooks are the most powerful feature of BakkesMod. This guide covers proper usage patterns based on the BakkesMod SDK reference.

## Hook Types

### 1. Basic Hooks (No Caller)

```cpp
// Pre-function execution
gameWrapper->HookEvent("Function TAGame.Car_TA.SetVehicleInput",
  [this](std::string eventName) {
    // Code runs BEFORE the function executes
    // eventName contains the function name
});

// Post-function execution
gameWrapper->HookEventPost("Function TAGame.Car_TA.SetVehicleInput",
  [this](std::string eventName) {
    // Code runs AFTER the function returns
});
```

**When to use**:
- When you don't need access to the calling object
- Simple event detection (match ended, goal scored)
- Timing-based operations

### 2. WithCaller Hooks

```cpp
// Pre-function with caller access
gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnHitBall",
  [this](CarWrapper caller, void* params, std::string eventname) {
    if (!caller) { return; }  // Always null check!
    // caller is the CarWrapper that hit the ball
    // params contains function parameters
});

// Post-function with caller access
gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.OnHitBall",
  [this](CarWrapper caller, void* params, std::string eventname) {
    // Code runs after function returns
});
```

**When to use**:
- Need access to the calling wrapper (car, ball, server, etc.)
- Need to read function parameters
- Want to modify caller state

## Determining Caller Type

Function names follow pattern: `Function TAGame.ClassName_TA.MethodName`

**Rules**:
1. Find the class name (ends with `_TA`)
2. Remove `_TA` suffix
3. Look for matching wrapper name

**Examples**:
- `Car_TA` → `CarWrapper`
- `Ball_TA` → `BallWrapper`
- `GameEvent_TA` → `ServerWrapper`
- `VehiclePickup_Boost_TA` → `BoostWrapper`

![Function naming breakdown](https://cdn.bakkesplugins.com/uploads/081d6069-07f5-47e2-9768-1202c996451c-1_functionnaming.png)

## Commonly Hooked Functions

### Match Events

#### Match Ended
```cpp
gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
  [this](std::string eventName) {
    LOG("Match ended!");
    // Only called if match completes normally (no early quit)
});
```

#### Goal Scored
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.OnHitGoal",
  [this](BallWrapper caller, void* params, std::string eventname) {
    if (!caller) { return; }
    LOG("Goal scored!");
    // Note: Also fires during goal replay unless filtered
});
```

#### Ball Exploded
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.Explode",
  [this](BallWrapper caller, void* params, std::string eventname) {
    if (!caller) { return; }
    LOG("Ball exploded (usually means goal)");
});
```

### Car/Ball Interaction

#### Car Hit Ball
```cpp
struct CarBallTouchParams {
    uintptr_t ball;  // Ball memory address
};

gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.OnHitBall",
  [this](CarWrapper caller, void* params, std::string eventname) {
    if (!caller) { return; }
    
    // Cast params to struct
    CarBallTouchParams* touchParams = (CarBallTouchParams*)params;
    BallWrapper ball(touchParams->ball);
    if (!ball) { return; }
    
    LOG("Car {} hit ball", caller.GetOwnerName());
});
```

#### Ball Hit Car
```cpp
struct BallCarTouchParams {
    uintptr_t car;  // Car memory address
};

gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.OnCarTouch",
  [this](BallWrapper caller, void* params, std::string eventname) {
    if (!caller) { return; }
    
    BallCarTouchParams* touchParams = (BallCarTouchParams*)params;
    CarWrapper car(touchParams->car);
    if (!car) { return; }
    
    LOG("Ball touched car");
});
```

### Boost Pickup

```cpp
struct BoostPickupParams {
    uintptr_t car;
};

gameWrapper->HookEventWithCaller<BoostWrapper>("Function TAGame.VehiclePickup_Boost_TA.Pickup",
  [this](BoostWrapper caller, void* params, std::string eventname) {
    if (!caller) { return; }
    
    BoostPickupParams* pickupParams = (BoostPickupParams*)params;
    CarWrapper car(pickupParams->car);
    if (!car) { return; }
    
    LOG("Car picked up boost");
});
```

### Vehicle Input (High Frequency)

```cpp
gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput",
  [this](CarWrapper caller, void* params, std::string eventname) {
    if (!caller) { return; }
    // Fires 120 times per second while playing!
    // Use sparingly - performance impact
});
```

**Warning**: This hook fires at physics tick rate (120 Hz). Only use if absolutely necessary.

## Critical Hook Limitations

### 1. No Duplicate Hooks (CRITICAL)

**Only the FIRST hook callback is executed. Subsequent hooks are SILENTLY IGNORED.**

```cpp
// WRONG: Only first callback runs!
gameWrapper->HookEvent(eventName, [this](...) { 
    LOG("First - this runs"); 
});
gameWrapper->HookEvent(eventName, [this](...) { 
    LOG("Second - IGNORED!"); 
});
```

**CORRECT**: Combine logic in single callback
```cpp
gameWrapper->HookEvent(eventName, [this](...) { 
    LOG("First action");
    LOG("Second action");
    // All logic in one callback
});
```

**Applies to**:
- `HookEvent` - only first callback
- `HookEventPost` - only first callback
- `HookEventWithCaller` - only first callback
- `HookEventWithCallerPost` - only first callback

### 2. Wrapper Null Checking (CRITICAL)

**Always null check wrappers obtained from hooks**:

```cpp
gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnHitBall",
  [this](CarWrapper caller, void* params, std::string eventname) {
    // CRITICAL: Check caller
    if (!caller) { return; }
    
    // CRITICAL: Check params before casting
    if (!params) { return; }
    
    CarBallTouchParams* touchParams = (CarBallTouchParams*)params;
    BallWrapper ball(touchParams->ball);
    
    // CRITICAL: Check constructed wrapper
    if (!ball) { return; }
    
    // Now safe to use
});
```

### 3. Never Store Wrappers from Hooks

**WRONG**:
```cpp
class Plugin {
    CarWrapper storedCar;  // DANGER!
    
    void RegisterHooks() {
        gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnHitBall",
          [this](CarWrapper caller, void* params, std::string eventname) {
            storedCar = caller;  // DANGER! Pointer becomes invalid
        });
    }
    
    void UseLater() {
        storedCar.GetLocation();  // CRASH! Invalid pointer
    }
};
```

**CORRECT**:
```cpp
class Plugin {
    void RegisterHooks() {
        gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnHitBall",
          [this](CarWrapper caller, void* params, std::string eventname) {
            if (!caller) { return; }
            // Use immediately, don't store
            Vector location = caller.GetLocation();
            ProcessLocation(location);  // Store data, not wrapper
        });
    }
};
```

## Parameter Struct Patterns

### Safe Parameter Casting

```cpp
gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnHitBall",
  [this](CarWrapper caller, void* params, std::string eventname) {
    // 1. Check caller
    if (!caller) { return; }
    
    // 2. Check params pointer
    if (!params) { return; }
    
    // 3. Cast to known struct
    struct CarBallTouchParams {
        uintptr_t ball;
    };
    CarBallTouchParams* touchParams = (CarBallTouchParams*)params;
    
    // 4. Construct wrapper from uintptr_t
    BallWrapper ball(touchParams->ball);
    
    // 5. Null check constructed wrapper
    if (!ball) { return; }
    
    // 6. Now safe to use
    Vector ballVelocity = ball.GetVelocity();
});
```

### Finding Unknown Parameters

If parameters aren't documented:

1. Use Function Scanner to find the function
2. Ask in BakkesMod Discord: https://discord.gg/TgVst5QDru
3. Test with dummy struct and observe behavior
4. Check similar function patterns

## Hook Timing: Pre vs Post

### HookEvent (Pre-function)

- Code runs **before** the game function executes
- Can modify state before function processes it
- Cannot see function's results

### HookEventPost (Post-function)

- Code runs **after** the game function returns
- Can see function's results/side effects
- Cannot prevent function from executing

**Example use cases**:

```cpp
// Pre: Modify input before game processes it
gameWrapper->HookEvent("Function TAGame.Car_TA.SetVehicleInput",
  [this](std::string eventName) {
    // Modify car input before game applies it
});

// Post: React to completed action
gameWrapper->HookEventPost("Function TAGame.Car_TA.OnHitBall",
  [this](std::string eventName) {
    // Ball has been hit, update stats
});
```

## Best Practices Summary

1. ✅ **Always null check** caller and params
2. ✅ **Never store wrappers** - use immediately
3. ✅ **Only hook once** per function
4. ✅ **Check params** before casting to struct
5. ✅ **Null check constructed wrappers** from uintptr_t
6. ✅ **Use Post hooks** for reactions to events
7. ✅ **Use Pre hooks** for input modification
8. ❌ **Never assume** params are valid
9. ❌ **Never store** caller/params for later use
10. ❌ **Never hook multiple times** - only first runs

## Integration with Other Patterns

### Execute Pattern (UI Safety)

When calling hooked code from UI:

```cpp
// In UI code
if (ImGui::Button("Trigger Event")) {
  gameWrapper->Execute([this](GameWrapper* gw) {
    // Safe to call game code here
    TriggerMyHookedFunction();
  });
}
```

### SetTimeout Pattern (Deferred Execution)

When needing to wait before executing:

```cpp
gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
  [this](std::string eventName) {
    // Don't load immediately - game will crash!
    gameWrapper->SetTimeout([this](GameWrapper* gw) {
      // Safe to load after delay
      cvarManager->executeCommand("load_training CODE");
    }, 0.5f);  // Minimum 0.5s delay
});
```

## Finding New Functions

### Using Function Scanner

1. Add `-dev` launch option to Rocket League
2. Open console (F6) and type `togglemenu devtools`
3. Use whitelist/blacklist to filter functions
4. Watch for functions firing during gameplay
5. Hook the function name you find

**Tutorial video**: https://www.youtube.com/watch?v=gDZ1wWKE8aw

### Function Naming Patterns

Common patterns:
- `OnHit*` - Collision/touch events
- `Event*` - Major game events
- `Set*` - State changes
- `Add*` / `Remove*` - List modifications
- `Begin*` / `End*` - State transitions

## See Also

- [pre-edit.md](./pre-edit.md) - Hook registration validation
- [post-edit.md](./post-edit.md) - Hook usage audit
- [bakkesmod-context.md](../bakkesmod-context.md) - General patterns
- [BakkesMod SDK Reference](../../docs/bakkesmod-sdk-reference.md) - Full SDK docs
