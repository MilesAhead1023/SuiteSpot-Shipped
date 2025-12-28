# SuiteSpot Post-Match Overlay: Quick Reference

## Test Button Command

```bash
ss_testoverlay    # Toggle test overlay on/off
```

**Expected Output**:
```
SuiteSpot: Test overlay ACTIVATED via ss_testoverlay
[overlay appears on screen, shows mock player data]
[after 15 seconds, overlay fades out and disappears]
[run ss_testoverlay again to close it immediately]
```

---

## Code Files (Key Locations)

| File | Lines | Purpose |
|------|-------|---------|
| `SuiteSpot.h` | 37-62 | PostMatchInfo & PostMatchPlayerRow structs |
| `SuiteSpot.h` | 169-184 | PostMatchOverlayWindow class definition |
| `SuiteSpot.cpp` | 180-232 | PostMatchOverlayWindow implementation |
| `SuiteSpot.cpp` | 485-516 | Test button (ss_testoverlay) registration |
| `OverlayRenderer.h` | 1-138 | OverlayRenderer class definition |
| `OverlayRenderer.cpp` | 1-217 | RenderPostMatchOverlay() implementation |
| `SettingsUI.cpp` | 560-843 | Post-match overlay settings UI |
| `GuiBase.h` | 1-30 | PluginWindowBase framework |
| `GuiBase.cpp` | 1-67 | PluginWindowBase implementation |

---

## How It Works: Execution Flow

```
1. User types: ss_testoverlay
   └─→ Notifier handler (SuiteSpot.cpp:488)

2. Handler populates PostMatchInfo
   ├─ postMatch.players[] = { 4 mock players }
   ├─ postMatch.myTeamName = "Blue Team"
   ├─ postMatch.oppTeamName = "Orange Team"
   └─ postMatch.active = true

3. PostMatchOverlayWindow::Open() called
   └─ Sets isWindowOpen_ = true

4. BakkesMod calls SuiteSpot::RenderSettings() [every frame when F2 is open, or handled by SDK]
   ├─ postMatchOverlayWindow->Render() 
   │  ├─ ImGui::Begin("SuiteSpot Post-Match Overlay", ...)
   │  ├─ RenderWindow() → delegates to OverlayRenderer
   │  └─ ImGui::End()
   └─ RenderMainSettingsWindow()

5. OverlayRenderer::RenderPostMatchOverlay() [every frame]
   ├─ Validate ImGui context
   ├─ Get ImDrawList* for drawing
   ├─ Draw background (semi-transparent black)
   ├─ Draw header (darker background)
   ├─ Draw title "MATCH COMPLETE"
   ├─ Draw match info (scores, playlist)
   ├─ Draw column headers
   ├─ Loop teams (0=blue, 1=orange):
   │  ├─ Draw team header with team color
   │  └─ Loop players on this team:
   │     └─ Draw player row (name, score, goals, assists, saves, shots, ping)
   └─ Calculate fade: alpha = 1.0 - (elapsed / fadeOutDuration)

6. Every frame, check elapsed time
   ├─ If elapsed >= 15 seconds (default overlay_duration):
   │  ├─ postMatch.active = false
   │  ├─ postMatchOverlayWindow->Close()
   │  └─ Stop rendering
   └─ Overlay disappears
```

---

## Data Structures

### PostMatchPlayerRow
```cpp
struct PostMatchPlayerRow {
    int teamIndex;      // 0 = blue, 1 = orange
    bool isLocal;       // Is this the player running the plugin?
    std::string name;   // "LocalPlayer", "Opponent 1", etc.
    int score;          // Player score (0-999)
    int goals;          // Goals (0-10)
    int assists;        // Assists (0-10)
    int saves;          // Saves (0-10)
    int shots;          // Shots (0-20)
    int ping;           // Network ping (ms)
    bool isMVP;         // Gold star indicator
};
```

### PostMatchInfo
```cpp
struct PostMatchInfo {
    bool active;                              // Is overlay showing?
    std::chrono::steady_clock::time_point start;  // When did it start?
    int myScore;                              // Blue team score
    int oppScore;                             // Orange team score
    std::string myTeamName;                   // "Blue Team"
    std::string oppTeamName;                  // "Orange Team"
    std::string playlist;                     // "Competitive Doubles"
    bool overtime;                            // Did match go OT?
    std::vector<PostMatchPlayerRow> players;  // 4-8 players total
};
```

---

## ImGui Window Flags Explained

```cpp
ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration       // Remove title bar, borders
    | ImGuiWindowFlags_NoScrollbar      // No scrollbars
    | ImGuiWindowFlags_NoSavedSettings  // Don't save window state
    | ImGuiWindowFlags_NoFocusOnAppearing   // Don't steal focus
    | ImGuiWindowFlags_NoInputs         // Don't capture mouse/keyboard
    | ImGuiWindowFlags_NoNavFocus       // Don't participate in nav
    | ImGuiWindowFlags_NoBackground;    // We draw our own background
```

**Result**: Invisible ImGui window that only serves as context for drawing.

---

## Drawing API Cheat Sheet

```cpp
ImDrawList* dl = ImGui::GetWindowDrawList();

// Filled rectangle (team header background)
dl->AddRectFilled(
    ImVec2(x, y),           // Top-left corner
    ImVec2(x+width, y+height),  // Bottom-right corner
    ImU32 color,            // RGBA color
    8.0f                    // Rounded corner radius
);

// Text (player name, stats)
dl->AddText(
    ImGui::GetFont(),       // Font pointer
    14.0f,                  // Font size in pixels
    ImVec2(x, y),           // Position (top-left)
    ImU32 color,            // RGBA color
    "Player Name"           // Text string
);

// Convert ImVec4 to ImU32 color
ImU32 color32 = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.9f));
```

---

## CVar Access Patterns

```cpp
// Read CVar value
float duration = plugin_->GetPostMatchDurationSec();
float width = plugin_->GetOverlayWidth();
float height = plugin_->GetOverlayHeight();

// These internally call:
// cvarManager->getCvar("overlay_duration").getValue<float>()

// Write CVar value
plugin_->cvarManager->getCvar("overlay_duration").setValue(15.0f);
plugin_->cvarManager->getCvar("blue_team_hue").setValue(240.0f);
```

---

## Common Settings

| CVar | Default | Range | Purpose |
|------|---------|-------|---------|
| `overlay_duration` | 15.0 | 5.0-60.0 | Seconds to display overlay |
| `overlay_width` | 880 | 400-1600 | Overlay width in pixels |
| `overlay_height` | 400 | 200-800 | Overlay height in pixels |
| `overlay_alpha` | 0.85 | 0.0-1.0 | Overall opacity |
| `blue_team_hue` | 240.0 | 0-360 | Blue team color (hue) |
| `orange_team_hue` | 25.0 | 0-360 | Orange team color (hue) |
| `blue_team_sat` | 0.8 | 0.0-1.0 | Blue saturation |
| `blue_team_val` | 0.6 | 0.0-1.0 | Blue value/brightness |
| `orange_team_sat` | 0.9 | 0.0-1.0 | Orange saturation |
| `orange_team_val` | 0.7 | 0.0-1.0 | Orange value/brightness |

---

## Debug Logging

Add to OverlayRenderer.cpp:

```cpp
void OverlayRenderer::RenderPostMatchOverlay() {
    // Check ImGui context
    LOG("ImGui context: " << (ImGui::GetCurrentContext() ? "VALID" : "NULL"));

    // Check PostMatchInfo
    const auto& postMatch = plugin_->GetPostMatchInfo();
    LOG("Overlay active: " << (postMatch.active ? "YES" : "NO"));
    LOG("Players count: " << postMatch.players.size());

    // Check elapsed time
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - postMatch.start).count();
    LOG("Elapsed: " << elapsed << "s, Duration: " << plugin_->GetPostMatchDurationSec() << "s");

    // Check display size
    ImVec2 display = ImGui::GetIO().DisplaySize;
    LOG("Display: " << display.x << "x" << display.y);
}
```

---

## Troubleshooting Checklist

### Overlay doesn't appear
- [ ] Run `ss_testoverlay` (check console for "ACTIVATED" message)
- [ ] Check `postMatchOverlayWindow` is not null
- [ ] Verify ImGui context is valid: `ImGui::GetCurrentContext() != nullptr`
- [ ] Check `postMatch.active == true`
- [ ] Verify `isWindowOpen_ == true`

### Text is invisible/garbled
- [ ] Check text color is white: `ImVec4(1.f, 1.f, 1.f, alpha)`
- [ ] Check background is dark: `ImVec4(0.f, 0.f, 0.f, 0.4f)`
- [ ] Check font size > 0: `mainFontSize = 14.0f`
- [ ] Check alpha > 0: `plugin_->GetOverlayAlpha() = 0.85f`

### Overlay position wrong
- [ ] Check display size: `ImGui::GetIO().DisplaySize` should be ~1920x1080
- [ ] Verify position calculation: `(display.x - overlaySize.x) * 0.5f`
- [ ] Check offset values: `offsetX` and `offsetY`
- [ ] Ensure `ImGui::SetNextWindowPos()` is called before `ImGui::Begin()`

### Overlay doesn't auto-hide
- [ ] Check `overlay_duration` CVar (default 15.0 seconds)
- [ ] Verify elapsed time calculation: `std::chrono::steady_clock::now() - postMatch.start`
- [ ] Check fade-out condition: `elapsed >= postMatchDurationSec`
- [ ] Verify `postMatch.active = false` is being set

### Settings don't apply
- [ ] Ensure slider value is written back to CVar: `cvarManager->getCvar(...).setValue(...)`
- [ ] Check overlay is open when changing settings
- [ ] Verify callback is triggered: check if condition `if (ImGui::SliderFloat(...))` is true
- [ ] Look for compilation errors in SettingsUI.cpp

---

## Settings Panel Location

In SuiteSpot settings menu:

```
SuiteSpot Settings
├── Loadouts
├── Maps & Playlists
├── Training
├── Workshop
├── Prejump Packs
├── Post-Match Overlay  ← HERE
│   ├─ Reset to Defaults [Button]
│   ├─ Window Layout [Collapsing Header]
│   │  ├─ Display Time (sec) [Slider: 5-60]
│   │  ├─ Width [Slider: 400-1600]
│   │  ├─ Height [Slider: 200-800]
│   │  ├─ Offset X [Slider: -1000 to 1000]
│   │  └─ Offset Y [Slider: -500 to 500]
│   ├─ Team Sections [Collapsing Header]
│   │  ├─ Header Height [Slider]
│   │  ├─ Player Row Height [Slider]
│   │  ├─ Section Spacing [Slider]
│   │  └─ Section Padding [Slider]
│   ├─ Column Positions [Collapsing Header]
│   │  ├─ Name Column X [Slider]
│   │  ├─ Score Column X [Slider]
│   │  └─ ... (6 more column positions)
│   ├─ Text & Font [Collapsing Header]
│   │  ├─ Main Font Size [Slider]
│   │  ├─ Header Font Size [Slider]
│   │  └─ Team Header Font Size [Slider]
│   ├─ Team Colors [Collapsing Header]
│   │  ├─ Blue Team Hue [Slider: 0-360]
│   │  ├─ Blue Saturation [Slider]
│   │  ├─ Blue Brightness [Slider]
│   │  ├─ Orange Team Hue [Slider: 0-360]
│   │  ├─ Orange Saturation [Slider]
│   │  └─ Orange Brightness [Slider]
│   ├─ Effects [Collapsing Header]
│   │  ├─ Background Alpha [Slider]
│   │  ├─ Header Alpha [Slider]
│   │  ├─ MVP Checkmark Size [Slider]
│   │  ├─ Show MVP Glow [Checkbox]
│   │  ├─ Show Team Scores [Checkbox]
│   │  ├─ Show Column Headers [Checkbox]
│   │  ├─ Fade In Duration [Slider]
│   │  ├─ Fade Out Duration [Slider]
│   │  └─ Enable Fade Effects [Checkbox]
│   └─ 💡 Changes apply in real-time to the test overlay
└─ About
```

---

## Hue Reference (for team colors)

```
Hue Wheel (0-360°):
  0°   Red
  60°  Yellow
  120° Green (Lime)
  180° Cyan
  240° Blue
  300° Magenta

Current Defaults:
  Blue Team: 240° (pure blue)
  Orange Team: 25° (orange-yellow)
```

---

## Common Customizations

### Make overlay bigger
1. In settings: increase "Width" and "Height" sliders
2. Or in code: `OverlayRenderer::ResetDefaults()` line 11

### Change team colors to purple/pink
1. In settings: set "Blue Team Hue" to 300° (magenta)
2. Set "Orange Team Hue" to 330° (pink)

### Fade in faster
1. In code: `OverlayRenderer.h` line 134
2. Change `fadeInDuration = 0.5f` to `fadeInDuration = 0.2f`

### Keep overlay longer
1. In settings: increase "Display Time (sec)" to 30.0
2. Or via command: `ss_overlay_duration 30`

### Disable MVP indicators
1. In settings: uncheck "Show MVP Glow" checkbox
2. Or in code: `OverlayRenderer::ResetDefaults()` line 40

---

## Performance Notes

- **Rendering**: ~0.1ms per frame (ImDrawList is very fast)
- **Memory**: ~2KB for PostMatchInfo struct
- **CPU**: Minimal (only runs when overlay is active)
- **GPU**: Handled by ImGui's batched drawing
- **No allocations**: Uses stack and std::vector (pre-allocated during init)

---

## RocketStats Documentation Cross-Reference

Key documents in `RocketStatsDocs/`:

- `IMPLEMENTATION_MAPPING.md`: Overlay subsystem patterns (caps 115-121)
- `Reference/115-render-overlay-text.md`: Text rendering pattern
- `Reference/116-render-overlay-rectangle.md`: Rectangle drawing pattern
- `Reference/118-render-settings-menu.md`: ImGui settings pattern
- `Reference/120-element-fill-color.md`: Color management pattern

---

## Links to Full Documentation

- **IMGUI_OVERLAY_FIX_GUIDE.md**: Complete ImGui and overlay system guide
- **ROCKETSTATS_INTEGRATION_PATTERNS.md**: How SuiteSpot aligns with RocketStats patterns

