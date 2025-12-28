# BoostPickupWrapper API

## Overview

The `BoostPickupWrapper` represents a boost pad on the field. It provides information about the pad's state (active or inactive), its location, and how much boost it provides.

This wrapper inherits from `VehiclePickupWrapper`, which provides the core functionality for a pickup item.

### How to Get a `BoostPickupWrapper`

You can get an array of all boost pads on the map from the `ServerWrapper`:

```cpp
ServerWrapper server = gameWrapper->GetCurrentGameState();
if (server)
{
    ArrayWrapper<BoostPickupWrapper> boostPads = server.GetBoostPads();
}
```

## API Description

### Header
`bakkesmod/wrappers/GameObject/BoostPickupWrapper.h`
`bakkesmod/wrappers/GameObject/VehiclePickupWrapper.h`

### Inheritance
`BoostPickupWrapper` -> `VehiclePickupWrapper` -> `ActorWrapper` -> `ObjectWrapper`

### Key Functions

#### Boost-Specific Information

-   `float GetBoostAmount()`: Returns the amount of boost this pad gives when picked up.
    -   Small pads give `0.12`.
    -   Large 100-boost orbs give `1.0`.

#### Pickup State (from `VehiclePickupWrapper`)

-   `bool GetbPickedUp()`: Returns `true` if the boost pad is currently inactive (i.e., someone has recently taken it). Returns `false` if it is active and can be picked up.
-   `void SetbPickedUp(unsigned long bNewPickedUp)`: Manually sets the state of the boost pad. `1` for inactive, `0` for active.
-   `float GetRespawnDelay()`: Returns the time in seconds it takes for this boost pad to respawn after being picked up.
-   `void Respawn()`: Forces the boost pad to respawn and become active.

#### Spatial Information (from `ActorWrapper`)

-   `Vector GetLocation()`: Returns the `Vector` location of the boost pad in the world.

## Example: Find the closest active boost pad

```cpp
void MyPlugin::FindClosestBoost()
{
    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) { return; }
    CarWrapper car = server.GetLocalCar();
    if (!car) { return; }

    Vector carLocation = car.GetLocation();
    BoostPickupWrapper closestPad;
    float closestDist = -1;

    ArrayWrapper<BoostPickupWrapper> boostPads = server.GetBoostPads();
    for (int i = 0; i < boostPads.Count(); i++)
    {
        BoostPickupWrapper pad = boostPads.Get(i);
        if (pad && !pad.GetbPickedUp()) // Check if the pad is active
        {
            Vector padLocation = pad.GetLocation();
            float distance = Vector::distance(carLocation, padLocation);

            if (closestDist == -1 || distance < closestDist)
            .md{
                closestDist = distance;
                closestPad = pad;
            }
        }
    }

    if (closestPad)
    {
        LOG("Closest active boost pad is {:.0f} units away.", closestDist);
    }
}
```
