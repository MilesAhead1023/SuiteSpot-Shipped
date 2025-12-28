# Implementation Spec: Track menu stack pop

**Capability ID**: 78 | **Slug**: menu-pop-detection

## Overview

This capability hooks into Rocket League game events to detect track menu stack pop. The hook is registered during plugin initialization and fires whenever the corresponding event occurs.

## Control Surfaces

- **Hook Type**: Game Event Hook
- **Detection**: Event-driven callback
- **Registration**: onLoad or onInit
- **Evidence**: RocketStats.cpp:443 (HookEvent GFxData_MenuStack_TA.PopMenu)

## Code Path Trace

1. **Registration** -> gameWrapper->HookEvent() in onInit
2. **Event Fires** -> Game engine triggers event
3. **Callback Invoked** -> Plugin callback executed on game thread
4. **Processing** -> Extract data, update state
5. **Side Effects** -> Update files, broadcast WebSocket, etc.

```
Game Engine Event
    |
    v
Event matches hook pattern
    |
    v
Callback function invoked
    |
    +-- Extract event data
    |
    +-- Update plugin state
    |
    +-- Trigger side effects
```

## Data and State

**Hook Registration**:
```cpp
gameWrapper->HookEvent("Function UE4EventName.StateName",
    std::bind(&RocketStats::CallbackFunction, this,
              std::placeholders::_1));

// With caller context
gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GFxHUD_TA.HandleStatEvent",
    std::bind(&RocketStats::onStatEvent, this,
              std::placeholders::_1, std::placeholders::_2));
```

**Event Data**: Passed to callback via event wrapper class.

## Threading and Safety

**Thread Safety**:
- Callbacks execute on game thread
- No locks needed for state updates
- Safe to read/write plugin members
- WebSocket broadcasts can queue messages

**Concurrency**: Single-threaded (game thread only)

## SuiteSpot Implementation Guide

### Step 1: Identify Event
- Determine UE4 event name
- Research when event fires

### Step 2: Register Hook
- Add gameWrapper->HookEvent() in onInit()
- Use correct event name pattern
- Bind callback method

### Step 3: Implement Callback
- Extract needed event data
- Perform state updates
- Trigger side effects

### Step 4: Add Error Handling
- Validate event data
- Check game context
- Handle edge cases

### Step 5: Test Hook
- Trigger event in-game
- Verify callback executes
- Check console output

### Step 6: Verify Effects
- Ensure state persists
- Check file outputs
- Verify WebSocket broadcasts

## Failure Modes

| Failure | Symptoms | Solution |
|---------|----------|----------|
| Hook not firing | Event never detected | Verify event name/path |
| Callback crashes | Plugin crashes on event | Add validation checks |
| State not updated | Logic runs but no effect | Check event timing |
| Memory leak | Plugin memory grows | Check resource cleanup |
| Event wrong time | Fires at unexpected time | Investigate game event lifecycle |

## Testing Checklist

- [ ] Hook registers without errors
- [ ] Hook fires in correct scenario
- [ ] Callback executes successfully
- [ ] State updates correctly
- [ ] Side effects trigger
- [ ] No crashes or errors
- [ ] Multiple fires work
- [ ] Edge cases handled
- [ ] Memory stable
- [ ] Performance acceptable

## Related Capabilities

- Cap #67: Game start detection
- Cap #68: Game end detection
- Cap #70: Stat event detection
