# Implementation Spec: Persist all-time per-gamemode statistics

**Capability ID**: 136 | **Slug**: persist-all-time-gamemode-stats

## Overview

This capability provides persist all-time per-gamemode statistics functionality for the RocketStats plugin. It integrates with the BakkesMod SDK and interacts with other subsystems through configuration variables, game events, or UI elements.

## Control Surfaces

- **Type**: `file`
- **Evidence**: FileManagement.cpp::WriteConfig (always_gm map)

## Code Path Trace

The capability is implemented through the following code path:

```
Plugin Event/Hook
    |
    v
Handler Function
    |
    +-- Extract Data
    |
    +-- Update State
    |
    v
Side Effects (File I/O, WebSocket, UI Update)
```

## Data and State

This capability manages state through:

- **Plugin Members**: Instance variables in RocketStats class
- **Configuration**: Stored in data/rocketstats.json
- **Persistence**: Survives across plugin reloads

## Threading and Safety

- **Thread Context**: Game thread (primary)
- **Synchronization**: No locks required (single-threaded game loop)
- **Race Conditions**: None (atomic operations)

## SuiteSpot Implementation Guide

### Step 1: Understand Requirements
Review the capability title and evidence references to understand scope.

### Step 2: Examine Code Path
Review the implementation path in source files identified in evidence.

### Step 3: Implement Changes
Modify the code to implement the capability.

### Step 4: Test Implementation
Create unit tests and integration tests.

### Step 5: Verify Integration
Ensure the capability integrates properly with related subsystems.

### Step 6: Document and Deploy
Add documentation and prepare for release.

## Failure Modes

| Failure | Symptoms | Solution |
|---------|----------|----------|
| Configuration not loading | Feature disabled | Check JSON deserialization |
| State not persisting | Changes lost on reload | Verify JSON write in WriteConfig |
| Event not firing | No response to game events | Check event name in HookEvent |
| Performance degradation | Stutters or lag | Profile and optimize hot paths |

## Testing Checklist

- [ ] Feature loads without errors
- [ ] Feature integrates with related capabilities
- [ ] State persists across reload
- [ ] File output correct (if applicable)
- [ ] WebSocket broadcasts correct (if applicable)
- [ ] UI displays correctly (if applicable)
- [ ] No memory leaks or crashes
- [ ] Performance impact minimal

## Related Capabilities

See capability-dependencies.md for full dependency graph.
