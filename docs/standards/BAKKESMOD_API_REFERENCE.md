# BakkesMod SDK API Reference

This document provides a comprehensive reference for the BakkesMod SDK used in SuiteSpot plugin development.

## Table of Contents

1. [Plugin System](#plugin-system)
2. [GameWrapper](#gamewrapper)
3. [CVarManagerWrapper](#cvarmanagerwrapper)
4. [CVarWrapper](#cvarwrapper)
5. [ServerWrapper](#serverwrapper)
6. [CarWrapper](#carwrapper)
7. [BallWrapper](#ballwrapper)
8. [PlayerControllerWrapper](#playercontrollerwrapper)
9. [PriWrapper](#priwrapper)
10. [CanvasWrapper](#canvaswrapper)
11. [Data Structures](#data-structures)
12. [Wrapper Hierarchy](#wrapper-hierarchy)

---

## Plugin System

### Base Classes

**BakkesModPlugin** - All plugins inherit from this:
```cpp
class BakkesModPlugin {
public:
    std::shared_ptr<CVarManagerWrapper> cvarManager;
    std::shared_ptr<GameWrapper> gameWrapper;

    virtual void onLoad() {};   // Called when plugin loads
    virtual void onUnload() {}; // Called when plugin unloads
};
```

**PluginWindow** - For plugins with floating windows:
```cpp
class PluginWindow {
    virtual void Render() = 0;                         // ImGui rendering
    virtual std::string GetMenuName() = 0;             // Menu toggle name
    virtual std::string GetMenuTitle() = 0;            // Window title
    virtual void SetImGuiContext(uintptr_t ctx) = 0;   // ImGui context
    virtual bool ShouldBlockInput() = 0;               // Block game input?
    virtual bool IsActiveOverlay() = 0;                // Non-interactive?
    virtual void OnOpen() = 0;                         // Window opened
    virtual void OnClose() = 0;                        // Window closed
};
```

**PluginSettingsWindow** - For F2 menu settings tab:
```cpp
class PluginSettingsWindow {
    virtual void RenderSettings() = 0;                 // Draw settings
    virtual std::string GetPluginName() = 0;
    virtual void SetImGuiContext(uintptr_t ctx) = 0;
};
```

### Plugin Registration Macro

```cpp
BAKKESMOD_PLUGIN(ClassName, "Plugin Name", "1.0.0", PLUGINTYPE_FREEPLAY)
```

### Plugin Types

```cpp
enum PLUGINTYPE {
    PLUGINTYPE_FREEPLAY         = 0x01,
    PLUGINTYPE_CUSTOM_TRAINING  = 0x02,
    PLUGINTYPE_SPECTATOR        = 0x04,
    PLUGINTYPE_BOTAI            = 0x08,
    PLUGINTYPE_REPLAY           = 0x10,
    PLUGINTYPE_THREADED         = 0x20,
    PLUGINTYPE_THREADEDUNLOAD   = 0x40
};
```

### Notifier Permissions

```cpp
enum NOTIFIER_PERMISSION {
    PERMISSION_ALL              = 0,
    PERMISSION_MENU             = (1 << 0),
    PERMISSION_SOCCAR           = (1 << 1),
    PERMISSION_FREEPLAY         = (1 << 2),
    PERMISSION_CUSTOM_TRAINING  = (1 << 3),
    PERMISSION_ONLINE           = (1 << 4),
    PERMISSION_PAUSEMENU_CLOSED = (1 << 5),
    PERMISSION_REPLAY           = (1 << 6),
    PERMISSION_OFFLINE          = (1 << 7)
};
```

---

## GameWrapper

The primary interface for game interaction.

### Game State Detection

```cpp
bool IsInGame();              // Currently in active game
bool IsInOnlineGame();        // Online multiplayer
bool IsInFreeplay();          // Freeplay mode
bool IsInReplay();            // Replay viewer
bool IsInCustomTraining();    // Training mode
bool IsPaused();              // Game paused
bool IsUsingEpicVersion();    // Epic Games version
bool IsUsingSteamVersion();   // Steam version
```

### Getting Game Objects

```cpp
ServerWrapper GetCurrentGameState();           // Current game/server
ServerWrapper GetOnlineGame();                 // Online game state
ServerWrapper GetGameEventAsServer();          // Cast to server
ReplayServerWrapper GetGameEventAsReplay();    // Cast to replay
CarWrapper GetLocalCar();                      // Player's car
CameraWrapper GetCamera();                     // Active camera
EngineTAWrapper GetEngine();                   // Engine reference
PlayerControllerWrapper GetPlayerController(); // Local controller
ItemsWrapper GetItemsWrapper();                // Items system
MatchmakingWrapper GetMatchmakingWrapper();    // Matchmaking
SettingsWrapper GetSettings();                 // Game settings
```

### Event Hooking

```cpp
// Hook before event execution
void HookEvent(std::string eventName,
    std::function<void(std::string)> callback);
void UnhookEvent(std::string eventName);

// Hook after event execution
void HookEventPost(std::string eventName,
    std::function<void(std::string)> callback);
void UnhookEventPost(std::string eventName);

// Hook with caller object (type-safe)
template<typename T>
void HookEventWithCaller(std::string eventName,
    std::function<void(T caller, void* params, std::string)> callback);

template<typename T>
void HookEventWithCallerPost(std::string eventName,
    std::function<void(T caller, void* params, std::string)> callback);
```

**Supported caller types:** ServerWrapper, ActorWrapper, CarWrapper, CarComponentWrapper, PlayerControllerWrapper, BallWrapper, PriWrapper

### Execution & Timing

```cpp
void SetTimeout(std::function<void(GameWrapper*)> lambda, float seconds);
    // Schedule callback after delay (thread-safe)

void Execute(std::function<void(GameWrapper*)> lambda);
    // Execute on game thread from different thread (CRITICAL for thread safety)
```

### Drawing & UI

```cpp
void RegisterDrawable(std::function<void(CanvasWrapper)> callback);
    // Register rendering callback (called each frame)

void UnregisterDrawables();
    // Remove ALL drawables

void Toast(std::string title, std::string text,
    std::string texture = "default", float timeout = 3.5f,
    uint8_t toastType = 0, float width = 290.f, float height = 60.f);

void LoadToastTexture(std::string name, std::string path);
```

**Toast types:** 0=Info, 1=OK, 2=Warning, 3=Error

### Display Information

```cpp
Vector2 GetScreenSize();
float GetDisplayScale();
float GetInterfaceScale();
bool GetbColorBlind();
```

### Player & Map Info

```cpp
std::string GetCurrentMap();
std::string GetRandomMap();
unsigned long long GetSteamID();
std::string GetEpicID();
UniqueIDWrapper GetUniqueID();
UnrealStringWrapper GetPlayerName();
```

### File Paths

```cpp
std::filesystem::path GetBakkesModPath();  // Installation directory
std::filesystem::path GetDataFolder();     // Data storage directory
```

---

## CVarManagerWrapper

Console variable and command management.

### Command Execution

```cpp
void executeCommand(std::string command, bool log = true);
void log(std::string text);
void log(std::wstring text);
```

### Notifier Registration

```cpp
void registerNotifier(
    std::string name,
    std::function<void(std::vector<std::string>)> callback,
    std::string description,
    unsigned char permissions
);
bool removeNotifier(std::string name);
```

### CVar Registration

```cpp
CVarWrapper registerCvar(
    std::string name,
    std::string defaultValue,
    std::string description = "",
    bool searchable = true,
    bool hasMin = false, float min = 0,
    bool hasMax = false, float max = 0,
    bool saveToCfg = true
);
bool removeCvar(std::string name);
CVarWrapper getCvar(std::string name);
```

### Key Bindings

```cpp
std::string getBindStringForKey(std::string key);
void setBind(std::string key, std::string command);
void removeBind(std::string key);
```

### Aliases

```cpp
std::string getAlias(std::string alias);
void setAlias(std::string key, std::string script);
```

### Config Management

```cpp
void backupCfg(std::string path);
void backupBinds(std::string path);
void loadCfg(std::string path);
```

---

## CVarWrapper

Individual console variable interface.

### Getting Values

```cpp
int getIntValue();
float getFloatValue();
bool getBoolValue();
std::string getStringValue();
LinearColor getColorValue();  // Parses "(R,G,B,A)" 0-255 or hex "#RRGGBB"
```

### Setting Values

```cpp
void setValue(std::string value);
void setValue(int value);
void setValue(float value);
void setValue(LinearColor value);  // 0-255 range
void ResetToDefault();
void notify();  // Trigger all listeners
```

### Metadata

```cpp
std::string getCVarName();
std::string getDescription();
std::string GetDefaultValue();
bool HasMinimum();
bool HasMaximum();
float GetMinimum();
float GetMaximum();
bool IsHidden();
bool ShouldSaveToCfg();
```

### Change Notifications

```cpp
void addOnValueChanged(std::function<void(std::string oldValue, CVarWrapper cvar)> callback);
void removeOnValueChanged();
```

### Data Binding

```cpp
void bindTo(std::shared_ptr<int> var);
void bindTo(std::shared_ptr<float> var);
void bindTo(std::shared_ptr<std::string> var);
void bindTo(std::shared_ptr<bool> var);
void bindTo(std::shared_ptr<LinearColor> var);
```

### Null Checking

```cpp
bool IsNull();
explicit operator bool();  // if(cvar) { ... }
```

---

## ServerWrapper

Match/server state management. Inherits from TeamGameEventWrapper.

### Ball Management

```cpp
BallWrapper GetBall();
BallWrapper SpawnBall(Vector position, bool wake, bool spawnCannon);
ArrayWrapper<BallWrapper> GetGameBalls();
void ResetBalls();
void RemoveGameBall(BallWrapper ball);
void AddGameBall(BallWrapper ball);
```

### Car Management

```cpp
CarWrapper GetGameCar();
void SpawnCar(int carBody, std::string name);
void SpawnBot(int carBody, std::string name);
ArrayWrapper<CarWrapper> GetCars();
void RemoveCar(CarWrapper car);
void DestroyCars();
```

### Time & Scoring

```cpp
int GetGameTime();
void SetGameTime(int time);
int GetSecondsRemaining();
float GetGameTimeRemaining();
int GetMaxScore();
void SetMaxScore(int score);
float GetGameSpeed();
void SetGameSpeed(float speed);  // 0=pause, 1=normal, 2=2x
```

### Match State

```cpp
bool IsFinished();
bool HasAuthority();  // Server has game authority
TeamWrapper GetGameWinner();
TeamWrapper GetMatchWinner();
PriWrapper GetMVP();
```

### Players

```cpp
ArrayWrapper<ControllerWrapper> GetPlayers();
ArrayWrapper<PriWrapper> GetPRIs();
ArrayWrapper<CarWrapper> GetCars();
int GetNumPlayers();
```

### Goals

```cpp
ArrayWrapper<GoalWrapper> GetGoals();
Vector GetGoalLocation(int goalNumber = 0);
Vector GetGoalExtent(int goalNumber = 0);
bool IsInGoal(Vector vec);
bool IsBallMovingTowardsGoal(int goalNo, BallWrapper ball);
```

### Game Control

```cpp
void StartRound();
void EndRound();
void EndGame();
void ForceOvertime();
void ResetGame();
```

---

## CarWrapper

Vehicle object wrapper. Inherits from VehicleWrapper.

### State Queries

```cpp
bool IsBoostCheap();
void SetBoostCheap(bool cheap);
bool IsDodging();
bool AnyWheelTouchingGround();
bool IsOnWall();
bool IsOnGround();
unsigned long HasFlip();  // Can double jump
int GetLoadoutBody();
std::string GetOwnerName();
```

### Input

```cpp
ControllerInput GetInput();
void SetInput(ControllerInput input);
```

### Physics

```cpp
Vector GetVelocity();
void SetVelocity(Vector velocity);
Vector GetLocation();
void SetLocation(Vector location);
Rotator GetRotation();
void SetRotation(Rotator rotation);
void SetCarRotation(Rotator rotation);
Vector GetAngularVelocity();
void SetAngularVelocity(Vector v, bool addToCurrent);
float GetForwardSpeed();
```

### Actions

```cpp
void Destroy();
void Demolish();
void Unfreeze();
void ForceBoost(bool force);
void SetCarScale(float scale);
void SetCarColor(LinearColor main, LinearColor secondary);  // Freeplay only
```

### Components

```cpp
BoostWrapper GetBoostComponent();
FlipCarComponentWrapper GetFlipComponent();
ArrayWrapper<CarComponentWrapper> GetDefaultCarComponents();
```

### Teleport

```cpp
bool Teleport(Vector& location, Rotator& rotation,
    bool stopVelocity, bool updateRotation, float extraForce);
```

### References

```cpp
GameEventWrapper GetGameEvent();
PriWrapper GetAttackerPRI();
```

---

## BallWrapper

Ball/puck object wrapper. Inherits from RBActorWrapper.

### Ball Variants

```cpp
bool IsDropshotBall() const;
bool IsHauntedBall() const;
bool IsGodBall() const;
```

### Physics

```cpp
Vector GetVelocity();
void SetVelocity(Vector velocity);
Vector GetLocation();
void SetLocation(Vector location);
float GetRadius();
void SetRadius(float radius);
Vector GetMagnusCoefficient();
void SetMagnusCoefficient(Vector coefficient);
```

### Prediction

```cpp
PredictionInfo PredictPosition(float timeAhead);
float GetLastTouchTime();
```

### Visibility

```cpp
void FadeOutBall();
void FadeInBall();
```

### Physics Modifiers

```cpp
void SetBallScale(float scale);
void SetBallGravityScale(float scale);
void SetBallMaxLinearSpeedScale(float scale);
void SetWorldBounceScale(float scale);
void SetCarBounceScale(float scale);
```

### Hit Tracking

```cpp
unsigned char GetHitTeamNum();
float GetLastHitWorldTime();
CarWrapper GetCurrentAffector();
```

### Game Reference

```cpp
ServerWrapper GetGameEvent();
```

---

## PlayerControllerWrapper

Local player input controller. Inherits from ActorWrapper.

### Vehicle & Player

```cpp
CarWrapper GetCar();
void SetCar(CarWrapper car);
PriWrapper GetPRI();
void SetPRI(PriWrapper pri);
bool GetbUsingGamepad();
```

### Input

```cpp
ControllerInput GetVehicleInput();
void SetVehicleInput(ControllerInput input);
```

### Camera

```cpp
void SetUsingFreecam(bool freecam);
void SetUsingBehindView(bool behindView);
void ToggleCameraPosition();
void ResetCameraMode();
```

### Game Actions

```cpp
void ChangeTeam(int teamNum);
void SwitchTeam();
void ReadyUp();
void Spectate();
```

### Chat

```cpp
void Say_TA2(std::string message, unsigned char channel,
    SteamID& recipient, bool preset);
bool CanChatWith(PlayerControllerWrapper other, bool preset);
```

### Spectating

```cpp
PriWrapper GetFollowTarget();
void SetFollowTarget(PriWrapper target);
void FollowPlayer(PriWrapper player);
```

---

## PriWrapper

Player Replication Info - persistent player data.

### Match Stats

```cpp
int GetMatchScore();
int GetMatchGoals();
int GetMatchOwnGoals();
int GetMatchAssists();
int GetMatchSaves();
int GetMatchShots();
int GetMatchDemolishes();
```

### Player State

```cpp
bool GetbReady();
bool GetbBusy();
CarWrapper GetCar();
GameEventWrapper GetGameEvent();
UnrealStringWrapper GetPlayerName();
```

### Camera Settings

```cpp
ProfileCameraSettings GetCameraSettings();
unsigned char GetCameraPitch();
unsigned char GetCameraYaw();
```

### Team Info

```cpp
unsigned char GetPawnType();  // Team number
```

---

## CanvasWrapper

2D rendering interface for drawable callbacks.

### Position

```cpp
void SetPosition(Vector2F pos);
void SetPosition(Vector2 pos);
Vector2F GetPositionFloat();
Vector2 GetPosition();
Vector2 GetSize();
```

### Color

```cpp
void SetColor(char R, char G, char B, char A);  // 0-255
void SetColor(LinearColor color);
LinearColor GetColor();
```

### Drawing Primitives

```cpp
void DrawBox(Vector2F size);
void FillBox(Vector2F size);
void DrawLine(Vector2F start, Vector2F end);
void DrawLine(Vector2F start, Vector2F end, float width);
void DrawRect(Vector2F start, Vector2F end);
void FillTriangle(Vector2F p1, Vector2F p2, Vector2F p3);
void FillTriangle(Vector2F p1, Vector2F p2, Vector2F p3, LinearColor color);
```

### Text

```cpp
void DrawString(std::string text);
void DrawString(std::string text, float xScale, float yScale);
void DrawString(std::string text, float xScale, float yScale,
    bool dropShadow, bool wrapText = false);
Vector2F GetStringSize(std::string text, float xScale = 1, float yScale = 1);
```

### Textures

```cpp
void DrawTexture(ImageWrapper* img, float scale);
void DrawRect(float x, float y, ImageWrapper* img);
```

### Coordinate Conversion

```cpp
Vector2 Project(Vector worldLocation);       // World to screen (int)
Vector2F ProjectF(Vector worldLocation);     // World to screen (float)
std::pair<Vector2F, Vector2F> DeProject(Vector2F screenPos);  // Screen to world
    // Returns (worldOrigin, worldDirection)
```

---

## Data Structures

### Vector (3D)

```cpp
struct Vector {
    float X, Y, Z;

    float magnitude();
    void normalize();
    Vector getNormalized();
    Vector clone();

    static float dot(Vector v1, Vector v2);
    static Vector cross(Vector v1, Vector v2);
    static Vector lerp(Vector v1, Vector v2, float t);
    static Vector slerp(Vector v1, Vector v2, float t);

    // Operators: +, -, *, /, +=, -=, *=, /=
};
```

### Rotator

```cpp
struct Rotator {
    int Pitch, Yaw, Roll;
    // Pitch: [-16364, 16340]
    // Yaw/Roll: [-32768, 32764]
};

Rotator VectorToRotator(Vector v);
Vector RotatorToVector(Rotator r);
```

### Quat (Quaternion)

```cpp
struct Quat {
    float X, Y, Z, W;

    Quat conjugate() const;
    Quat normalize();

    static Quat QuatSlerp(Quat q1, Quat q2, float percent);
    static Vector RotateVectorWithQuat(Vector v, Quat q);
    static Quat RotatorToQuat(Rotator rot);
    static Rotator QuatToRotator(Quat q);
};
```

### Vector2 / Vector2F

```cpp
struct Vector2 { int X, Y; };
struct Vector2F { float X, Y; };
```

### LinearColor

```cpp
struct LinearColor {
    float R, G, B, A;  // 0-1 range
};
```

### UnrealColor

```cpp
struct UnrealColor {
    unsigned char B, G, R, A;  // Note: BGRA order, 0-255
};
```

### ControllerInput

```cpp
struct ControllerInput {
    float Throttle, Steer, Pitch, Yaw, Roll;
    float DodgeForward, DodgeStrafe;
    unsigned long Handbrake : 1;
    unsigned long Jump : 1;
    unsigned long ActivateBoost : 1;
    unsigned long HoldingBoost : 1;
    unsigned long Jumped : 1;
};
```

### PredictionInfo

```cpp
struct PredictionInfo {
    Vector Location;
    Vector Velocity;
    float Time;
};
```

### ArrayWrapper

```cpp
template<typename T>
class ArrayWrapper {
    int Count();
    T Get(int index);
    bool IsNull();
    // Supports range-for: for(auto item : array) {}
};
```

---

## Wrapper Hierarchy

```
ObjectWrapper (base memory address)
├── ActorWrapper (3D objects with physics)
│   ├── RBActorWrapper (rigid body physics)
│   │   ├── VehicleWrapper
│   │   │   └── CarWrapper
│   │   └── BallWrapper
│   ├── GameEventWrapper
│   │   └── TeamGameEventWrapper
│   │       └── ServerWrapper
│   └── PlayerControllerWrapper
├── CVarWrapper
├── CanvasWrapper
├── EngineTAWrapper
└── [Other specialized wrappers]
```

### Null Checking Pattern

All wrappers support null checking:

```cpp
CarWrapper car = gameWrapper->GetLocalCar();
if (!car) return;  // Wrapper is null
if (car.IsNull()) return;  // Alternative
```

---

## Common Event Names

Events follow the pattern `Function TAGame.ClassName.MethodName`:

```
Function TAGame.GameEvent_Soccar_TA.EventMatchEnded
Function TAGame.AchievementManager_TA.HandleMatchEnded
Function TAGame.Car_TA.SetVehicleInput
Function TAGame.Ball_TA.OnCarTouch
Function TAGame.GameEvent_TA.OnAllTeamsCreated
```

Use `HookEvent()` or `HookEventWithCaller<T>()` to subscribe.
