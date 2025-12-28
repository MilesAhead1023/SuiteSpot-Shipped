# FlipCar Component API

## Overview

The `FlipCarComponentWrapper` is a specialized `CarComponent` that manages the state and physics of a car's flip (including side dodges and forward/backward flips). It can be used to inspect the properties of an ongoing flip.

### How to Get a `FlipCarComponentWrapper`

You can get an instance of this component from a `CarWrapper`:

```cpp
CarWrapper car = gameWrapper->GetLocalCar();
if (car)
{
    FlipCarComponentWrapper flipComponent = car.GetFlipComponent();
    if (flipComponent)
    {
        // Use the flip component
    }
}
```

## API Description

### Header
`bakkesmod/wrappers/GameObject/CarComponent/FlipCarComponentWrapper.h`

### Inheritance
`FlipCarComponentWrapper` -> `CarComponentWrapper` -> ... -> `ObjectWrapper`

### Key Functions

-   `float GetFlipCarTime()`: Returns the time in seconds that the flip has been active. You can use this to check how far along a flip is.
-   `float GetFlipCarImpulse()`: Returns the magnitude of the impulse force applied at the start of a flip.
-   `float GetFlipCarTorque()`: Returns the torque applied during the flip.
-   `unsigned long GetbFlipRight()`: Returns `true` if the flip is a right-side dodge, `false` otherwise (e.g., for a left dodge or forward/backward flip).
-   `void InitFlip()`: Re-initializes the flip, allowing a car to flip again (e.g., for a flip reset).

## Example: Checking flip time

```cpp
// Get the flip component from the local car
FlipCarComponentWrapper flip = gameWrapper->GetLocalCar().GetFlipComponent();
if (flip)
{
    float flipTime = flip.GetFlipCarTime();
    if (flipTime > 0.0f)
    {
        LOG("Car has been flipping for {} seconds.", flipTime);
    }
}
```
