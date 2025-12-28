# Jump Component API

## Overview

The `JumpComponentWrapper` is a specialized `CarComponent` that manages the car's initial jump from a surface. It controls the forces and impulses that make up the first jump.

### How to Get a `JumpComponentWrapper`

Similar to the `BoostWrapper`, there is no direct function on `CarWrapper` to get the `JumpComponentWrapper`. You must get the array of default car components and iterate through it, casting each component to find the `JumpComponentWrapper`.

```cpp
CarWrapper car = gameWrapper->GetLocalCar();
if (!car) { return; }

JumpComponentWrapper jumpComponent;
for (CarComponentWrapper component : car.GetDefaultCarComponents())
{
    if (auto castedJump = std::dynamic_pointer_cast<JumpComponentWrapper>(component.get_shared_ptr()))
    {
        jumpComponent = *castedJump;
        break;
    }
}

if (jumpComponent)
{
    // Use the jump component
}
```

## API Description

### Header
`bakkesmod/wrappers/GameObject/CarComponent/JumpComponentWrapper.h`

### Inheritance
`JumpComponentWrapper` -> `CarComponentWrapper` -> ... -> `ObjectWrapper`

### Key Functions

These functions allow for inspection and modification of the physics properties of a car's first jump.

-   `float GetJumpImpulse()`: Returns the initial upward impulse applied when a jump is initiated.
-   `void SetJumpImpulse(float newImpulse)`: Sets the jump impulse.
-   `float GetJumpForce()`: Returns the sustained upward force applied as long as the jump button is held.
-   `void SetJumpForce(float newForce)`: Sets the jump force.
-   `float GetJumpAccel()`: Returns the jump acceleration.
-   `void SetJumpAccel(float newAccel)`: Sets the jump acceleration.
-   `float GetMinJumpTime()`: Returns the minimum time the jump button must be held to get the full jump force.
-   `void SetMinJumpTime(float newTime)`: Sets the minimum jump time.

## Example: Modifying Jump Force

```cpp
void MyPlugin::GiveSuperJump()
{
    CarWrapper car = gameWrapper->GetLocalCar();
    if (!car) { return; }

    // Find the jump component by iterating
    JumpComponentWrapper jumpComponent;
    for (CarComponentWrapper component : car.GetDefaultCarComponents())
    {
        if (auto casted = std::dynamic_pointer_cast<JumpComponentWrapper>(component.get_shared_ptr()))
        {
            jumpComponent = *casted;
            break;
        }
    }
    
    if (jumpComponent)
    {
        // Double the normal jump force
        float currentForce = jumpComponent.GetJumpForce();
        jumpComponent.SetJumpForce(currentForce * 2.0f);
        LOG("Super jump enabled!");
    }
}
```
