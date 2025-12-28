# Dodge Component API

## Overview

The `DodgeComponentWrapper` is a specialized `CarComponent` that manages the physics of a car's dodge (the second jump). It controls the directional impulses and torques that are applied when a car dodges forward, sideways, or backward.

### How to Get a `DodgeComponentWrapper`

Similar to the `BoostWrapper` and `JumpComponentWrapper`, you must get the array of default car components and iterate through it, casting each component to find the `DodgeComponentWrapper`.

```cpp
CarWrapper car = gameWrapper->GetLocalCar();
if (!car) { return; }

DodgeComponentWrapper dodgeComponent;
for (CarComponentWrapper component : car.GetDefaultCarComponents())
{
    if (auto castedDodge = std::dynamic_pointer_cast<DodgeComponentWrapper>(component.get_shared_ptr()))
    {
        dodgeComponent = *castedDodge;
        break;
    }
}

if (dodgeComponent)
{
    // Use the dodge component
}
```

## API Description

### Header
`bakkesmod/wrappers/GameObject/CarComponent/DodgeComponentWrapper.h`

### Inheritance
`DodgeComponentWrapper` -> `CarComponentWrapper` -> ... -> `ObjectWrapper`

### Key Functions

These functions allow for inspection and modification of the physics properties of a car's dodge.

#### Dodge Impulse (Force)

-   `float GetForwardDodgeImpulse()`: Returns the impulse applied for a forward dodge.
-   `void SetForwardDodgeImpulse(float newImpulse)`: Sets the forward dodge impulse.
-   `float GetSideDodgeImpulse()`: Returns the impulse applied for a side dodge.
-   `void SetSideDodgeImpulse(float newImpulse)`: Sets the side dodge impulse.
-   `float GetBackwardDodgeImpulse()`: Returns the impulse applied for a backward dodge.
-   `void SetBackwardDodgeImpulse(float newImpulse)`: Sets the backward dodge impulse.

#### Dodge Torque (Rotation)

-   `float GetForwardDodgeTorque()`: Returns the torque applied for a forward dodge.
-   `void SetForwardDodgeTorque(float newTorque)`: Sets the forward dodge torque.
-   `float GetSideDodgeTorque()`: Returns the torque applied for a side dodge.
-   `void SetSideDodgeTorque(float newTorque)`: Sets the side dodge torque.

#### Dodge State

-   `Vector GetDodgeDirection()`: Returns a normalized vector representing the direction of the current dodge. Will be `(0,0,0)` if not currently dodging.
-   `float GetDodgeInputThreshold()`: Returns the threshold for analog stick input to trigger a dodge. A value below this results in a double jump instead of a directional dodge.

## Example: Displaying Dodge Information

```cpp
void MyPlugin::ShowDodgeInfo()
{
    CarWrapper car = gameWrapper->GetLocalCar();
    if (!car) { return; }

    // Find the dodge component
    DodgeComponentWrapper dodgeComponent;
    for (CarComponentWrapper& component : car.GetDefaultCarComponents())
    {
        if (auto casted = std::dynamic_pointer_cast<DodgeComponentWrapper>(component.get_shared_ptr()))
        {
            dodgeComponent = *casted;
            break;
        }
    }
    
    if (dodgeComponent)
    {
        Vector dodgeDir = dodgeComponent.GetDodgeDirection();
        if (dodgeDir.X != 0 || dodgeDir.Y != 0)
        {
            LOG("Currently dodging in direction: ({}, {})", dodgeDir.X, dodgeDir.Y);
        }
    }
}
```
