# Plugin Deployment Guide

This guide explains the process of deploying a compiled BakkesMod plugin. The process involves copying the plugin artifacts to the correct directories within the BakkesMod installation folder.

## Deployment Steps

Deployment requires placing the compiled plugin files into the user's BakkesMod folder. The location of this folder can vary, but it can be found by opening the BakkesMod menu in-game (F2) and selecting `File -> Open BakkesMod folder`.

### 1. Locate Plugin Artifacts

After a successful compilation, the following files are generated. The primary artifact is the plugin's DLL.

-   **Plugin DLL (`.dll`):** The compiled C++ code. For a project named `MyPlugin`, this would be `MyPlugin.dll`. This file is typically found in a `plugins/` or `Release/` directory within the project's build output.
-   **Plugin Settings (`.set`):** Some plugins may also have associated settings files. These are not automatically generated but are created by BakkesMod when a plugin saves settings. For deployment, you typically only need the `.dll`.

### 2. Copy to BakkesMod Directory

The compiled files must be copied to the following locations within the main BakkesMod folder:

1.  **Copy the `.dll` file to the `plugins` folder:**
    -   **Source:** `[YourProject]/plugins/MyPlugin.dll`
    -   **Destination:** `[BakkesModFolder]/plugins/MyPlugin.dll`

2.  **(If applicable) Copy the `.set` file to the `settings` folder:**
    -   **Source:** `[YourProject]/settings/MyPlugin.set`
    -   **Destination:** `[BakkesModFolder]/settings/MyPlugin.set`

### 3. Loading the Plugin

Once the `.dll` is in the `plugins` folder, BakkesMod needs to be instructed to load it. This can be done in two ways:

1.  **Manual Load (for testing):**
    -   Open the BakkesMod console (F6).
    -   Type the command: `plugin load MyPlugin` (do not include the `.dll` extension).
    -   The console will output whether the plugin was loaded successfully.

2.  **Automatic Load (on game start):**
    -   To have the plugin load every time the game starts, you need to add it to the `plugins.cfg` file.
    -   **File location:** `[BakkesModFolder]/cfg/plugins.cfg`
    -   **Action:** Open the file and add the line `plugin load MyPlugin` to it.

### Summary

For a basic automated deployment script, the core logic would be:

1.  Identify the BakkesMod installation path.
2.  Copy the compiled `[PluginName].dll` to the `[BakkesModPath]/plugins/` directory.
3.  Ensure the line `plugin load [PluginName]` exists in `[BakkesModPath]/cfg/plugins.cfg`.

## Advanced: Automated Hot-Reloading

For a much faster development workflow, you can automate the process of unloading the old plugin, deploying the new `.dll`, and reloading it, all without restarting Rocket League. This process is often called "hot-reloading" or "hotswapping". The `bakkes_patchplugin.py` script in the SDK is an example of this workflow.

### Concept

The process leverages the BakkesMod remote console (rcon), which runs a WebSocket server on `ws://127.0.0.1:9002` by default. By sending rcon commands, a script can control the plugin manager remotely.

### Hot-Reload Workflow

1.  **Connect to RCON:** Your script establishes a WebSocket connection to the BakkesMod rcon server.
2.  **Authenticate:** It sends the rcon password to authenticate. The default password is often `password`.
3.  **Unload Plugin:** It sends the `plugin unload [PluginName]` command. This is crucial as it allows the operating system to release its lock on the `.dll` file.
4.  **Replace DLL:** The script then replaces the old `[PluginName].dll` file in the `[BakkesModFolder]/plugins/` directory with the newly compiled version.
5.  **Load Plugin:** Finally, the script sends the `plugin load [PluginName]` command to load the new version of the plugin.

This entire sequence happens in a fraction of a second, allowing a developer to compile their code and see the changes in-game almost instantly. An AI agent could be tasked with creating or modifying such a script to fit a specific development environment (e.g., by updating the hardcoded path to the plugins folder).
