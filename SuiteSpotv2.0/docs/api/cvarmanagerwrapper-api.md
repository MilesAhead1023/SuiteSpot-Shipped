# CVarManagerWrapper API

The `CVarManagerWrapper` is the interface to the BakkesMod console system. It allows you to create and manage console variables (`CVars`) and console commands (`notifiers`). An instance of this wrapper is available as the `cvarManager` member of your main plugin class.

## Executing Commands

You can execute any command in the BakkesMod console, just as a user would.

-   `void executeCommand(std::string command, bool log = true)`: Executes the given command string. If `log` is true, the command will be logged to the console.

    ```cpp
    // Example: Make the ball blue in freeplay
    cvarManager->executeCommand("freeplay_ball_color 0 0 255 255");
    ```

## Notifiers (Console Commands)

Notifiers are custom commands that you can define and then execute from the console.

-   `void registerNotifier(std::string cvar, std::function<void(std::vector<std::string>)> notifier, std::string description, unsigned char permissions)`: Registers a function to be called when a command is entered in the console.

    -   `cvar`: The name of the command.
    -   `notifier`: A `std::function` that will be called. It receives a `std::vector<std::string>` containing the command arguments.
    -   `description`: A help text description of the command.
    -   `permissions`: A bitmask of `NOTIFIER_PERMISSION` flags that determine when the command can be used.

    ```cpp
    // In your onLoad() function:
    cvarManager->registerNotifier("my_custom_command", [this](std::vector<std::string> args) {
        cvarManager->log("My custom command was executed!");
        if (args.size() > 1) {
            cvarManager->log("It was called with argument: " + args[1]);
        }
    }, "An example custom command", PERMISSION_ALL);
    ```
    *Usage in console:* `my_custom_command hello`

-   `bool removeNotifier(std::string cvar)`: Removes a notifier that was previously registered.

## CVars (Console Variables)

CVars are variables that can be inspected and modified through the console. They are useful for storing settings for your plugin.

-   `CVarWrapper registerCvar(std::string cvar, std::string defaultValue, ...)`: Registers a new CVar.

    -   `cvar`: The name of the CVar.
    -   `defaultValue`: The initial string value of the CVar.
    -   `desc`: A help text description.
    -   `...`: Other optional parameters for min/max values, searchability, and whether it should be saved to the config.

    ```cpp
    // In your onLoad() function:
    cvarManager->registerCvar("my_plugin_enabled", "1", "Enable or disable my plugin", true, true, 0, true, 1);
    ```

-   `CVarWrapper getCvar(std::string cvar)`: Gets a wrapper for an existing CVar, allowing you to read or change its value.

    ```cpp
    CVarWrapper myCvar = cvarManager->getCvar("my_plugin_enabled");
    bool isEnabled = myCvar.getBoolValue();
    myCvar.setValue(false);
    ```

-   `bool removeCvar(std::string cvar)`: Removes a CVar.

## Logging

-   `void log(std::string text)`: Prints a message to the BakkesMod console. This is the primary way to output debug information.

## Binds and Aliases

`CVarManagerWrapper` also provides functions for managing key binds and command aliases, though these are less commonly used within plugins themselves.

-   `void setBind(std::string key, std::string command)`: Binds a key to a command.
-   `void removeBind(std::string key)`: Removes a key bind.
