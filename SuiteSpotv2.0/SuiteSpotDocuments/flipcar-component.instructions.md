# FlipCarComponentWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

FlipCarComponentWrapper represents the flip/dodge mechanics of a car. It controls jump and flip behavior, including impulse, torque, time, and direction. Use this for analyzing or modifying flip mechanics.

## Overview

FlipCarComponentWrapper gives you access to:
- Flip impulse (upward force)
- Flip torque (rotation force)
- Flip duration/time
- Flip direction (left/right)
- Flip activation and physics

## Constructor/Initialization

Get via CarWrapper:

```cpp
auto gameWrapper = GetGameWrapper();
auto server = gameWrapper->GetCurrentGameState();
if (!server) return;

auto cars = server->GetCars();
if (cars.Count() > 0) {
    auto car = cars.Get(0);
    if (car) {
        auto flipComponent = car->GetFlipComponent();
        if (flipComponent) {
            // Use flip component
        }
    }
}
```

## Flip Properties

### Get/Set Flip Parameters
```cpp
// Get/set impulse (upward force when flip activates)
float flipImpulse = flipComponent->GetFlipCarImpulse();
flipComponent->SetFlipCarImpulse(500.0f);  // Typical: 400-600

// Get/set torque (rotation force)
float flipTorque = flipComponent->GetFlipCarTorque();
flipComponent->SetFlipCarTorque(500.0f);  // Typical: 400-600

// Get/set flip duration (how long flip lasts)
float flipTime = flipComponent->GetFlipCarTime();
flipComponent->SetFlipCarTime(0.5f);  // Typical: 0.4-0.6 seconds

// Get/set flip direction (true = right, false = left)
bool flipRight = flipComponent->GetbFlipRight();
flipComponent->SetbFlipRight(true);
```

## Flip Control Methods

### Activate & Apply
```cpp
// Initialize flip (start the flip)
flipComponent->InitFlip();

// Apply forces during flip (called per frame)
// activeTime = how far into flip we are (0.0 to FlipTime)
flipComponent->ApplyForces(0.25f);  // Mid-flip

// Check if flip can activate
bool canActivate = flipComponent->CanActivate();

// Called when flip is created
flipComponent->OnCreated();
```

## Common Patterns

### Monitor Flip Status
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto flipComp = car->GetFlipComponent();
    if (!flipComp) return;
    
    bool isDodging = car->IsDodging();
    bool hasFlip = car->HasFlip() == 0;  // 0 = has flip
    
    std::cout << "Dodging: " << isDodging << ", Has flip: " << hasFlip << "\n";
}, 0.016f);
```

### Detect Flip Direction
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnJumpPressed",
    [](CarWrapper car, void* params) {
        auto flipComp = car->GetFlipComponent();
        if (flipComp) {
            bool flipRight = flipComp->GetbFlipRight();
            std::cout << (flipRight ? "Flip right" : "Flip left") << "\n";
        }
    }
);
```

### Modify Flip Physics
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto flipComp = car->GetFlipComponent();
    if (!flipComp) return;
    
    // Increase flip force
    flipComp->SetFlipCarImpulse(700.0f);
    flipComp->SetFlipCarTorque(700.0f);
    
    // Make flip faster
    flipComp->SetFlipCarTime(0.3f);
}, 1.0f);
```

## Thread Safety Notes

- **Safe to call from**: Game thread
- **Not safe to call from**: Other threads
- **Wrapper lifetime**: Valid as long as car exists
- **Always null-check**: `if (!flipComponent) return;`

## Performance Considerations

- **ApplyForces()** is called internally each frame
- **Get/set operations** are O(1); safe to call frequently
- **No performance concern** for monitoring flip state

## SuiteSpot-Specific Usage

SuiteSpot doesn't modify flip mechanics, but could extend for training:

```cpp
// Example: Track flip usage
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnJumpPressed",
    [this](CarWrapper car, void* params) {
        // Log jump event
    }
);
```

## Important Notes for AI Agents

1. **Null-check always** - FlipComponent might not exist
2. **Impulse and torque affect feel** - Modifying both changes flip behavior
3. **FlipTime affects skill ceiling** - Shorter time = harder to flip
4. **Direction is binary** - Right/left; stored in single boolean
5. **HasFlip()== 0 means has flip** - Non-zero means no flip available
6. **Flip mechanics are server-authoritative** - Modifications apply locally only
7. **CanActivate() depends on state** - Check before forcing activation

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/GameObject/CarComponent/FlipCarComponentWrapper.h
