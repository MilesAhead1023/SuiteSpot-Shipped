# VehicleSimWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

VehicleSimWrapper (Vehicle Simulation) represents the physics engine for a car. It provides direct access to vehicle physics state including throttle, steering, braking, handbrake inputs, torque parameters, and wheel information. Essential for advanced vehicle control and physics manipulation.

## Overview

VehicleSimWrapper gives you access to:
- Input output states (throttle, steer, brake, handbrake)
- Physics parameters (drive torque, brake torque, suspension)
- Wheel information and physics
- Vehicle and car references
- Steering sensitivity
- Ackermannsteering toggle

## Constructor/Initialization

Get an instance via CarWrapper:

```cpp
auto gameWrapper = GetGameWrapper();
auto server = gameWrapper->GetCurrentGameState();
if (!server) return;

// Get first car
auto cars = server->GetCars();
if (cars.Count() > 0) {
    auto car = cars.Get(0);
    if (car) {
        // Access vehicle physics simulator
        auto vehicleSim = car->GetVehicleSim();
        if (vehicleSim) {
            // Use vehicleSim
        }
    }
}

// Or get your own car
auto myCar = gameWrapper->GetLocalCar();
if (myCar) {
    auto vehicleSim = myCar->GetVehicleSim();
    if (vehicleSim) {
        // Use vehicleSim
    }
}
```

## Input Output Methods

These represent what the car is actually doing (after processing input):

```cpp
// Get throttle output (-1.0 to 1.0)
// Negative = reverse, positive = forward
float throttle = vehicleSim->GetOutputThrottle();

// Get steering output (-1.0 to 1.0)
// Negative = left, positive = right
float steer = vehicleSim->GetOutputSteer();

// Get brake output (0.0 to 1.0)
// Only active when not throttling forward
float brake = vehicleSim->GetOutputBrake();

// Get handbrake output (0.0 to 1.0)
// Activates drift/power slide
float handbrake = vehicleSim->GetOutputHandbrake();

// Set throttle output
vehicleSim->SetOutputThrottle(1.0f);    // Full forward
vehicleSim->SetOutputThrottle(-1.0f);   // Full reverse
vehicleSim->SetOutputThrottle(0.0f);    // Coasting

// Set steering output
vehicleSim->SetOutputSteer(0.5f);       // Half right
vehicleSim->SetOutputSteer(-1.0f);      // Full left

// Set brake
vehicleSim->SetOutputBrake(0.5f);       // Half brake

// Set handbrake
vehicleSim->SetOutputHandbrake(1.0f);   // Drift/powerslide
```

## Physics Parameters

### Torque & Braking
```cpp
// Get drive torque (how hard engine accelerates)
float driveTorque = vehicleSim->GetDriveTorque();
vehicleSim->SetDriveTorque(500.0f);     // Typical: 400-600

// Get brake torque (how hard brakes are)
float brakeTorque = vehicleSim->GetBrakeTorque();
vehicleSim->SetBrakeTorque(1000.0f);    // Typical: 800-1200

// Get stop threshold (velocity below which car stops)
float stopThreshold = vehicleSim->GetStopThreshold();
vehicleSim->SetStopThreshold(1.0f);     // Typical: 0.5-2.0

// Get idle brake factor (braking when throttle is neutral)
float idleBrakeFactor = vehicleSim->GetIdleBrakeFactor();
vehicleSim->SetIdleBrakeFactor(0.5f);

// Get opposite brake factor (braking when reversing while going forward)
float oppositeBrakeFactor = vehicleSim->GetOppositeBrakeFactor();
vehicleSim->SetOppositeBrakeFactor(2.0f);
```

### Steering & Control
```cpp
// Get steering sensitivity (how responsive steering is)
float steeringSensitivity = vehicleSim->GetSteeringSensitivity();
vehicleSim->SetSteeringSensitivity(1.0f);      // 1.0 = normal

// Check if using Ackermann steering (more realistic)
bool ackermannSteering = vehicleSim->GetbUseAckermannSteering();
vehicleSim->SetbUseAckermannSteering(true);

// Check if vehicle was attached (physics state)
bool wasAttached = vehicleSim->GetbWasAttached();
vehicleSim->SetbWasAttached(false);
```

## Wheel Access Methods

```cpp
// Get all wheels
ArrayWrapper<WheelWrapper> wheels = vehicleSim->GetWheels();

// Iterate wheels
for (int i = 0; i < wheels.Count(); i++) {
    auto wheel = wheels.Get(i);
    if (!wheel) continue;
    
    // Get wheel properties
    float wheelRadius = wheel->GetWheelRadius();
    float spinSpeed = wheel->GetSpinSpeed();
    int wheelIndex = wheel->GetWheelIndex();
    
    // Check wheel contact
    WheelContactData contact = wheel->GetContact();
    
    // Get wheel location
    Vector wheelLoc = wheel->GetRefWheelLocation();
}

// Example: Get suspension distances for all wheels
float suspensionZ = 0;
for (int i = 0; i < wheels.Count(); i++) {
    auto wheel = wheels.Get(i);
    if (wheel) {
        suspensionZ += wheel->GetSuspensionDistance();
    }
}
float avgSuspension = suspensionZ / wheels.Count();
```

## Vehicle & Car References

```cpp
// Get the vehicle wrapper (parent object)
VehicleWrapper vehicle = vehicleSim->GetVehicle();
if (vehicle) {
    // Access vehicle-level info
}

// Get the car wrapper (if this is a car)
CarWrapper car = vehicleSim->GetCar();
if (car) {
    // Access car-level info
    Vector carLoc = car->GetLocation();
}
```

## Wheel Physics Details

### WheelWrapper Comprehensive Usage

```cpp
auto wheels = vehicleSim->GetWheels();
if (wheels.Count() < 1) return;

auto wheel = wheels.Get(0);
if (!wheel) return;

// Steering
float steerFactor = wheel->GetSteerFactor();        // How much this wheel steers
wheel->SetSteerFactor(1.0f);

// Wheel Properties
float radius = wheel->GetWheelRadius();             // Size of wheel
wheel->SetWheelRadius(17.5f);

int index = wheel->GetWheelIndex();                 // 0=front-left, 1=front-right, 2=rear-left, 3=rear-right

// Suspension Physics
float suspensionStiffness = wheel->GetSuspensionStiffness();
wheel->SetSuspensionStiffness(15000.0f);

float suspensionDampingCompression = wheel->GetSuspensionDampingCompression();
wheel->SetSuspensionDampingCompression(2500.0f);

float suspensionDampingRelaxation = wheel->GetSuspensionDampingRelaxation();
wheel->SetSuspensionDampingRelaxation(2500.0f);

float suspensionTravel = wheel->GetSuspensionTravel();
wheel->SetSuspensionTravel(55.0f);

float suspensionMaxRaise = wheel->GetSuspensionMaxRaise();
wheel->SetSuspensionMaxRaise(55.0f);

float contactForceDistance = wheel->GetContactForceDistance();
wheel->SetContactForceDistance(20.0f);

// Geometry
Vector boneOffset = wheel->GetBoneOffset();         // Offset from car origin
wheel->SetBoneOffset(Vector{0, 0, 0});

Vector presetRestPosition = wheel->GetPresetRestPosition();
wheel->SetPresetRestPosition(Vector{0, -50, 0});

Vector localSuspensionRayStart = wheel->GetLocalSuspensionRayStart();
Vector localRestPosition = wheel->GetLocalRestPosition();

// Friction & Spin
float spinSpeed = wheel->GetSpinSpeed();            // Rotation speed
wheel->SetSpinSpeed(1000.0f);

float spinSpeedDecayRate = wheel->GetSpinSpeedDecayRate();
wheel->SetSpinSpeedDecayRate(0.2f);

float frictionCurveInput = wheel->GetFrictionCurveInput();
wheel->SetFrictionCurveInput(0.5f);

// Acceleration Factors
float aerialThrottleToVelocityFactor = wheel->GetAerialThrottleToVelocityFactor();
wheel->SetAerialThrottleToVelocityFactor(350.0f);

float aerialAccelerationFactor = wheel->GetAerialAccelerationFactor();
wheel->SetAerialAccelerationFactor(0.0f);

// Contact State
WheelContactData contact = wheel->GetContact();
bool hadContact = wheel->GetbHadContact();
wheel->SetbHadContact(true);

// Debug
bool drawDebug = wheel->GetbDrawDebug();
wheel->SetbDrawDebug(true);

// Query suspension distance
float suspensionDistance = wheel->GetSuspensionDistance();

// Query steer input for this wheel
float steerInput = wheel->GetSteer2();

// Get wheel linear velocity
Vector wheelVelocity = wheel->GetLinearVelocity();
```

## Common Patterns

### Monitor Vehicle Input State (Thread-Safe)
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto vehicleSim = car->GetVehicleSim();
    if (!vehicleSim) return;
    
    float throttle = vehicleSim->GetOutputThrottle();
    float steer = vehicleSim->GetOutputSteer();
    float brake = vehicleSim->GetOutputBrake();
    float handbrake = vehicleSim->GetOutputHandbrake();
    
    std::cout << "Throttle: " << throttle << ", Steer: " << steer 
              << ", Brake: " << brake << ", Handbrake: " << handbrake << "\n";
}, 0.016f);  // Every frame
```

### Modify Vehicle Physics (Training/Testing)
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto vehicleSim = car->GetVehicleSim();
    if (!vehicleSim) return;
    
    // Increase acceleration
    vehicleSim->SetDriveTorque(700.0f);
    
    // Increase steering response
    vehicleSim->SetSteeringSensitivity(1.5f);
    
    // Modify brake power
    vehicleSim->SetBrakeTorque(1500.0f);
}, 0.016f);
```

### Read All Wheel Information
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
    std::cout << "Wheel count: " << wheels.Count() << "\n";
    
    for (int i = 0; i < wheels.Count(); i++) {
        auto wheel = wheels.Get(i);
        if (!wheel) continue;
        
        float suspension = wheel->GetSuspensionDistance();
        float spinSpeed = wheel->GetSpinSpeed();
        bool hadContact = wheel->GetbHadContact();
        
        std::cout << "Wheel " << i << ": suspension=" << suspension 
                  << ", spin=" << spinSpeed << ", contact=" << hadContact << "\n";
    }
}, 1.0f);
```

### Compare Vehicle Physics (Comparing Two Cars)
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto cars = server->GetCars();
    if (cars.Count() < 2) return;
    
    auto car1 = cars.Get(0);
    auto car2 = cars.Get(1);
    if (!car1 || !car2) return;
    
    auto vs1 = car1->GetVehicleSim();
    auto vs2 = car2->GetVehicleSim();
    if (!vs1 || !vs2) return;
    
    // Compare throttle
    float throttle1 = vs1->GetOutputThrottle();
    float throttle2 = vs2->GetOutputThrottle();
    
    std::cout << "Car1 throttle: " << throttle1 << ", Car2: " << throttle2 << "\n";
}, 0.5f);
```

## Thread Safety Notes

- **Safe to call from**: Game thread (inside hooks, SetTimeout callbacks)
- **Not safe to call from**: Other threads without synchronization
- **Wrapper lifetime**: Valid as long as car exists; becomes null when car is destroyed
- **Always null-check**: `if (!vehicleSim) return;` before using
- **Null-check wheels**: `if (!wheel) continue;` in wheel iteration loops

```cpp
// SAFE
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto car = gw->GetLocalCar();
    if (!car) return;
    
    auto vehicleSim = car->GetVehicleSim();
    if (!vehicleSim) return;  // NULL CHECK
    
    float throttle = vehicleSim->GetOutputThrottle();
}, 0.016f);

// UNSAFE - Storing wrapper
auto cachedVehicleSim = vehicleSim;
// ... later frame ...
float throttle = cachedVehicleSim->GetOutputThrottle();  // Might crash!
```

## Performance Considerations

- **GetWheels()** creates ArrayWrapper; cache if iterating multiple times
- **Accessing wheels** is efficient; wheels don't change frequently
- **Setting physics parameters** is fast; safe to do frequently
- **Getting input state** is O(1) field access; no performance concern

```cpp
// INEFFICIENT - Creates ArrayWrapper 3 times
int wheelCount1 = vehicleSim->GetWheels().Count();
int wheelCount2 = vehicleSim->GetWheels().Count();
auto wheel = vehicleSim->GetWheels().Get(0);

// EFFICIENT - Creates once
auto wheels = vehicleSim->GetWheels();
int wheelCount = wheels.Count();
auto wheel = wheels.Get(0);
```

## SuiteSpot-Specific Usage

SuiteSpot doesn't directly modify vehicle physics, but you could extend it for training plugins:

```cpp
// Example: Custom training vehicle modifications
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventBegin",
    [this](void* params) {
        auto gw = GetGameWrapper();
        auto car = gw->GetLocalCar();
        if (car) {
            auto vehicleSim = car->GetVehicleSim();
            if (vehicleSim) {
                // Apply custom physics for training
                vehicleSim->SetDriveTorque(600.0f);
                vehicleSim->SetSteeringSensitivity(1.2f);
            }
        }
    }
);
```

## Important Notes for AI Agents

1. **Null-check always** - VehicleSim can be null; always check before using
2. **Wheels are index-based** - Typically 0=FL, 1=FR, 2=RL, 3=RR (front-left, etc.)
3. **Input is output, not desired** - GetOutputThrottle() is what's actually happening
4. **Modifications are game-thread only** - Use SetTimeout() for physics changes
5. **Wheel count is typically 4** - But always use `.Count()` to be safe
6. **Suspension distance** - Positive = compressed, negative = extended (approximate)
7. **Physics parameters are interdependent** - Changing one can affect handling significantly
8. **Spin speed is angular velocity** - Positive/negative indicates rotation direction
9. **Steering sensitivity affects player input** - Doesn't directly affect in-game steering
10. **Ackermann steering is real-world physics** - More realistic but less arcade feel

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/GameObject/CarComponent/VehicleSimWrapper.h
