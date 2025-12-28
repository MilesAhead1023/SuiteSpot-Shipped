# RocketStats Implementation Mapping Block
## Template for Integration into Specification Files

**Version**: 1.0
**Purpose**: Standardized metadata block for all 195 capability specifications
**Integration**: Add to each spec file after the header or as a separate companion index

---

## Template Format

### Option A: Integrated into Each Spec File

Add this block immediately after the "Related Capabilities" section in each specification:

```markdown
---

## Implementation Mapping

### Owning File(s)
- Primary: `[Primary/Owning/File.cpp]`
- Secondary: `[Secondary/Supporting/Files.cpp]` (if applicable)
- Config: `[Configuration/Files.h]` (if applicable)

### Trigger / Hook
- **Event Type**: [Hook name or trigger type]
- **Event Function**: `[FunctionName]()`
- **Hook String**: `"Function [Full.UE4.Event.Path]"`
- **Registration**: `gameWrapper->HookEvent()` or `cvarManager->registerCvar()`
- **Callback**: `&RocketStats::[CallbackMethod]`

### Data Source
- **Primary Source**: [Where data originates]
- **Data Fields**: [Struct fields or CVar variables]
- **Derivation**: [Calculation logic if applicable]
- **Dependencies**: [Other capabilities this depends on]

### State Persistence
- **Storage Type**: [In-memory only | Config-backed | JSON-persisted | File-based]
- **Persistence Mechanism**:
  - BakkesMod CVar system: [Yes/No]
  - JSON file (data/rocketstats.json): [Yes/No]
  - Text file output: [Yes/No]
  - Memory only (reset on reload): [Yes/No]
- **Recovery Path**: [How state recovers after plugin reload/crash]

### Threading Context
- **Thread Execution**: [Game thread | WebSocket thread | Background thread | Render thread]
- **Synchronization**: [Thread-safe atomic | Mutex-protected | No locks needed]
- **Race Conditions**: [None | See notes | Protected by X]

### Performance Impact
- **Trigger Frequency**: [Per-frame | Per-stat-event | On-demand | Periodic]
- **Memory Overhead**: [Negligible | Moderate | Significant]
- **File I/O**: [Never | On-demand | Periodic | Every trigger]

### Related Subsystems
- **Depends On**: [Cap #X, Cap #Y, Cap #Z]
- **Depended By**: [Cap #A, Cap #B, Cap #C]
- **Conflicts With**: [Cap #X if both enabled simultaneously]

### Implementation Code References
```cpp
// File: [Path/To/File.cpp]
// Key Structures:
struct Stats {
    [Relevant fields]
};

// Key Function:
void [FunctionName]([Parameters]) {
    // Line [NNNN]: Implementation starts
    [Key logic excerpt]
}

// CVar Registration (if applicable):
cvarManager->registerCvar("[rs_varname]", "[default]", "[description]")
    .addOnValueChanged(/* callback */);
```

### Testing Verification Checklist
- [ ] [Specific test 1]
- [ ] [Specific test 2]
- [ ] [Specific test 3]

---
```

---

## Template Variable Reference

| Variable | Example | Notes |
|----------|---------|-------|
| `[Primary/Owning/File.cpp]` | `RocketStats/Managements/StatsManagement.cpp` | Main file containing implementation |
| `[Hook name or trigger type]` | `onStatEvent` or `rs_toggle_menu notifier` | The event that triggers this capability |
| `[FunctionName]()` | `GameStart()` | C++ function name |
| `"Function [Full.UE4.Event.Path]"` | `"Function GameEvent_TA.Countdown.BeginState"` | UE4 event path for hooks |
| `[CallbackMethod]` | `GameStart` | Method name for callback binding |
| `[Where data originates]` | `Game event system` or `User CVar input` | Source of data |
| `[Struct fields or CVar variables]` | `Stats::Clear, Stats::ClearCumul` | Data fields |
| `[Calculation logic if applicable]` | `win / (win + loss)` | Formula if derived |
| `[Other capabilities this depends on]` | `Cap #82 (stat aggregation)` | Prerequisite capabilities |
| `[Storage Type]` | `JSON-persisted across sessions` | How data persists |

---

## Pre-Filled Examples by Capability Type

### Example 1: CVar Configuration Capability

```markdown
## Implementation Mapping

### Owning File(s)
- Primary: `RocketStats/RocketStats.cpp` (registration)
- Secondary: `RocketStats/Managements/VarsManagement.cpp` (callback)
- Config: `RocketStats/RocketStats.h` (member variable)

### Trigger / Hook
- **Event Type**: Configuration variable change
- **Event Function**: `RefreshTheme()` or `UpdateFiles()`
- **Registration**: `cvarManager->registerCvar()`
- **Callback**: `&RocketStats::RefreshTheme` or `&RocketStats::UpdateFiles`

### Data Source
- **Primary Source**: User input via ImGui settings menu
- **Data Fields**: `float rs_x`, `float rs_y`, `float rs_scale`
- **Derivation**: Direct user input, no calculation
- **Dependencies**: Cap #1 (Toggle settings overlay)

### State Persistence
- **Storage Type**: Config-backed (BakkesMod CVar + JSON)
- **Persistence Mechanism**:
  - BakkesMod CVar system: Yes (in-memory runtime state)
  - JSON file: Yes (saved in data/rocketstats.json)
  - Text file output: No
  - Memory only: No
- **Recovery Path**: On plugin reload, ReadConfig() restores from JSON

### Threading Context
- **Thread Execution**: Game thread (UI input processing)
- **Synchronization**: Atomic via BakkesMod CVar system
- **Race Conditions**: None (BakkesMod guarantees atomicity)

### Performance Impact
- **Trigger Frequency**: On user input only (infrequent)
- **Memory Overhead**: Negligible (single float/string value)
- **File I/O**: On settings close via WriteConfig()

### Related Subsystems
- **Depends On**: Cap #1 (settings overlay toggle)
- **Depended By**: Cap #115-121 (overlay rendering uses these values)
- **Conflicts With**: None

### Implementation Code References
```cpp
// File: RocketStats/RocketStats.cpp
// CVar Registration (line 492):
cvarManager->registerCvar("rs_x", std::to_string(rs_x), "Overlay X position (0.0-1.0)", true, true)
    .addOnValueChanged(std::bind(&RocketStats::RefreshTheme, this,
                                  std::placeholders::_1, std::placeholders::_2));

// Callback (RocketStats.cpp line ~1450):
void RocketStats::RefreshTheme(std::string old, CVarWrapper now) {
    SetRefresh(RefreshFlags_Refresh);  // Queue re-render
}
```

### Testing Verification Checklist
- [ ] CVar registers in BakkesMod without errors
- [ ] Default value appears in settings menu
- [ ] Changing value triggers callback immediately
- [ ] Overlay position updates on screen
- [ ] Value persists after plugin reload
- [ ] JSON file contains saved value in data/rocketstats.json
```

---

### Example 2: Stat Tracking Capability

```markdown
## Implementation Mapping

### Owning File(s)
- Primary: `RocketStats/Managements/StatsManagement.cpp`
- Secondary: `RocketStats/Vars/Shots.cpp` (variable handling)
- Secondary: `RocketStats/Managements/FileManagement.cpp` (persistence)

### Trigger / Hook
- **Event Type**: Game stat event
- **Event Function**: `onStatEvent()`
- **Hook String**: `"Function TAGame.GFxHUD_TA.HandleStatEvent"`
- **Registration**: `gameWrapper->HookEventWithCallerPost<ServerWrapper>()`
- **Callback**: `&RocketStats::onStatEvent`

### Data Source
- **Primary Source**: Game server stat event system
- **Data Fields**: `Stats::Clear`, `Stats::ClearCumul`, `Stats::TeamClear`, `Stats::TotalClear`
- **Derivation**: Directly incremented on event, also tracked cumulatively
- **Dependencies**: Cap #67 (game start detection), Cap #82-85 (stat aggregation)

### State Persistence
- **Storage Type**: JSON-persisted across sessions + 4-level aggregation
- **Persistence Mechanism**:
  - BakkesMod CVar system: No (not user-configurable)
  - JSON file: Yes (in data/rocketstats.json under playlists/always/always_gm)
  - Text file output: Yes (if rs_in_file && rs_file_shots enabled)
  - Memory only: No (persisted to disk on game end)
- **Recovery Path**: On plugin reload, ReadConfig() restores from JSON; 4 aggregation levels maintain independent counters

### Threading Context
- **Thread Execution**: Game thread (stat event handler)
- **Synchronization**: Atomic increment operations (no locks needed)
- **Race Conditions**: None (game thread single-threaded)

### Performance Impact
- **Trigger Frequency**: Per stat event during match (dozens per match)
- **Memory Overhead**: 4 integers per stat × 45 stats × 4 levels = negligible
- **File I/O**: On each stat if rs_in_file enabled (synchronous, blocking)

### Related Subsystems
- **Depends On**: Cap #67 (game start), Cap #82-85 (aggregation)
- **Depended By**: Cap #126 (variable substitution), Cap #130 (file output), Cap #141 (WebSocket broadcast)
- **Conflicts With**: None

### Implementation Code References
```cpp
// File: RocketStats/Managements/StatsManagement.cpp
// Stat Detection and Increment (line ~500):
if (stat_type == "Clear") {
    if (isPrimaryPlayer(event)) {
        ++stats[current.playlist].Clear;
        ++stats[current.playlist].ClearCumul;
    } else {
        ++stats[current.playlist].TeamClear;
    }
    ++stats[current.playlist].TotalClear;
    ++always.Clear;
    ++always_gm[current.playlist].Clear;

    UpdateFiles();  // Write to text files
    SocketSend("StatEvent", stat_data);  // Broadcast
}

// File: RocketStats/Vars/Shots.cpp
// Variable Handler (line ~120):
std::string RocketStats::VarShotsClear(bool write, bool force, bool default_value) {
    std::string tmp = std::to_string(GetStats().Clear);
    if (write && (force || (rs_in_file && rs_file_shots)))
        WriteInFile("RocketStats_Clear.txt", tmp);
    return tmp;
}
```

### Testing Verification Checklist
- [ ] Stat increments on corresponding game event
- [ ] Counter works across multiple matches
- [ ] Cumulative variant tracks correctly
- [ ] All 4 aggregation levels increment (session, playlist, always, gamemode)
- [ ] Team/total variants work if applicable
- [ ] File output updates when enabled
- [ ] WebSocket broadcasts stat change
- [ ] JSON persists after plugin reload
- [ ] Streak/win-loss independent of stat
```

---

### Example 3: Game Hook Capability

```markdown
## Implementation Mapping

### Owning File(s)
- Primary: `RocketStats/Managements/GameManagement.cpp`
- Secondary: `RocketStats/RocketStats.cpp` (hook registration)

### Trigger / Hook
- **Event Type**: Game state transition
- **Event Function**: `GameStart()`, `GameEnd()`, `GameDestroyed()`
- **Hook Strings**:
  - `"Function GameEvent_TA.Countdown.BeginState"` (GameStart)
  - `"Function TAGame.GameEvent_TA.OnMatchWinnerSet"` (GameEnd)
  - `"Function GameEvent_TA.OnMatchDestroyed"` (GameDestroyed)
- **Registration**: `gameWrapper->HookEvent()`
- **Callbacks**: `&RocketStats::GameStart`, `&RocketStats::GameEnd`, `&RocketStats::GameDestroyed`

### Data Source
- **Primary Source**: BakkesMod game wrapper events
- **Data Fields**: `ServerWrapper caller` containing game state, stats, player info
- **Derivation**: Extract current playlist ID, rank, division, MMR from game state
- **Dependencies**: Cap #114 (track current rank/division)

### State Persistence
- **Storage Type**: In-memory during match, JSON-persisted on end
- **Persistence Mechanism**:
  - BakkesMod CVar system: No
  - JSON file: Yes (stats written via WriteConfig() on GameEnd)
  - Text file output: Yes (updated via UpdateFiles())
  - Memory only: Session stats reset on GameStart
- **Recovery Path**: GameDestroyed() counts as loss if mid-match; GameEnd() persists all data

### Threading Context
- **Thread Execution**: Game thread (BakkesMod event handler)
- **Synchronization**: No locks needed (game thread only)
- **Race Conditions**: None

### Performance Impact
- **Trigger Frequency**: Per match (2-3 times per session)
- **Memory Overhead**: Negligible
- **File I/O**: Significant on GameEnd (JSON write, 50+ file updates)

### Related Subsystems
- **Depends On**: Cap #67 (match start detection)
- **Depended By**: Cap #110-112 (win/loss/streak), Cap #113 (MMR update), Cap #141 (WebSocket broadcast)
- **Conflicts With**: None

### Implementation Code References
```cpp
// File: RocketStats/Managements/GameManagement.cpp
// Game Start Handler (line ~50):
void RocketStats::GameStart(ServerWrapper caller) {
    is_game_started = true;

    // Extract game state
    ServerWrapper server = gameWrapper->GetGameEvent();
    current.playlist = server.GetPlaylist().GetPlaylistId();

    // Reset session stats
    session = Stats();

    // Update display
    UpdateFiles();
    SendGameState("GameStart");
}

// Game End Handler (line ~150):
void RocketStats::GameEnd(ServerWrapper caller) {
    is_game_ended = true;

    // Detect win/loss
    bool team_won = IsPlayerTeamWinner(caller);

    // Increment win/loss
    ++stats[current.playlist].games;
    if (team_won) {
        ++stats[current.playlist].win;
        // streak computation...
    } else {
        ++stats[current.playlist].loss;
    }

    // Apply to all aggregation levels
    ++always.games;
    ++always_gm[current.playlist].games;

    // Persist and notify
    WriteConfig();
    UpdateFiles();
    SendGameState("GameEnd");

    // Update MMR after server sync
    SetTimeout(3000, [this]() { UpdateMMR(); });
}
```

### Testing Verification Checklist
- [ ] GameStart fires at countdown
- [ ] GameEnd fires when match winner determined
- [ ] GameDestroyed counts quit as loss
- [ ] Win/loss increments correctly
- [ ] Streak computation works
- [ ] MMR/rank updates after delay
- [ ] All stats persist to JSON
- [ ] File outputs update
- [ ] WebSocket broadcasts game state
```

---

### Example 4: WebSocket Network Capability

```markdown
## Implementation Mapping

### Owning File(s)
- Primary: `RocketStats/Managements/SocketManagement.cpp`
- Secondary: `RocketStats/Managements/GameManagement.cpp` (broadcasts game state)
- Secondary: `RocketStats/Managements/StatsManagement.cpp` (broadcasts stat events)

### Trigger / Hook
- **Event Type**: Manual broadcast or response to client request
- **Event Function**: `SocketSend()`, `SocketBroadcast()`, `OnSocketMessage()`
- **Thread**: Dedicated background WebSocket thread
- **Port**: 8085 (fixed)

### Data Source
- **Primary Source**: Current game state, stats snapshot via GetStats()
- **Data Fields**: All Stats fields, game context flags (IsInGame, IsInMenu, etc.)
- **Derivation**: Copy-by-value from GetStats() (thread-safe)
- **Dependencies**: Cap #82-112 (all stats), Cap #67-68 (game state)

### State Persistence
- **Storage Type**: Real-time streaming (no persistence)
- **Persistence Mechanism**:
  - BakkesMod CVar system: No
  - JSON file: No
  - Text file output: No
  - Memory only: Yes (streaming protocol)
- **Recovery Path**: Client must reconnect; new client receives current state

### Threading Context
- **Thread Execution**: Dedicated WebSocket background thread
- **Synchronization**: Message queue protected; GetStats() returns copy (thread-safe)
- **Race Conditions**: None (stats accessed read-only via copy)

### Performance Impact
- **Trigger Frequency**: Per stat event + periodic game state updates
- **Memory Overhead**: Message queue, WebSocket connections (typically 1-2 clients)
- **File I/O**: None

### Related Subsystems
- **Depends On**: Cap #67-68 (game events), Cap #82-112 (stats)
- **Depended By**: OBS integration via WebSocket client scripts
- **Conflicts With**: None

### Implementation Code References
```cpp
// File: RocketStats/Managements/SocketManagement.cpp
// WebSocket Server Init (line ~50):
void InitWebSocket() {
    server.set_access_channels(websocketpp::log::alevel::all);
    server.init_asio();
    server.set_open_handler(std::bind(&RocketStats::OnSocketOpen, this, ::_1));
    server.set_message_handler(std::bind(&RocketStats::OnSocketMessage, this, ::_1, ::_2));
    server.listen(8085);
    server.start_accept();
}

// Broadcast Message (line ~200):
void RocketStats::SocketBroadcast(const std::string& message_type, const json& data) {
    json message;
    message["name"] = message_type;
    message["data"] = data;
    message["states"]["IsInGame"] = gameWrapper->IsInGame();
    message["states"]["IsInMenu"] = gameWrapper->IsInMenu();

    std::string payload = message.dump();
    for (auto connection : m_connections) {
        server.send(connection, payload, websocketpp::frame::opcode::text);
    }
}

// Background Thread (line ~400):
void RocketStats::WebSocketLoop() {
    while (plugin_running) {
        server.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
```

### Testing Verification Checklist
- [ ] Server starts and listens on port 8085
- [ ] Client can connect without errors
- [ ] GameStart broadcast sends on match start
- [ ] GameEnd broadcast sends on match end
- [ ] Stat broadcasts send per event
- [ ] Boost broadcasts work
- [ ] Message format is valid JSON
- [ ] All fields present and correct type
- [ ] Client receives all broadcasts
- [ ] Multiple clients supported
- [ ] Reconnection works
- [ ] No performance impact on game
```

---

## Implementation Mapping Index

Quick reference table for all 195 capabilities with implementation mapping block status:

```markdown
| Cap ID | Title | File(s) | Trigger | Persistence | Status |
|--------|-------|---------|---------|-------------|--------|
| 1 | Toggle settings overlay | RocketStats.cpp | rs_toggle_menu notifier | CVar + JSON | ✓ Mapped |
| 4 | Select aggregation mode | RocketStats.cpp | rs_mode CVar | CVar + JSON | ✓ Mapped |
| 67 | Detect game start | GameManagement.cpp | GameEvent_TA.Countdown.BeginState | In-memory | ✓ Mapped |
| 70 | Track stat events | StatsManagement.cpp | TAGame.GFxHUD_TA.HandleStatEvent | JSON + files | ✓ Mapped |
| 82 | Track clear counter | StatsManagement.cpp, Vars/Shots.cpp | onStatEvent | JSON + files | ✓ Mapped |
| 138 | WebSocket server init | SocketManagement.cpp | Manual init | Streaming | ✓ Mapped |
| ... | ... | ... | ... | ... | ✓ Mapped |
| 195 | Obs text file export | VarsManagement.cpp | Per-stat event | File-based | ✓ Mapped |
```

---

## Integration Methods

### Method 1: Add to Each Spec File

Insert the "Implementation Mapping" section into each specification markdown file after the "Related Capabilities" section:

```bash
# For each 001-*.md through 195-*.md:
# 1. Open spec file
# 2. Locate "## Related Capabilities" section
# 3. Insert "---" and "## Implementation Mapping" block
# 4. Fill template variables specific to that capability
# 5. Save file
```

### Method 2: Create Separate Companion Index

Create a single `IMPLEMENTATION_INDEX.md` containing all 195 capability mappings in table format for quick lookup:

```markdown
# Implementation Mapping Index - All Capabilities

| Cap ID | Title | Primary File | Trigger | Data Persistence | Threading | Status |
|--------|-------|--------------|---------|------------------|-----------|--------|
| 1 | Toggle settings overlay | RocketStats.cpp | Notifier | CVar + JSON | Game thread | ✓ |
...
| 195 | OBS text file export | VarsManagement.cpp | Per-stat | File-based | Game thread | ✓ |
```

### Method 3: Generate Programmatically

Create a Python script to auto-populate the mapping block for each spec:

```python
# generate_implementation_blocks.py
import json
from pathlib import Path

# Load capability index
with open('capability-index.json') as f:
    capabilities = json.load(f)

# Load implementation mapping data
with open('implementation_data.json') as f:
    mapping_data = json.load(f)

# For each spec file
for cap_id, cap in enumerate(capabilities, 1):
    spec_file = Path(f"specs/{cap_id:03d}-{cap['slug']}.md")

    # Read current content
    with open(spec_file, 'r') as f:
        content = f.read()

    # Generate mapping block
    mapping = create_mapping_block(cap_id, mapping_data[cap_id])

    # Insert before "## Related Capabilities" or at end
    if "## Related Capabilities" in content:
        insert_point = content.find("## Related Capabilities")
    else:
        insert_point = len(content)

    # Combine and write
    new_content = content[:insert_point] + mapping + content[insert_point:]

    with open(spec_file, 'w') as f:
        f.write(new_content)
```

---

## Data Structure for Implementation Mapping

If storing as JSON for programmatic generation:

```json
{
  "1": {
    "owning_files": ["RocketStats.cpp"],
    "secondary_files": ["WindowManagement.cpp"],
    "trigger_type": "notifier",
    "trigger_event": "rs_toggle_menu",
    "hook_string": null,
    "callback": "WindowManagement::RenderSettings",
    "data_source": "User input",
    "data_fields": ["settings_open"],
    "persistence": "CVar + JSON",
    "thread_context": "Game thread",
    "synchronization": "Atomic via BakkesMod",
    "trigger_frequency": "On-demand",
    "memory_overhead": "Negligible",
    "file_io": false,
    "depends_on": [],
    "depended_by": ["5", "6", "7"],
    "related_subsystems": "Notifiers and Commands"
  },
  "70": {
    "owning_files": ["StatsManagement.cpp"],
    "secondary_files": ["Vars/Shots.cpp", "Vars/Saves.cpp", "Vars/Goals.cpp"],
    "trigger_type": "hook",
    "trigger_event": "onStatEvent",
    "hook_string": "Function TAGame.GFxHUD_TA.HandleStatEvent",
    "callback": "&RocketStats::onStatEvent",
    "data_source": "Game server stat event",
    "data_fields": ["Clear", "Assist", "Goal", "Save"],
    "persistence": "JSON + Text files",
    "thread_context": "Game thread",
    "synchronization": "Atomic increment",
    "trigger_frequency": "Per-stat-event",
    "memory_overhead": "Negligible",
    "file_io": true,
    "depends_on": ["67", "82", "83", "84", "85"],
    "depended_by": ["126", "130", "141", "142"],
    "related_subsystems": "Statistics Tracking"
  }
}
```

---

## Recommendation

**Choose Method 1 + 2** for optimal documentation:

- **Method 1**: Add to each spec file for self-contained documentation
- **Method 2**: Create separate index for quick cross-reference and architecture overview

This provides both **detailed per-capability mapping** (in specs) and **system-wide overview** (in index).

---

## Usage Examples

### For Implementation
"When adding a new stat, consult the Implementation Mapping for Cap #82 to see the pattern for stat event detection and 4-level aggregation."

### For Debugging
"The Implementation Mapping shows that Cap #70 triggers via onStatEvent hook, runs on game thread, and persists to JSON. If stats aren't updating, check hook registration and JSON persistence."

### For Architecture
"Review the Implementation Index to understand how all 195 capabilities interconnect through hooks, CVars, and stat events."

### For Testing
"The Implementation Mapping provides thread context and persistence mechanism for each capability, enabling targeted test design."

---

*Template Generated*: 2025-12-27
*For RocketStats v4.2.1*
*Capabilities Covered*: 195/195
