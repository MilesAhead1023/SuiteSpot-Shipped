# Implementation Spec: Track team statistics separately

**Capability ID**: 161 | **Slug**: track-team-stats

## Overview

This capability tracks track team statistics separately in Rocket League matches. The statistic is accumulated across four aggregation levels (session, per-playlist, all-time, all-time per-gamemode) and synchronized with file output and WebSocket streams.

## Control Surfaces

- **Stat Tracking**: Game event detection and counting
- **Detection**: onStatEvent hook in StatsManagement.cpp
- **Aggregation**: 4 levels (session, per-playlist, all-time, per-gamemode)
- **Evidence**: RocketStats.h (Team* fields), StatsManagement.cpp::onStatEvent

## Code Path Trace

1. **Event Fired** -> Game event system triggers onStatEvent callback
2. **Type Check** -> Verify stat type matches game event
3. **Player Check** -> isPrimaryPlayer() confirms it's the player's own stat
4. **Increment** -> All 4 aggregation levels incremented
5. **Persistence** -> Stat saved to JSON on game end
6. **Display** -> Vars handler creates text for overlay/files

```
Game Server Event
    |
    v
onStatEvent() hook
    |
    +-- Determine stat type
    |
    +-- Check isPrimaryPlayer()
    |
    +-- Increment all 4 levels:
    |       - session.*
    |       - stats[playlist].*
    |       - always.*
    |       - always_gm[playlist].*
    |
    v
UpdateFiles() / SocketSend()
```

## Data and State

**Stats Structure**:
```cpp
struct Stats {
    int STAT_NAME = 0;           // Base counter
    int STAT_NAMECumul = 0;      // Cumulative
    int TeamSTAT_NAME = 0;       // Team aggregate
    int TotalSTAT_NAME = 0;      // Self + team
};

// 4 Aggregation Levels
Stats session;                      // Current game only
std::map<int, Stats> stats;        // Per-playlist
Stats always;                       // All-time
std::map<int, Stats> always_gm;    // Per-gamemode
```

**JSON Persistence**:
```json
{
    "playlists": {
        "10": {"STAT_NAME": 42}
    },
    "always": {"STAT_NAME": 150}
}
```

## Threading and Safety

**Thread Safety**:
- Stat increments on game thread only
- GetStats() returns copy (thread-safe)
- JSON writes are blocking but acceptable
- No race conditions

**Concurrency**: Single-threaded (game thread)

## SuiteSpot Implementation Guide

### Step 1: Verify Event Detection
- Confirm stat type recognized in onStatEvent()
- Check game event system identifies stat correctly

### Step 2: Implement Increment
- Add increment for all 4 aggregation levels
- Handle team/total variants if applicable

### Step 3: Add File Output
- Create VarStats[STAT]() handler in Vars/
- Implement WriteInFile() for .txt output

### Step 4: Update Variable Substitution
- Add [STAT] to ReplaceStats()
- Update theme definitions with new variable

### Step 5: Add Optional CVars
- Register rs_file_[CATEGORY] for output toggle
- Register rs_hide_[CATEGORY] for overlay hiding

### Step 6: Test and Validate
- Play matches, verify stat increments
- Check file output and WebSocket
- Verify persistence across reload

## Failure Modes

| Failure | Symptoms | Solution |
|---------|----------|----------|
| Not incrementing | Counter stays at 0 | Verify event type in hook |
| Over-counting | Multiple increments per event | Check isPrimaryPlayer() |
| Lost on reload | Resets to 0 | Check JSON serialization |
| File not updating | .txt files don't change | Verify rs_in_file is enabled |
| Overlay shows 0 | Display blank despite stat | Check variable substitution |

## Testing Checklist

- [ ] Event detection works
- [ ] Counter increments on event
- [ ] Cumulative variant tracks across games
- [ ] All 4 levels increment
- [ ] Team variant (if used) works
- [ ] File output writes correctly
- [ ] WebSocket broadcasts stat
- [ ] Overlay displays value
- [ ] JSON persists correctly
- [ ] Related stats independent

## Related Capabilities

- Cap #82-85: Stat aggregation levels
- Cap #165: Update files on change
- Cap #142: Broadcast game state
