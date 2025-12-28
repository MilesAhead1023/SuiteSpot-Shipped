# WheelWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

WheelWrapper represents a single wheel/tire on a car. It provides detailed access to wheel physics, suspension, contact information, and steering. Use this for advanced vehicle physics analysis or modification.

## Overview

WheelWrapper gives you access to:
- Wheel geometry (radius, bone offset, position)
- Suspension properties (stiffness, damping, travel)
- Contact information (ground hit, contact data)
- Spin speed and friction
- Steering factor
- Wheel index (FL, FR, RL, RR)

## Constructor/Initialization

Get via VehicleSimWrapper:

```cpp
auto gameWrapper = GetGameWrapper();
auto server = gameWrapper->GetCurrentGameState();
if (!server) return;

auto cars = server->GetCars();
if (cars.Count() > 0) {
    auto car = cars.Get(0);
    if (car) {
        auto vehicleSim = car->GetVehicleSim();
        if (vehicleSim) {
            auto wheels = vehicleSim->GetWheels();
            for (int i = 0; i < wheels.Count(); i++) {
                auto wheel = wheels.Get(i);
                // Use wheel
            }
        }
    }
}
```

## Wheel Identification

```cpp
// Get wheel index (0=FL, 1=FR, 2=RL, 3=RR typically)
int wheelIndex = wheel->GetWheelIndex();
wheel->SetWheelIndex(0);

std::string wheelName;
switch (wheelIndex) {
    case 0: wheelName = "Front-Left"; break;
    case 1: wheelName = "Front-Right"; break;
    case 2: wheelName = "Rear-Left"; break;
    case 3: wheelName = "Rear-Right"; break;
    default: wheelName = "Unknown"; break;
}
```

## Wheel Geometry

```cpp
// Get wheel radius (size)
float radius = wheel->GetWheelRadius();
wheel->SetWheelRadius(17.5f);  // Typical car tire

// Get bone offset (position relative to car center)
Vector boneOffset = wheel->GetBoneOffset();
wheel->SetBoneOffset(Vector{50, -50, -20});

// Get preset rest position (suspension relaxed)
Vector restPos = wheel->GetPresetRestPosition();
wheel->SetPresetRestPosition(Vector{0, -50, 0});

// Get/set local suspension ray start (suspension collision ray)
Vector suspensionRayStart = wheel->GetLocalSuspensionRayStart();
wheel->SetLocalSuspensionRayStart(Vector{0, 0, 50});

// Get/set local rest position
Vector localRestPos = wheel->GetLocalRestPosition();
wheel->SetLocalRestPosition(Vector{0, 0, 0});
```

## Suspension Physics

```cpp
// Get/set suspension stiffness (spring constant, higher = stiffer)
float stiffness = wheel->GetSuspensionStiffness();
wheel->SetSuspensionStiffness(15000.0f);  // Typical: 10000-20000

// Get/set suspension damping compression (resistance when compressing)
float dampingCompression = wheel->GetSuspensionDampingCompression();
wheel->SetSuspensionDampingCompression(2500.0f);

// Get/set suspension damping relaxation (resistance when extending)
float dampingRelaxation = wheel->GetSuspensionDampingRelaxation();
wheel->SetSuspensionDampingRelaxation(2500.0f);

// Get/set suspension travel distance (how far it can compress)
float suspensionTravel = wheel->GetSuspensionTravel();
wheel->SetSuspensionTravel(55.0f);

// Get/set suspension max raise (how far it can extend)
float maxRaise = wheel->GetSuspensionMaxRaise();
wheel->SetSuspensionMaxRaise(55.0f);

// Get/set contact force distance
float contactForceDistance = wheel->GetContactForceDistance();
wheel->SetContactForceDistance(20.0f);

// Query current suspension distance (compression amount)
float currentSuspensionDistance = wheel->GetSuspensionDistance();
```

## Contact State

```cpp
// Get contact information
WheelContactData contact = wheel->GetContact();
wheel->SetContact(contact);

// Check if had contact this frame
bool hadContact = wheel->GetbHadContact();
wheel->SetbHadContact(true);

// Get wheel reference location (where wheel is physically)
Vector wheelLocation = wheel->GetRefWheelLocation();

// Get wheel linear velocity
Vector wheelVelocity = wheel->GetLinearVelocity();
```

## Steering & Input

```cpp
// Get steer factor (how much this wheel responds to steering input)
float steerFactor = wheel->GetSteerFactor();
wheel->SetSteerFactor(1.0f);  // 1.0 = normal steering response

// Get current steer input amount
float steerInput = wheel->GetSteer2();

// Get friction curve input
float frictionInput = wheel->GetFrictionCurveInput();
wheel->SetFrictionCurveInput(0.5f);

// Get/set aerial throttle to velocity factor
float aerialThrottle = wheel->GetAerialThrottleToVelocityFactor();
wheel->SetAerialThrottleToVelocityFactor(350.0f);

// Get/set aerial acceleration factor
float aerialAccel = wheel->GetAerialAccelerationFactor();
wheel->SetAerialAccelerationFactor(0.0f);
```

## Spin & Friction

```cpp
// Get spin speed (wheel rotation speed)
float spinSpeed = wheel->GetSpinSpeed();
wheel->SetSpinSpeed(1000.0f);

// Get/set spin speed decay rate (how fast spin stops)
float decayRate = wheel->GetSpinSpeedDecayRate();
wheel->SetSpinSpeedDecayRate(0.2f);

// Get vehicle sim reference
VehicleSimWrapper vehicleSim = wheel->GetVehicleSim();
```

## Debug & Misc

```cpp
// Enable debug visualization
bool drawDebug = wheel->GetbDrawDebug();
wheel->SetbDrawDebug(true);

// Trigger contact changed event
wheel->EventContactChanged(wheel);
```

## Common Patterns

### Monitor All Wheel Suspension
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto vehicleSim = car->GetVehicleSim();
    if (!vehicleSim) return;
    
    auto wheels = vehicleSim->GetWheels();
    std::cout << "Wheel suspension distances:\n";
    
    for (int i = 0; i < wheels.Count(); i++) {
        auto wheel = wheels.Get(i);
        if (!wheel) continue;
        
        float suspensionDist = wheel->GetSuspensionDistance();
        std::cout << "  Wheel " << i << ": " << suspensionDist << "\n";
    }
}, 0.5f);
```

### Detect Wheels in Contact
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto vehicleSim = car->GetVehicleSim();
    if (!vehicleSim) return;
    
    auto wheels = vehicleSim->GetWheels();
    int wheelsOnGround = 0;
    
    for (int i = 0; i < wheels.Count(); i++) {
        auto wheel = wheels.Get(i);
        if (wheel && wheel->GetbHadContact()) {
            wheelsOnGround++;
        }
    }
    
    std::cout << wheelsOnGround << " wheels on ground\n";
}, 0.016f);
```

### Modify Suspension Stiffness
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto vehicleSim = car->GetVehicleSim();
    if (!vehicleSim) return;
    
    auto wheels = vehicleSim->GetWheels();
    
    // Make suspension stiffer
    for (int i = 0; i < wheels.Count(); i++) {
        auto wheel = wheels.Get(i);
        if (wheel) {
            wheel->SetSuspensionStiffness(20000.0f);
        }
    }
}, 1.0f);
```

### Wheel Spin Monitoring
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto cars = server->GetCars();
    if (cars.Count() < 1) return;
    
    auto car = cars.Get(0);
    if (!car) return;
    
    auto vehicleSim = car->GetVehicleSim();
    if (!vehicleSim) return;
    
    auto wheels = vehicleSim->GetWheels();
    
    std::cout << car->GetOwnerName() << " wheel spins:\n";
    for (int i = 0; i < wheels.Count(); i++) {
        auto wheel = wheels.Get(i);
        if (wheel) {
            float spin = wheel->GetSpinSpeed();
            std::cout << "  Wheel " << i << ": " << spin << "\n";
        }
    }
}, 1.0f);
```

## Thread Safety Notes

- **Safe to call from**: Game thread
- **Not safe to call from**: Other threads
- **Wrapper lifetime**: Valid as long as car exists
- **Always null-check**: `if (!wheel) continue;` in loops

## Performance Considerations

- **GetWheels()** creates ArrayWrapper; cache if iterating multiple times
- **Per-wheel operations** are fast; O(1)
- **Safe to modify all wheels** every frame

## SuiteSpot-Specific Usage

SuiteSpot doesn't access wheel data, but could extend for training:

```cpp
// Example: Log wheel contact during training
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (car) {
        auto vehicleSim = car->GetVehicleSim();
        if (vehicleSim) {
            auto wheels = vehicleSim->GetWheels();
            // Track wheel contact for training metrics
        }
    }
}, 0.5f);
```

## Important Notes for AI Agents

1. **Wheel indices are typically 0-3** - FL, FR, RL, RR
2. **Suspension distance is compression** - Positive = compressed, negative = extended
3. **Stiffness affects bounce** - Higher stiffness = less bounce
4. **Damping prevents oscillation** - Balance between compression and relaxation
5. **Steer factor is per-wheel** - Front wheels typically have factor, rear have 0
6. **Spin speed is rotation** - Positive/negative indicates direction
7. **Contact data is complex** - Contains detailed collision information
8. **Wheel radius is fixed** - Doesn't change during gameplay
9. **Suspension travel is max compression** - Hard limit on how much wheel can compress
10. **Vehicle sim reference** - Can access owning VehicleSim from wheel

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/GameObject/CarComponent/WheelWrapper.h
