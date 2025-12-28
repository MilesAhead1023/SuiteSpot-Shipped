# CarWrapper API

The `CarWrapper` represents a car in the game. You can get the local player's car using `gameWrapper->GetLocalCar()`, or you can get a list of all cars in the match from `serverWrapper->GetCars()`.

## API Description

### Header
`bakkesmod/wrappers/GameObject/CarWrapper.h`

### Inheritance
`CarWrapper` -> `VehicleWrapper` -> `RBActorWrapper` -> `ActorWrapper` -> `ObjectWrapper`

Many common functions for physics manipulation (like `GetRBState`, `AddForce`) are available on this wrapper because it inherits from [RBActorWrapper](./rbactor-wrapper.md).

## Boost

-   `bool IsBoostCheap()`: Returns `true` if the "unlimited boost" mutator is active.
-   `void SetBoostCheap(bool b)`: Sets the boost to be unlimited.
-   `void ForceBoost(bool force)`: Forces the car to boost.

## Movement and Control

-   `void SetCarRotation(Rotator rotation)`: Sets the car's rotation.
-   `ControllerInput GetInput()`: Returns the car's current controller input.
-   `void SetInput(ControllerInput input)`: Sets the car's controller input. This can be used to make the car drive, jump, etc.
-   `void Unfreeze()`: Unfreezes the car if it is frozen (e.g., at the start of a kickoff).
-   `void Demolish()`: Demolishes the car.
-   `bool IsDodging()`: Returns true if the car is currently dodging.

## State

-   `unsigned long HasFlip()`: Returns `true` if the car has a flip.
-   `std::string GetOwnerName()`: Returns the name of the player who owns the car.
-   `bool AnyWheelTouchingGround()`: Returns true if at least one wheel is touching the ground.

## Loadout

-   `int GetLoadoutBody()`: Returns the car's body ID.
-   `void SetCarColor(LinearColor mainColor, LinearColor secondaryColor)`: Sets the car's primary and secondary colors. This only works in freeplay.

## Components

A car's functionality is broken down into several components. While some have direct accessors, most must be found by iterating through the car's component array.

-   `FlipCarComponentWrapper GetFlipComponent()`: Returns the `FlipCarComponentWrapper` for the car. This is a special case with a direct getter. See the [FlipCar Component API](./flipcar-component.md) for more details.
-   `ArrayWrapper<CarComponentWrapper> GetDefaultCarComponents()`: Returns an array of all default components attached to the car. To access a specific component like the `BoostWrapper`, you must iterate this array and perform a dynamic cast to find the component you need.
-   **`BoostWrapper`**: Manages the car's boost. See the [Boost Component API](./boost-component.md) for details on its functionality and the code pattern required to access it.
-   **`JumpComponentWrapper`**: Manages the car's first jump. See the [Jump Component API](./jump-component.md) for details.
-   **`DodgeComponentWrapper`**: Manages the car's dodge (second jump). See the [Dodge Component API](./dodge-component.md) for details.
-   Other components can be accessed using the same iteration and casting pattern.

## Other

-   `void Destroy()`: Destroys the car.
-   `void Teleport(Vector& SpawnLocation, Rotator& SpawnRotation, unsigned long bStopVelocity, unsigned long bUpdateRotation, float ExtraForce)`: Teleports the car to a new location.