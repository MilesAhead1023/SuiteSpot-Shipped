# SuiteSpot × RocketStats: UI/Overlay Integration Patterns

This document explains how SuiteSpot's post-match overlay system aligns with and can leverage patterns documented in the RocketStats capability system (located in `RocketStatsDocs/`).

---

## RocketStats Capability System Overview

RocketStats documents **195 capabilities** across 12 control surface types. These capabilities are organized into subsystems:

- **Subsystem 5**: Overlay Rendering (Caps 115-121) - **← Most relevant to SuiteSpot**
- **Subsystem 6**: Theme System (Caps 122-126)
- **Subsystem 15**: Settings & Behavior (Caps 180-194)

### Key Capabilities for Post-Match Overlay

| Cap ID | Slug | Type | Relevance |
|--------|------|------|-----------|
| **115** | render-overlay-text | ui | Core: Text rendering with `dl->AddText()` |
| **116** | render-overlay-rectangle | ui | Background boxes: `dl->AddRectFilled()` |
| **117** | render-overlay-image | ui | Not used (no image display) |
| **118** | render-settings-menu | ui | Settings window: `SettingsUI::RenderPostMatchOverlaySettings()` |
| **119** | element-rotation-transform | ui | Not used (no rotation) |
| **120** | element-fill-color | ui | Team colors: HSV→RGB with hue CVars |
| **121** | element-stroke-color | ui | Not used (no stroke/outline) |

---

## RocketStats Pattern: Variable Substitution (Caps 175-184)

RocketStats uses a **substitution pattern** for dynamic content. SuiteSpot can adopt similar patterns:

### RocketStats Pattern (from IMPLEMENTATION_MAPPING.md)

```cpp
// RocketStats::VarsReplace() replaces {{variable}} placeholders
std::map<std::string, std::string> vars;
vars["Games"] = VarGames();        // {{Games}} → "42"
vars["MMR"] = VarMMR();            // {{MMR}} → "1234"
vars["Clear"] = VarShotsClear();   // {{Clear}} → "120"

// Then substitute in theme text:
std::string text = "Games: {{Games}}, MMR: {{MMR}}";
// After substitution: "Games: 42, MMR: 1234"
```

### SuiteSpot Adaptation (Post-Match Overlay)

```cpp
// Could implement dynamic placeholder system:
struct PlayerRowFormat {
    std::string pattern;  // "{{Name}} - Score: {{Score}}, Goals: {{Goals}}"
};

// Then in rendering:
std::string RenderPlayerRow(const PostMatchPlayerRow& player, const PlayerRowFormat& fmt) {
    std::map<std::string, std::string> vars;
    vars["Name"] = player.name;
    vars["Score"] = std::to_string(player.score);
    vars["Goals"] = std::to_string(player.goals);
    vars["MVP"] = player.isMVP ? "★" : "";

    // Substitute and render
    std::string formatted = SubstituteVars(fmt.pattern, vars);
    return formatted;
}
```

**Current Implementation**: SuiteSpot uses direct field access (no substitution layer). This is fine for simple overlays but could be extended with RocketStats pattern if customization needed.

---

## RocketStats Pattern: Subsystem Organization

### RocketStats Architecture (from IMPLEMENTATION_MAPPING.md)

RocketStats organizes capabilities into subsystems with clear ownership:

```
Subsystem 5: Overlay Rendering
├── Cap 115: Render overlay text
├── Cap 116: Render overlay rectangle
├── Cap 117: Render overlay image
├── Cap 118: Render settings menu
├── Cap 119: Element rotation transform
├── Cap 120: Element fill color
└── Cap 121: Element stroke color
```

Each subsystem has:
- **Owning File(s)**: Primary implementation location
- **Trigger/Hook**: Event that initiates rendering
- **Data Source**: Where data comes from
- **State Persistence**: How state survives across reloads

### SuiteSpot Mapping (Post-Match Overlay as Subsystem)

```
SuiteSpot Subsystem: Post-Match Overlay Display
├── Cap P1: Display post-match overlay (when match ends)
├── Cap P2: Render player statistics table
├── Cap P3: Colorize team sections
├── Cap P4: Show/hide MVP indicator
├── Cap P5: Configure overlay parameters
├── Cap P6: Auto-hide after duration
└── Cap P7: Fade in/out effects

Owning Files:
- Primary: SuiteSpot/OverlayRenderer.cpp
- Secondary: SuiteSpot/SuiteSpot.h (PostMatchInfo struct)
           SuiteSpot/SettingsUI.cpp (configuration)

Trigger/Hook:
- Event Type: Game end event (from RocketStats Cap 69)
- Handler: GameEndedEvent() → Populate PostMatchInfo
- Test: ss_testoverlay notifier

Data Source:
- Primary: Game server stats (player scores, goals, etc.)
- Fallback: Mock data (for testing)

State Persistence:
- In-memory: PostMatchInfo lives in SuiteSpot member
- CVar-backed: overlay_duration, overlay_width, overlay_height, etc.
- JSON: All CVars saved to plugin config
```

---

## RocketStats Pattern: Control Surface Types

### RocketStats Control Surfaces

RocketStats documents 12 control surface types:

| Type | Purpose | Example |
|------|---------|---------|
| **cvar** | Configuration variable | `rs_toggle_overlay` |
| **stat** | Game statistic | `Stats::Clear` counter |
| **file** | Text file output | `RocketStats_Games.txt` |
| **hook** | Game event hook | `Function GameEvent_TA.Countdown.BeginState` |
| **ui** | User interface element | ImGui text, button |
| **network** | WebSocket message | `{"name": "GameState", ...}` |
| **notifier** | Console command | `rs_reset_stats` |
| **lang** | Localization string | `{ "INT": "...", "FRA": "..." }` |
| **timer** | Time-based trigger | `SetTimeout(3s) → UpdateMMR()` |
| **resource** | Embedded resource | Image file, font |
| **windows** | Plugin window | Settings menu |
| **thread** | Background thread | WebSocket server thread |

### SuiteSpot Post-Match Overlay Control Surfaces

```
Cap P1: Display overlay (when match ends)
├── Control Surfaces:
│   ├── hook: GameEnded event from BakkesMod
│   ├── cvar: ss_postmatch_enabled (enable/disable overlay)
│   ├── ui: Settings menu for overlay options
│   └── timer: Auto-hide timer (setTimeout for fade-out)

Cap P2: Render player table
├── Control Surfaces:
│   ├── ui: ImGui overlay window
│   ├── cvar: overlay_width, overlay_height
│   └── file: (optional) Write CSV/JSON to disk

Cap P3: Colorize teams
├── Control Surfaces:
│   ├── cvar: blue_team_hue, orange_team_hue
│   ├── ui: Hue slider in settings
│   └── resource: HSV→RGB color conversion

Cap P5: Configure parameters
├── Control Surfaces:
│   ├── ui: SettingsUI sliders
│   ├── cvar: 8+ CVars (duration, width, height, colors, etc.)
│   └── windows: Post-Match Settings section in SettingsUI
```

---

## RocketStats Pattern: Hook/Event System

### RocketStats Hook Pattern (from IMPLEMENTATION_MAPPING.md)

```cpp
// Registration (in onInit())
gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState",
    std::bind(&RocketStats::GameStart, this, std::placeholders::_1));

gameWrapper->HookEventWithCallerPost<ServerWrapper>(
    "Function TAGame.GFxHUD_TA.HandleStatEvent",
    std::bind(&RocketStats::onStatEvent, this, std::placeholders::_1, std::placeholders::_2));

// Implementation
void RocketStats::GameStart(std::string eventName) {
    // Extract game info
    // Reset session stats
    // UpdateFiles()
}

void RocketStats::onStatEvent(ServerWrapper caller, void* event) {
    // Extract stat type
    // Increment counters (4-level aggregation)
    // UpdateFiles()
}
```

### SuiteSpot Adoption (GameEnded Event)

SuiteSpot could implement similar hook pattern for match end:

```cpp
// In SuiteSpot::LoadHooks()
gameWrapper->HookEvent("Function GameEvent_TA.Countdown.EndState",
    std::bind(&SuiteSpot::GameEndedEvent, this, std::placeholders::_1));

// Implementation
void SuiteSpot::GameEndedEvent(std::string eventName) {
    // Get match info from game server
    // Populate PostMatchInfo struct
    //   ├─ Player names, scores, goals, assists, saves, shots
    //   ├─ Team scores, overtime flag
    //   └─ Playlist ID

    // Trigger overlay display
    postMatch.active = true;
    postMatch.start = std::chrono::steady_clock::now();
    postMatchOverlayWindow->Open();

    // Call OverlayRenderer::RenderPostMatchOverlay() on next frame
}
```

**Current Status**: SuiteSpot has `GameEndedEvent()` hook registered but implementation details depend on actual game wrapper capabilities.

---

## RocketStats Pattern: 4-Level Aggregation (Adaptable)

### RocketStats 4-Level Stat Aggregation

RocketStats tracks stats at 4 levels for flexible viewing:

```cpp
struct Stats {
    // Level 1: Current game session
    int session_clear;

    // Level 2: Per-playlist (e.g., Doubles, Solo 3v3)
    int per_playlist_clear;

    // Level 3: All-time across all playlists
    int all_time_clear;

    // Level 4: All-time per-gamemode
    int all_time_per_gamemode_clear;
};

// User selects which level to display:
void RenderStats() {
    Stats& stats = GetStats();  // Returns correct level based on rs_mode CVar
    text = "Clear: " + std::to_string(stats.clear);
}
```

### SuiteSpot Adaptation (Overlay View Modes)

SuiteSpot could extend overlay with similar view selection:

```cpp
// View mode CVar
enum PostMatchViewMode {
    VIEW_LAST_MATCH = 0,      // Only show last match
    VIEW_SESSION_AGGREGATE = 1,  // Show aggregated stats for current session
    VIEW_SEASON_STATS = 2,     // Show aggregated season stats
    VIEW_COMPARISON = 3        // Show comparison with previous matches
};

// Then in overlay:
void OverlayRenderer::RenderPostMatchOverlay() {
    PostMatchViewMode mode = GetViewMode();

    switch (mode) {
        case VIEW_LAST_MATCH:
            RenderLastMatchOverlay();      // Current implementation
            break;
        case VIEW_SESSION_AGGREGATE:
            RenderSessionStatsOverlay();   // Show running totals
            break;
        case VIEW_SEASON_STATS:
            RenderSeasonStatsOverlay();    // Cross-match statistics
            break;
    }
}
```

**Current Status**: Not implemented. SuiteSpot currently shows only last match data.

---

## RocketStats Pattern: CVar Callback System

### RocketStats Callback Pattern (from IMPLEMENTATION_MAPPING.md)

```cpp
// Registration with callback
cvarManager->registerCvar("rs_x", std::to_string(rs_x), help_text, ...)
    .addOnValueChanged(std::bind(&RocketStats::RefreshTheme, this,
                                  std::placeholders::_1, std::placeholders::_2));

// Callback implementation
void RocketStats::RefreshTheme(std::string old, CVarWrapper now) {
    SetRefresh(RefreshFlags_Refresh);  // Queue re-render next frame
}
```

### SuiteSpot Callback Pattern (Post-Match Overlay Settings)

Currently, SuiteSpot applies overlay changes immediately in SettingsUI:

```cpp
// In SettingsUI::RenderPostMatchOverlaySettings()
if (ImGui::SliderFloat("Display Time (sec)", &postMatchDurationSecValue, 5.0f, 60.0f)) {
    plugin_->cvarManager->getCvar("overlay_duration").setValue(postMatchDurationSecValue);
    // Change is applied immediately to overlay if open
}
```

Could extend with callback pattern for more complex reactions:

```cpp
// More explicit callback approach:
cvarManager->registerCvar("overlay_duration", "15.0", "Time to show overlay", true, true)
    .addOnValueChanged([this](std::string old, CVarWrapper now) {
        float newDuration = now.getValue<float>();
        // Adjust fade timing if overlay is open
        if (postMatch.active) {
            // Recalculate fade endpoints
            RefreshOverlayFadeTiming();
        }
    });
```

---

## RocketStats Pattern: JSON Persistence

### RocketStats JSON Structure (from IMPLEMENTATION_MAPPING.md)

```json
{
    "rs_mode": 1,
    "rs_theme": "Default",
    "playlists": {
        "10": { "games": 42, "win": 28, "loss": 14, "clear": 120 }
    },
    "always": { "games": 150, "win": 95, "loss": 55 },
    "always_gm": {
        "10": { "games": 42, ... },
        "11": { "games": 35, ... }
    }
}
```

### SuiteSpot Overlay Settings JSON (if extended)

```json
{
    "post_match_overlay": {
        "enabled": true,
        "duration_sec": 15.0,
        "width": 880,
        "height": 400,
        "offset_x": 0.0,
        "offset_y": 0.0,
        "opacity": 0.85,
        "blue_team_hue": 240.0,
        "orange_team_hue": 25.0,
        "fade_in_duration": 0.5,
        "fade_out_duration": 2.0,
        "show_mvp_indicator": true,
        "show_team_scores": true,
        "show_column_headers": true
    }
}
```

**Current Status**: SuiteSpot uses individual CVars (no JSON override yet). Could be extended for theme-like system.

---

## Recommended Integration Steps

### Phase 1: Current Implementation (Done)

- [x] Basic post-match overlay display
- [x] ImGui window framework (PluginWindowBase)
- [x] Test button (ss_testoverlay notifier)
- [x] Mock data population
- [x] Team color customization (HSV)
- [x] Settings UI sliders
- [x] Auto-hide timer

### Phase 2: RocketStats Pattern Adoption

- [ ] Implement `GameEndedEvent()` hook to populate PostMatchInfo from actual game data
- [ ] Add "View Mode" CVar (last match vs. session aggregate)
- [ ] Implement CVar callback pattern for dynamic updates
- [ ] Add JSON persistence layer for overlay presets

### Phase 3: Advanced Features

- [ ] Implement variable substitution system like RocketStats (Cap 175)
- [ ] Add custom format strings for player row layout
- [ ] Create "overlay themes" system (like RocketStats Cap 122-126)
- [ ] Export match data to JSON/CSV files
- [ ] Broadcast match data via WebSocket (like RocketStats Cap 138-149)

---

## File Alignment with RocketStats

| File | RocketStats Analogue | Purpose |
|------|----------------------|---------|
| `OverlayRenderer.cpp` | `OverlayManagement.cpp` | Rendering logic (caps 115-121) |
| `OverlayRenderer.h` | `OverlayManagement.h` | Configuration container |
| `SettingsUI.cpp` (overlay section) | `WindowManagement.cpp` | Settings UI (cap 118) |
| `SuiteSpot.h` (PostMatchInfo) | `RocketStats.h` (Stats struct) | Data structures |
| `SuiteSpot.cpp` (GameEndedEvent) | `GameManagement.cpp` | Event handling (cap 69) |

---

## Capability Mapping: SuiteSpot Post-Match Overlay

| SuiteSpot Feature | RocketStats Cap | Pattern Type | Ownership |
|------------------|------------------|--------------|-----------|
| Display overlay on match end | 69 (Detect game end) | Hook | GameManagement |
| Render text (names, scores) | 115 (Render text) | UI/ImGui | OverlayManagement |
| Draw team section backgrounds | 116 (Render rectangle) | UI/ImGui | OverlayManagement |
| Colorize by team | 120 (Element fill color) | UI/ImGui + CVar | OverlayManagement |
| Settings menu | 118 (Render settings) | UI/ImGui | WindowManagement |
| Duration CVar | 4-66 (CVar system) | CVar | VarsManagement |
| Auto-hide timer | 70 (Stat tick timer) | Timer | GameManagement |
| MVP indicator | (custom) | UI/ImGui | OverlayManagement |
| Fade in/out | (custom) | Timer/UI | OverlayManagement |

---

## Debugging Using RocketStats Patterns

### Pattern 1: Use RocketStats-style Logging

```cpp
// RocketStats approach:
LOG(cap_id << ": " << "Render overlay");

// SuiteSpot adaptation:
LOG("Cap P1: Display overlay (active=" << postMatch.active << ")");
LOG("Cap P2: Player table (count=" << postMatch.players.size() << ")");
```

### Pattern 2: Verify Hook Execution

```cpp
// Like RocketStats Cap 67-81 (Game hooks):
void SuiteSpot::GameEndedEvent(std::string eventName) {
    LOG("Cap 69: Detect game end (eventName=" << eventName << ")");

    // Get match info
    // Populate PostMatchInfo

    LOG("Cap P1: Overlay display activated");
}
```

### Pattern 3: CVar Change Tracking

```cpp
// Like RocketStats CVars (cap 4-66):
cvarManager->registerCvar("overlay_duration", "15.0", ...)
    .addOnValueChanged([this](std::string old, CVarWrapper now) {
        LOG("CVar: overlay_duration changed from " << old << " to "
            << now.getValue<float>());
    });
```

---

## Conclusion

SuiteSpot's post-match overlay uses **ImGui-native patterns** (direct drawing via ImDrawList), while RocketStats uses **theme-based composition** (JSON-driven element placement via TinyExpr evaluation).

However, both systems share fundamental patterns:
- **Hook-based event detection** (game state changes)
- **CVar configuration system** (user settings)
- **Real-time UI updates** (ImGui integration)
- **JSON persistence** (settings serialization)

By adopting RocketStats patterns incrementally, SuiteSpot can extend overlay capabilities while maintaining clean architecture alignment.

