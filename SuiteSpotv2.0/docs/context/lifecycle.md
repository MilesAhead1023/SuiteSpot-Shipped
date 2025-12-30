# Lifecycle & Automation Context

## Purpose
Manages plugin initialization, CVar synchronization, event hooking, and post-match automation.

## Key Components
- **SuiteSpot**: Main class handling `onLoad`, `onUnload`, and event routing.
- **SettingsSync**: Centralized CVar registration and state persistence.
- **AutoLoadFeature**: Logic for automatically queuing or loading maps when a match ends.

## Data Flow
1. **Hook**: `LoadHooks` registers for `EventMatchEnded`.
2. **Event**: `GameEndedEvent` captures state and triggers `AutoLoadFeature`.
3. **Execute**: `AutoLoadFeature` issues `load_freeplay`, `load_training`, or `queue` commands.

## Files
- `SuiteSpot.h/cpp`
- `SettingsSync.h/cpp`
- `AutoLoadFeature.h/cpp`

## Triggers
Keywords: onLoad, onUnload, Hook, CVar, MatchEnded.
Files: `SuiteSpot.*`, `SettingsSync.*`, `AutoLoadFeature.*`.
