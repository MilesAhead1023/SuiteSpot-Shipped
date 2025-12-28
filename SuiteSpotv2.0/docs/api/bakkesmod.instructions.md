# BakkesMod SDK Context

Complete reference for the BakkesMod SDK and how to use it in SuiteSpot development.

## Core Architecture

### Plugin System Foundation
The SDK is built around the `BakkesModPlugin` base class which provides access to core systems:

- `cvarManager`: Console variable and command management
- `gameWrapper`: Primary interface to game state and engine
- `onLoad()`/`onUnload()`: Plugin lifecycle hooks

### Plugin Types and Permissions
Plugins declare their type and execution context through enums:

**Plugin Types:**
- `PLUGINTYPE_FREEPLAY` - Load in freeplay mode
- `PLUGINTYPE_CUSTOM_TRAINING` - Load in training mode
- `PLUGINTYPE_SPECTATOR` - Load in spectator mode
- `PLUGINTYPE_REPLAY` - Load in replay mode
- `PLUGINTYPE_THREADED` - Run in separate thread
- `PLUGINTYPE_THREADEDUNLOAD` - Unload in separate thread

**Permissions:**
- `PERMISSION_ALL` - Execute everywhere
- `PERMISSION_FREEPLAY` - Only in freeplay
- `PERMISSION_CUSTOM_TRAINING` - Only in training
- `PERMISSION_ONLINE` - Only in online matches
- `PERMISSION_OFFLINE` - Only offline (no online match)

## Wrapper System Architecture

### Design Patterns
- **PIMPL Pattern**: Hides implementation details for binary compatibility
- **Null Safety**: All wrappers implement `IsNull()` and boolean operators
- **Constructor Pattern**: Standardized through `CONSTRUCTORS()` macro

### Wrapper Hierarchy
```
ObjectWrapper (base)
├── ActorWrapper (spatial objects)
│   ├── CarWrapper, BallWrapper, GoalWrapper
│   └── CameraWrapper
├── PlayerReplicationInfoWrapper
│   └── PriXWrapper, PriWrapper
└── ControllerWrapper
    └── PlayerControllerWrapper
```

### Key Wrappers
- **Engine Layer**: `ObjectWrapper`, `ActorWrapper`, `EngineTAWrapper`
- **Game Events**: `GameEventWrapper`, `ServerWrapper`, `ReplayWrapper`
- **Game Objects**: `CarWrapper`, `BallWrapper`, `BoostWrapper`
- **Components**: `VehicleSimWrapper`, `FlipCarComponentWrapper`, `WheelWrapper`
- **Player System**: `PriWrapper`, `PriXWrapper`, `TeamWrapper`

## GameWrapper: Primary Interface

### Game State Detection
```cpp
bool IsInGame();
bool IsInOnlineGame();
bool IsInFreeplay();
bool IsInReplay();
bool IsInCustomTraining();
bool IsPaused();
```

### Core Accessors
```cpp
ServerWrapper GetCurrentGameState();
CarWrapper GetLocalCar();
CameraWrapper GetCamera();
EngineTAWrapper GetEngine();
PlayerControllerWrapper GetPlayerController();
MMRWrapper GetMMRWrapper();
```

### Event System
```cpp
void HookEvent(std::string eventName, std::function<void(std::string)> callback);
void HookEventPost(std::string eventName, std::function<void(std::string)> callback);
void RegisterDrawable(std::function<void(CanvasWrapper)> callback);
void SetTimeout(std::function<void(GameWrapper*)> callback, float delay);
```

## Plugin Registration

All plugins must be registered using the macro:
```cpp
BAKKESMOD_PLUGIN(ClassName, "Plugin Name", "1.0.0", PLUGINTYPE_FREEPLAY)
```

## Important Notes

- All wrappers provide null safety through `IsNull()` methods
- The wrapper system uses automated code generation for consistency (sections marked `//AUTO-GENERATED`)
- CVars and notifiers are automatically cleaned up when plugin unloads
- Plugin DLL must be placed in `plugins/` folder
- Load via console: `plugin load filename` (without .dll extension)
- Current API version: `BAKKESMOD_PLUGIN_API_VERSION 95`

## Common Patterns

### CVarManager Usage
```cpp
cvarManager->registerCvar(name, defaultValue, description, searchable, hasMin, min, hasMax, max)
    .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        // Handle value change
    });

cvarManager->getCvar(name).setValue(newValue);
cvarManager->executeCommand(command);
```

### Event Hooking
```cpp
gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", 
    bind(&ClassName::OnMatchEnded, this, placeholders::_1));
```

### Delayed Execution
```cpp
gameWrapper->SetTimeout([this](GameWrapper* gw) {
    // Code executes after delay
}, delayInSeconds);
```

## Math Utilities

The SDK provides quaternion and rotator conversions:
```cpp
static inline Quat RotatorToQuat(const Rotator rot);
static inline Vector RotateVectorWithQuat(const Vector v, const Quat q);
```

## Community Resources

- BakkesMod programming Discord: https://discord.gg/HMptXSzCvU
- BakkesModSDK GitHub: https://github.com/bakkesmodorg/BakkesModSDK
- Plugin Template: https://github.com/Martinii89/BakkesmodPluginTemplate
