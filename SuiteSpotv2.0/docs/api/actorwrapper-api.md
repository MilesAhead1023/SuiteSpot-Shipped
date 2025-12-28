# ActorWrapper API

## Overview

`ActorWrapper` is a fundamental base class that provides functionality for any object with a physical presence in the game world (e.g., cars, balls, boost pads). It inherits from `ObjectWrapper` and serves as the parent class for most of the game-object wrappers you will interact with, such as `CarWrapper` and `BallWrapper`.

Understanding `ActorWrapper` is key to manipulating objects in 3D space.

## API Description

### Header
`bakkesmod/wrappers/Engine/ActorWrapper.h`

### Inheritance
`ActorWrapper` -> `ObjectWrapper`

### Key Functions

These functions provide the primary means of interacting with an actor's position, orientation, and movement in the game world.

#### Location & Velocity

-   `Vector GetLocation()`: Returns the actor's location in world space as a `Vector`.
-   `void SetLocation(const Vector location)`: Moves the actor to a new location.
-   `Vector GetVelocity()`: Returns the actor's velocity as a `Vector` (in uu/s).
-   `void SetVelocity(const Vector velocity)`: Sets the actor's velocity to a new value.
-   `void AddVelocity(const Vector velocity)`: Adds the given velocity to the actor's current velocity.

#### Rotation & Torque

-   `Rotator GetRotation()`: Returns the actor's current rotation as a `Rotator` (Pitch, Yaw, Roll).
-   `void SetRotation(const Rotator rotation)`: Sets the actor's rotation to a new value.
-   `Vector GetAngularVelocity()`: Returns the actor's angular velocity as a `Vector`.
-   `void SetAngularVelocity(const Vector v, bool addToCurrent)`: Sets the actor's angular velocity. If `addToCurrent` is true, the new value is added to the existing angular velocity.
-   `void SetTorque(const Vector torq)`: Applies a torque force to the actor.

#### Other Important Functions

-   `void Stop()`: A utility function that sets both linear and angular velocity to zero, effectively stopping the actor.
-   `WorldInfoWrapper GetWorldInfo()`: Returns a wrapper for the `WorldInfo` object, which contains properties of the game world like gravity and time dilation.
-   `ActorWrapper GetOwner()`: Returns the `ActorWrapper` that owns this actor, if any.

### Null-Checking

`ActorWrapper` and its derived classes provide an idiomatic way to check if the wrapper is valid (i.e., its `memory_address` is not 0).

```cpp
CarWrapper car = gameWrapper->GetLocalCar();

// The 'if (car)' check is the preferred way to ensure the wrapper is valid.
// This is equivalent to checking if car.memory_address is not 0.
if (car)
{
    // It is now safe to use the car wrapper.
    car.SetVelocity({0, 0, 500}); // Make the car fly up
}
```

> **Note for AI Agent:** Always use the `if (wrapper)` pattern to null-check a wrapper before using its methods. This prevents game crashes.
