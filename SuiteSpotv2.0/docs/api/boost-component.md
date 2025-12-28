# Boost Component API

## Overview

The `BoostWrapper` is a specialized `CarComponent` that manages a car's boost system. It provides access to the current boost amount and allows for modification of its behavior, such as enabling unlimited boost or changing the consumption rate.

### How to Get a `BoostWrapper`

Unlike the `FlipCarComponent`, there is no direct function on `CarWrapper` to get the `BoostWrapper`. Instead, you must get the array of default car components and iterate through it to find the component that is a `BoostWrapper`.

This is done by using `std::dynamic_pointer_cast` to attempt to cast the `CarComponentWrapper` to a `BoostWrapper`.

```cpp
CarWrapper car = gameWrapper->GetLocalCar();
if (!car) { return; }

BoostWrapper boostComponent;
ArrayWrapper<CarComponentWrapper> components = car.GetDefaultCarComponents();
for (int i = 0; i < components.Count(); i++)
{
    CarComponentWrapper component = components.Get(i);
    // Attempt to cast the component to a BoostWrapper
    auto castedBoost = std::dynamic_pointer_cast<BoostWrapper>(component.get_shared_ptr());
    if (castedBoost)
    {
        // Cast was successful, we found the boost component
        boostComponent = BoostWrapper(castedBoost->memory_address);
        break;
    }
}

if (boostComponent)
{
    // Now you can use the boost component
    boostComponent.SetUnlimitedBoost2(1);
}
```

> **Note for AI Agent:** This pattern of iterating components and using a dynamic cast is the standard way to access most car components that do not have a direct getter function on the `CarWrapper`.

## API Description

### Header
`bakkesmod/wrappers/GameObject/CarComponent/BoostWrapper.h`

### Inheritance
`BoostWrapper` -> `CarComponentWrapper` -> ... -> `ObjectWrapper`

### Key Functions

#### Boost Amount and State

-   `float GetCurrentBoostAmount()`: Returns the current boost amount, scaled from `0.0` to `1.0` (where `1.0` is a full 100 boost).
-   `void SetBoostAmount(float Amount)`: Sets the current boost amount. The input is also scaled from `0.0` to `1.0`.
-   `float GetMaxBoostAmount()`: Returns the maximum boost amount (typically `1.0`).
-   `bool IsFull()`: Returns `true` if the boost amount is at its maximum.
-   `void GiveFullBoost()`: Sets the current boost amount to its maximum value.

#### Modifying Boost Behavior

-   `void SetUnlimitedBoost2(unsigned long Enabled)`: Enables (`1`) or disables (`0`) unlimited boost for the car.
-   `void SetNoBoost(unsigned long Enabled)`: Completely disables (`1`) or enables (`0`) the car's ability to boost.
-   `float GetBoostConsumptionRate()`: Returns the rate at which boost is consumed.
-   `void SetBoostConsumptionRate(float newRate)`: Sets a new boost consumption rate.
-   `float GetRechargeRate()`: Returns the recharge rate (used in modes like Heatseeker).
-   `void SetRechargeRate(float newRate)`: Sets a new recharge rate.

## Example: Checking boost amount

```cpp
void MyPlugin::CheckBoost()
{
    CarWrapper car = gameWrapper->GetLocalCar();
    if (!car) { return; }

    // Find the boost component by iterating
    BoostWrapper boostComponent;
    for (CarComponentWrapper component : car.GetDefaultCarComponents())
    {
        if (auto boost = std::dynamic_pointer_cast<BoostWrapper>(component.get_shared_ptr()))
        {
            boostComponent = *boost;
            break;
        }
    }
    
    if (!boostComponent) { return; }

    float currentBoost = boostComponent.GetCurrentBoostAmount() * 100; // Multiply by 100 for display
    LOG("Current boost: {:.0f}", currentBoost);

    // Example of giving full boost
    if (currentBoost < 10)
    {
        boostComponent.GiveFullBoost();
        LOG("Boost was low. Refilling!");
    }
}
```
