# BakkesMod Plugin Creation

This document outlines the basic structure of a BakkesMod plugin for an AI coding agent.

## Core Concepts

A BakkesMod plugin is a C++ class that inherits from `BakkesMod::Plugin::BakkesModPlugin`. The plugin is compiled into a DLL that is loaded by BakkesMod.

### `BakkesModPlugin` Base Class

The `BakkesModPlugin` class is the foundation of any plugin. It is defined in `bakkesmod/plugin/bakkesmodplugin.h`.

```cpp
class BAKKESMOD_PLUGIN_EXPORT BakkesModPlugin
{
public:
    std::shared_ptr<CVarManagerWrapper> cvarManager;
    std::shared_ptr<GameWrapper> gameWrapper;

    virtual void onLoad();
    virtual void onUnload();
};
```

-   `onLoad()`: This is the entry point of the plugin. It is called when the plugin is loaded by BakkesMod. Use this function to initialize your plugin, register notifiers, hook events, and set up CVars.
-   `onUnload()`: This is the exit point of the plugin. It is called when the plugin is unloaded. Use this function to clean up any resources used by your plugin. Notifiers and CVars are automatically unregistered.
-   `cvarManager`: A shared pointer to the `CVarManagerWrapper`. This is used to interact with the BakkesMod console, create CVars (console variables), and register notifiers.
-   `gameWrapper`: A shared pointer to the `GameWrapper`. This is the main interface for interacting with the game. It provides access to the current game state, such as the cars, ball, and players.

### Plugin Exporting

To allow BakkesMod to load your plugin, you must export it using the `BAKKESMOD_PLUGIN` macro.

```cpp
BAKKESMOD_PLUGIN(MyPluginClass, "My Awesome Plugin", "1.0", PLUGINTYPE_FREEPLAY)
```

-   `MyPluginClass`: The name of your plugin class.
-   `"My Awesome Plugin"`: The name of your plugin as it will be displayed to the user.
-   `"1.0"`: The version of your plugin.
-   `PLUGINTYPE_FREEPLAY`: The type of your plugin. This can be one of the values from the `PLUGINTYPE` enum.

### `PLUGINTYPE` Enum

The `PLUGINTYPE` enum specifies when your plugin should be active. It is defined in `bakkesmod/plugin/bakkesmodsdk.h`.

```cpp
enum PLUGINTYPE {
    PLUGINTYPE_FREEPLAY = 0x01,
    PLUGINTYPE_CUSTOM_TRAINING = 0x02,
    PLUGINTYPE_SPECTATOR = 0x04,
    PLUGINTYPE_BOTAI = 0x08,
    PLUGINTYPE_REPLAY = 0x10,
    PLUGINTYPE_THREADED = 0x20,
    PLUGINTYPE_THREADEDUNLOAD = 0x40
};
```

### `NOTIFIER_PERMISSION` Enum

The `NOTIFIER_PERMISSION` enum is used to control when a notifier can be executed. It is defined in `bakkesmod/plugin/bakkesmodsdk.h`.

```cpp
enum NOTIFIER_PERMISSION {
    PERMISSION_ALL = 0,
    PERMISSION_MENU = (1 << 0),
    PERMISSION_SOCCAR = (1 << 1),
    PERMISSION_FREEPLAY = (1 << 2),
    PERMISSION_CUSTOM_TRAINING = (1 << 3),
    PERMISSION_ONLINE = (1 << 4),
    PERMISSION_PAUSEMENU_CLOSED = (1 << 5),
    PERMISSION_REPLAY = (1 << 6),
    PERMISSION_OFFLINE = (1 << 7) //Only when not in an online game
};
```
