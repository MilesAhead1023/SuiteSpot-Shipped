# PlayerControllerWrapper API

## Overview

The `PlayerControllerWrapper` is a crucial wrapper that represents the player's "mind" or point of control within the game. It is the bridge between the player's input (from a controller or keyboard) and the actions their car performs. It also manages camera control, chat, and spectating.

It inherits from `ActorWrapper`.

### How to Get a `PlayerControllerWrapper`

You can get the local player's controller directly from the `GameWrapper`.

```cpp
PlayerControllerWrapper playerController = gameWrapper->GetPlayerController();
if (playerController)
{
    // Use the player controller
}
```

## API Description

### Header
`bakkesmod/wrappers/PlayerControllerWrapper.h`

### Inheritance
`PlayerControllerWrapper` -> `ActorWrapper` -> `ObjectWrapper`

### Key Functions

#### Player Representation

This wrapper is the central hub for accessing all objects related to a specific player.

-   `PriWrapper GetPRI()`: Returns the `PlayerReplicationInfo` (`PriWrapper`) for this player. The PRI contains the player's name, stats, score, team, and unique ID. This is a very important function.
-   `CarWrapper GetCar()`: Returns the `CarWrapper` that this player is currently controlling. This will be null if the player is spectating or not in a car.

#### Player Input

You can read the player's raw controller input.

-   `ControllerInput GetVehicleInput()`: Returns a `ControllerInput` struct. This struct contains float values for all major axes:
    -   `Throttle`
    -   `Steer`
    -   `Pitch`
    -   `Yaw`
    -   `Roll`
    -   `DodgeStrafe`
    -   `DodgeForward`
-   `bool GetbJumpPressed()`: Returns `true` if the jump button is currently being held down.
-   `bool GetbBoostPressed()`: Returns `true` if the boost button is currently being held down.
-   `bool GetbHandbrakePressed()`: Returns `true` if the handbrake button is currently being held down.

#### Chat

-   `void ServerSay_TA(std::string Message, unsigned char ChatChannel, unsigned long bPreset)`: Sends a chat message.
    -   `ChatChannel`: `0` for team chat, `1` for all chat.
    -   `bPreset`: `true` if this is a quick chat.

#### Spectating

-   `void SetFollowTarget(PriWrapper InTarget)`: Forces the player's camera to start spectating the given target player.
-   `PriWrapper GetFollowTarget()`: Returns the `PriWrapper` of the player currently being spectated.

## Example: Displaying Player Input

```cpp
void MyPlugin::ShowPlayerInput()
{
    PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
    if (!pc) { return; }

    ControllerInput input = pc.GetVehicleInput();
    LOG("Throttle: {:.2f}, Steer: {:.2f}", input.Throttle, input.Steer);

    if (pc.GetbJumpPressed())
    {
        LOG("Player is holding jump!");
    }
}
```

## Example: Sending a Team Chat Message

```cpp
void MyPlugin::SayHelloTeam()
{
    PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
    if (!pc) { return; }

    // Channel 0 = Team Chat
    // bPreset = false
    pc.ServerSay_TA("Hello team!", 0, false);
}
```
