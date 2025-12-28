# RBActorWrapper API

## Overview

`RBActorWrapper` is a base class for any actor in the game that is simulated as a **Rigid Body** (which is why it is prefixed with "RB"). This includes objects that have physics properties like mass, velocity, and rotation, such as cars and the ball.

It inherits from `ActorWrapper` and adds more detailed physics controls. Both `CarWrapper` and `BallWrapper` inherit from `RBActorWrapper`.

## API Description

### Header
`bakkesmod/wrappers/GameObject/RBActorWrapper.h`

### Inheritance
`RBActorWrapper` -> `ActorWrapper` -> `ObjectWrapper`

### Key Functions

#### Full Physics State (`RBState`)

The most powerful feature of `RBActorWrapper` is the ability to get and set the entire physics state of an object at once using the `RBState` struct.

-   `RBState GetRBState()`: Returns an `RBState` struct containing the object's complete physics state.
-   `void SetRBState(RBState& NewState)`: Sets the object's physics state from an `RBState` struct.

The `RBState` struct includes:
-   `Vector Location`
-   `Quat Rotation`
-   `Vector LinearVelocity`
-   `Vector AngularVelocity`

#### Applying Forces

-   `void AddForce(Vector& Force, unsigned char ForceMode)`: Applies a linear force to the actor.
-   `void AddTorque(Vector& Torque, unsigned char ForceMode)`: Applies an angular torque to the actor.
    -   `ForceMode`: Can be used to specify the type of force (e.g., impulse vs. continuous), but often passing `0` is sufficient.

#### State Control

-   `void SetFrozen(unsigned long bEnabled)`: Freezes (`1`) or unfreezes (`0`) the actor. When frozen, the actor is not affected by physics.
-   `void SetMaxLinearSpeed(float newMaxLinearSpeed)`: Sets the maximum linear speed the object can travel.
-   `void SetMaxAngularSpeed(float newMaxAngularSpeed)`: Sets the maximum angular speed the object can rotate.

## Example: Teleporting a ball and stopping it

While `ActorWrapper` has `SetLocation`, using `SetRBState` is often better for physics objects as it allows you to modify location and velocity simultaneously, preventing odd physics interactions.

```cpp
BallWrapper ball = server.GetBall();
if (ball)
{
    // Get the current physics state
    RBState ballState = ball.GetRBState();

    // Modify the state
    ballState.Location = {0, 0, 1000}; // Move ball to 1000 units high at center
    ballState.LinearVelocity = {0, 0, 0}; // Stop all movement
    ballState.AngularVelocity = {0, 0, 0}; // Stop all rotation

    // Apply the new state
    ball.SetRBState(ballState);
}
```
