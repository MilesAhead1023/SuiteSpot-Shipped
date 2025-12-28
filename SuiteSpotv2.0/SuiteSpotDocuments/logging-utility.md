# Logging Utility Guide

The BakkesMod plugin template includes a `logging.h` utility that provides a powerful and convenient logging system, building upon the `cvarManager->log()` base functionality. An AI agent should prefer this system for all diagnostic and informational output.

## Overview

The utility provides two main logging functions: `LOG` for standard messages and `DEBUGLOG` for verbose, development-only messages. It relies on a global `_globalCvarManager` pointer, which is set in `plugin.cpp`, allowing you to log from any file in your project.

### Standard Logging (`LOG`)

The `LOG` function uses modern C++20's `std::format` to provide a simple and type-safe way to format messages.

**Usage:**

```cpp
#include "logging.h" // Make sure to include the header

// ...

// Simple log message
LOG("Plugin loaded successfully.");

// Log with formatted arguments
int playerCount = 10;
std::string mapName = "Utopia Coliseum";
LOG("There are {} players on the map {}.", playerCount, mapName);
```

### Debug Logging (`DEBUGLOG`)

The `DEBUGLOG` function is designed for messages that are only useful during development. It has two key features:

1.  **Conditional Compilation:** The logs will only be compiled into the plugin if `DEBUG_LOG` is set to `true` at the top of `logging.h`. This means you can leave debug messages in your code without affecting the performance of the release version.
2.  **Source Location:** It automatically appends the file name, line number, and function name where the log was called from, providing valuable context for debugging.

**Enabling `DEBUGLOG`:**

To enable debug logging, change the constant in `logging.h`:

```cpp
// in logging.h
constexpr bool DEBUG_LOG = true; // was false
```

**Usage:**

```cpp
#include "logging.h"

// ...

CarWrapper car = gameWrapper->GetLocalCar();
Vector location = car.GetLocation();
DEBUGLOG("Local car is at location ({}, {}, {})", location.X, location.Y, location.Z);
```

**Example Console Output:**

```
Local car is at location (-100.5, 50.0, 17.0) [MyPlugin::SomeFunction (MyPlugin.cpp:42)]
```

Using `LOG` for general information and `DEBUGLOG` for detailed, development-time diagnostics is the recommended logging strategy when building a plugin from the template.
