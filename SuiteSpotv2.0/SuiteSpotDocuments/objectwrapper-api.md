# ObjectWrapper API

## Overview

`ObjectWrapper` is the most fundamental base class for all other wrapper classes in the BakkesMod SDK. Every wrapper, from `ActorWrapper` to `CarWrapper` and `BallWrapper`, inherits from `ObjectWrapper`.

It provides the core link between the C++ wrapper object you use in your code and the actual object that exists in the game's memory.

## API Description

The primary purpose of `ObjectWrapper` is to hold the memory address of the game object it represents.

### Header
`bakkesmod/wrappers/Engine/ObjectWrapper.h`

### Public Members

-   `std::uintptr_t memory_address`: The memory address of the wrapped Unreal Engine object.

### Constructor

-   `ObjectWrapper(std::uintptr_t mem)`: Constructs the wrapper object, setting its `memory_address`.

## Usage and Concepts

You will almost never use `ObjectWrapper` directly. Its importance comes from the concepts it establishes for all derived wrappers.

### Null-Checking Wrappers

A wrapper is considered "null" or invalid if its `memory_address` is `0`. It is critical to check if a wrapper is valid before attempting to use it, as trying to call functions on a null wrapper will cause the game to crash.

While `ObjectWrapper` itself only provides the `memory_address`, its subclasses (like `ActorWrapper` and everything that inherits from it) provide a more convenient, idi-omatic way to perform this check by overloading the boolean conversion operator.

The **preferred** method for null-checking any wrapper is to use the wrapper object itself in a boolean context.

```cpp
ServerWrapper server = gameWrapper->GetCurrentGameState();

// The server wrapper will be null if you are not in a match.
// This is the correct, idiomatic way to check for validity.
if (server)
{
    // It is now safe to use the server wrapper.
    BallWrapper ball = server.GetBall();
}
else
{
    // Not in a game, do not proceed.
    return;
}
```

> **Note for AI Agent:** Always generate code that performs a null-check on a wrapper using the `if (wrapper)` pattern immediately after obtaining it, unless you are certain from the context (e.g., inside an event hook that guarantees its existence) that it cannot be null. This is the most common cause of plugin crashes.
