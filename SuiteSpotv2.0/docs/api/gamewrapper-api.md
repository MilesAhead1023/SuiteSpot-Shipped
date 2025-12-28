# GameWrapper API

The `GameWrapper` is the primary interface for interacting with the Rocket League game world. An instance of `GameWrapper` is provided to your plugin through the `gameWrapper` member of your main plugin class.

## Game State

You can check the current game state using the following functions:

-   `bool IsInGame()`: Returns `true` if you are in a match.
-   `bool IsInOnlineGame()`: Returns `true` if you are in an online match.
-   `bool IsInFreeplay()`: Returns `true` if you are in freeplay.
-   `bool IsInReplay()`: Returns `true` if you are watching a replay.
-   `bool IsInCustomTraining()`: Returns `true` if you are in a custom training pack.
-   `bool IsPaused()`: Returns `true` if the game is paused.

## Accessing Game Objects

The `GameWrapper` provides access to various game objects through wrappers. Here are some of the most important ones:

-   `ServerWrapper GetCurrentGameState()`: Returns the current game state as a `ServerWrapper`. This is the main object for accessing information about the current match, such as players, teams, and the ball.
-   `CarWrapper GetLocalCar()`: Returns the local player's car as a `CarWrapper`.
-   `CameraWrapper GetCamera()`: Returns the game camera as a `CameraWrapper`.
-   `PlayerControllerWrapper GetPlayerController()`: Returns the local player's controller as a `PlayerControllerWrapper`.
-   `MMRWrapper GetMMRWrapper()`: Returns the `MMRWrapper`, which can be used to get the player's MMR (matchmaking rank).

## Event Hooking

You can hook into game events to execute code when something happens in the game.

-   `void HookEvent(std::string eventName, std::function<void(std::string eventName)> callback)`: Hooks a function to a game event. The callback will be executed before the game processes the event.
-   `void UnhookEvent(std::string eventName)`: Unhooks a previously hooked event.
-   `void HookEventPost(std::string eventName, std::function<void(std::string eventName)> callback)`: Similar to `HookEvent`, but the callback is executed *after* the game has processed the event.
-   `void UnhookEventPost(std::string eventName)`: Unhooks a previously hooked post-event.

### `HookEventWithCaller`

For more advanced event hooking, you can use `HookEventWithCaller`. This allows you to get the specific object that called the event.

-   `template<typename T> void HookEventWithCaller(std::string eventName, std::function<void(T caller, void* params, std::string eventName)> callback)`: Hooks an event and provides the calling object, a `void*` pointer to the parameters, and the event name. `T` must be a class that inherits from `ObjectWrapper`.

## Asynchronous Execution

-   `void SetTimeout(std::function<void(GameWrapper*)> theLambda, float time)`: Executes a function after a specified delay.
-   `void Execute(std::function<void(GameWrapper*)> theLambda)`: Executes a function on the main game thread. This is useful for interacting with the game from other threads.

## UI and Drawing

-   `void RegisterDrawable(std::function<void(CanvasWrapper)> callback)`: Registers a function that will be called every frame to draw on the screen. The callback receives a `CanvasWrapper` object that you can use to draw shapes, text, and images.
-   `void UnregisterDrawables()`: Unregisters all previously registered drawables.
-implements
## Logging and Notifications

-   `void LogToChatbox(std::string text, std::string sender="BAKKESMOD")`: Logs a message to the BakkesMod chat box.
-   `void Toast(std::string title, std::string text, std::string texture = "default", float timeout = 3.5f, uint8_t toastType = 0, float width = 290.f, float height = 60.f)`: Displays a toast notification on the screen.

## Other useful functions

-   `void ExecuteUnrealCommand(std::string command)`: Executes a command in the Unreal Engine console.
-   `std::string GetCurrentMap()`: Returns the file name of the current map.
-   `UniqueIDWrapper GetUniqueID()`: Returns the local player's unique ID.
-   `UnrealStringWrapper GetPlayerName()`: Returns the local player's name.