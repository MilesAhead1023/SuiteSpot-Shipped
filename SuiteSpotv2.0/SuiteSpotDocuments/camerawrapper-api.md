# CameraWrapper API

## Overview

The `CameraWrapper` represents the player's in-game camera. It provides access to the camera's position, rotation, field of view (FOV), and the underlying camera state that controls its behavior (e.g., Ball Cam, Freecam).

### How to Get a `CameraWrapper`

You can get the current active camera directly from the `GameWrapper`.

```cpp
CameraWrapper camera = gameWrapper->GetCamera();
if (camera)
{
    // Use the camera wrapper
}
```

## API Description

### Header
`bakkesmod/wrappers/GameObject/CameraWrapper.h`

### Inheritance
`CameraWrapper` -> `CameraXWrapper` -> ... -> `ActorWrapper` -> `ObjectWrapper`

### Key Functions

#### Position, Rotation, and FOV (The "POV")

-   `Vector GetLocation()`: Returns the camera's current location in world space.
-   `void SetLocation(Vector location)`: Sets the camera's location.
-   `Rotator GetRotation()`: Returns the camera's current rotation.
-   `void SetRotation(Rotator rotation)`: Sets the camera's rotation.
-   `float GetFOV()`: Returns the camera's current field of view.
-   `void SetFOV(float fov)`: Sets the camera's FOV.
-   `POV GetPOV()`: Returns a `POV` struct, which contains `Location`, `Rotation`, and `FOV` all in one object.
-   `void SetPOV(POV newPov)`: Sets the camera's location, rotation, and FOV from a `POV` struct.

#### Camera States

The camera's behavior is determined by a "state" object (e.g., ball cam, player view, freecam). You can read and, in some cases, change this state.

-   `std::string GetCameraState()`: Returns the name of the current camera state class, for example:
    -   `"CameraState_BallCam_TA"`
    -   `"CameraState_PlayerView_TA"`
    -   `"CameraState_Freecam_TA"`
    -   `"CameraState_Replay_TA"`
-   `void SetCameraState(std::string stateName)`: Forces a new camera state. This can be unstable if not used carefully.

#### Player Camera Settings

-   `ProfileCameraSettings GetCameraSettings()`: Returns a `ProfileCameraSettings` struct containing the player's configured camera preferences (stiffness, height, distance, angle, etc.).
-   `void SetCameraSettings(ProfileCameraSettings settings)`: Applies a new set of camera settings.

## Example: Getting Camera Information

```cpp
void MyPlugin::LogCameraInfo()
{
    CameraWrapper camera = gameWrapper->GetCamera();
    if (!camera) { return; }

    Vector location = camera.GetLocation();
    float fov = camera.GetFOV();
    std::string state = camera.GetCameraState();

    LOG("Camera is at ({}, {}, {}) with FOV {} in state: {}", 
        location.X, location.Y, location.Z, fov, state);
}
```

## Example: Creating a Simple "Fly-Cam"

By taking control of the camera's POV every frame, you can create custom camera behaviors. The code below would create a simple camera that slowly flies upwards.

```cpp
// This function would be hooked to a tick event, e.g., via RegisterDrawable
void MyPlugin::UpdateFlyCam(CanvasWrapper canvas)
{
    CameraWrapper camera = gameWrapper->GetCamera();
    if (!camera) { return; }

    // Get current POV
    POV currentPov = camera.GetPOV();

    // Modify the location
    currentPov.Location.Z += 10; // Move up by 10 units each frame

    // Apply the new POV
    camera.SetPOV(currentPov);
}
```
