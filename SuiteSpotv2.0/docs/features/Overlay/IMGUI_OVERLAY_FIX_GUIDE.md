# SuiteSpot ImGui Overlay System: Complete Understanding & Fix Guide

## Overview

The SuiteSpot post-match overlay is a **non-interactive, high-performance ImGui overlay** that displays player statistics, team scores, and MVP indicators after a Rocket League match ends. It integrates with the RocketStats documentation system and uses ImGui's low-level drawing APIs (ImDrawList) for custom rendering.

---

## Architecture Overview

### Component Hierarchy

```
SuiteSpot (main plugin class)
├── PostMatchInfo { active, players[], myScore, oppScore, ... }
├── PostMatchOverlayWindow : PluginWindowBase
│   ├── Render() → Called by BakkesMod every frame
│   │   ├── SetNextWindowPos/Size (positioning)
│   │   ├── ImGui::Begin() → Creates ImGui window context
│   │   ├── RenderWindow() → Calls plugin_->GetOverlayRenderer()->RenderPostMatchOverlay()
│   │   └── ImGui::End() → Finalizes window
│   │
│   └── RenderWindow() → Delegates to OverlayRenderer
│
└── OverlayRenderer (rendering engine)
    └── RenderPostMatchOverlay()
        ├── Fetch PostMatchInfo from plugin_
        ├── Calculate fade effects (elapsed time)
        ├── Get ImDrawList* from ImGui
        ├── Draw background rectangle
        ├── Draw title and match info text
        ├── Draw team sections with colored backgrounds
        └── Draw player rows with statistics
```

### Data Flow: Test Button to Overlay Display

```
1. User types: ss_testoverlay (console command)
                     ↓
2. SuiteSpot::onInit() registers notifier [line 488]
   - Notifier: "ss_testoverlay"
   - Handler lambda populates postMatch with mock data
                     ↓
3. Notifier lambda executes:
   - Checks if postMatch.active is false
   - If false:
     • Populates postMatch.players[] with 4 mock players
     • Sets postMatch.myTeamName = "Blue Team"
     • Sets postMatch.oppTeamName = "Orange Team"
     • postMatch.active = true
     • postMatchOverlayWindow->Open()
     • Logs "Test overlay ACTIVATED"
                     ↓
4. BakkesMod calls PostMatchOverlayWindow::Render() [every frame]
   - Checks if isWindowOpen_ is true
   - Sets window position and size
   - Calls ImGui::Begin() to establish context
   - Calls RenderWindow()
                     ↓
5. PostMatchOverlayWindow::RenderWindow() [line 228]
   - Calls plugin_->GetOverlayRenderer()->RenderPostMatchOverlay()
                     ↓
6. OverlayRenderer::RenderPostMatchOverlay() [line 49]
   - Validates ImGui context
   - Fetches postMatch reference
   - Calculates elapsed time and fade alpha
   - Gets ImDrawList* (low-level drawing API)
   - Draws all visual elements:
     • Background (semi-transparent black)
     • Header (darker background)
     • Title text "MATCH COMPLETE"
     • Match info line (playlist, teams, scores)
     • Column headers (Player, Score, Goals, etc.)
     • Team sections (colored background per team)
     • Player rows (name, score, goals, assists, saves, shots, ping)
     • MVP indicators (gold star ★)
```

---

## Key ImGui Concepts in SuiteSpot

### 1. **ImGui Context & Thread Management**

ImGui is a **single-context system** and is **not thread-safe**. 

> [!CAUTION]
> **CRITICAL THREAD SAFETY RULE:** Never call ImGui functions (like `Begin`, `End`, or `AddText`) inside a `RegisterDrawable` callback. 
> - `RegisterDrawable` executes on the **GPU/Rendering Thread**.
> - BakkesMod manages ImGui on the **Main/UI Thread**.
> - Mixing these will cause a **Rendering Thread Exception (Fatal Error)**.

**Correct Pattern:**
Call your window's `Render()` method inside the plugin's `RenderSettings()` (in `Source.cpp`). This ensures your overlay is drawn during the safe ImGui pass provided by BakkesMod.

```cpp
// Source.cpp
void SuiteSpot::RenderSettings() {
    if (postMatchOverlayWindow) {
        postMatchOverlayWindow->Render(); // SAFE: Executes on UI thread
    }
    // ... rest of settings
}
```

### 2. Window Lifecycle (PluginWindowBase pattern)

```cpp
// GuiBase.h inheritance
class PostMatchOverlayWindow : public PluginWindowBase {
    bool isWindowOpen_ = false;  // Tracks if window should be rendered
    std::string menuTitle_;      // Window title

    void Render() override;      // Called by BakkesMod every frame
    void RenderWindow() override; // Your custom rendering logic
};

// Lifecycle:
// 1. OnOpen() is called when window becomes active
//    └─ Sets isWindowOpen_ = true
//
// 2. Render() is called every frame (by BakkesMod)
//    └─ Calls ImGui::Begin() and ImGui::End()
//    └─ Calls your RenderWindow() implementation
//
// 3. OnClose() is called when window closes
//    └─ Sets isWindowOpen_ = false
```

**Important**: `Render()` is called even if the window is not open. Check `isWindowOpen_` first.

### 3. **Window Flags** (Post-Match Overlay Specific)

```cpp
ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration      // No title bar, borders, padding
    | ImGuiWindowFlags_NoScrollbar     // No scrollbars
    | ImGuiWindowFlags_NoSavedSettings // Don't save window state to INI
    | ImGuiWindowFlags_NoFocusOnAppearing // Don't grab keyboard/mouse focus
    | ImGuiWindowFlags_NoInputs        // Don't capture mouse/keyboard input
    | ImGuiWindowFlags_NoNavFocus      // Don't participate in navigation
    | ImGuiWindowFlags_NoBackground;   // Don't draw background (we use ImDrawList)
```

These flags make the window **invisible to ImGui's normal system**, allowing us to use raw ImDrawList for complete control.

### 4. **Window Positioning & Sizing**

```cpp
ImVec2 display = ImGui::GetIO().DisplaySize;  // Screen resolution
const ImVec2 overlaySize = ImVec2(
    std::max(400.0f, plugin_->GetOverlayWidth()),   // Min 400px wide
    std::max(180.0f, plugin_->GetOverlayHeight())   // Min 180px tall
);

// Center horizontally, position at 8% from top
ImVec2 pos = ImVec2(
    (display.x - overlaySize.x) * 0.5f + offsetX,   // Center + user offset
    display.y * 0.08f + offsetY                      // 8% from top + offset
);

ImGui::SetNextWindowPos(pos, ImGuiCond_Always);     // Force position every frame
ImGui::SetNextWindowSize(overlaySize, ImGuiCond_Always); // Force size every frame
ImGui::SetNextWindowBgAlpha(0.0f);                  // Transparent ImGui background
```

**Key**: `ImGuiCond_Always` forces the window to stay at this position/size. Without it, user dragging would move it.

### 5. **ImDrawList: Low-Level Drawing API**

```cpp
ImDrawList* dl = ImGui::GetWindowDrawList();  // Get drawing surface
```

ImDrawList provides direct access to draw commands:

- `AddRectFilled(pos_min, pos_max, color, rounding)` - Draw filled rectangle
- `AddText(font, font_size, pos, color, text)` - Draw text
- `AddLine(p1, p2, color, thickness)` - Draw line
- `AddCircle(center, radius, color, num_segments, thickness)` - Draw circle outline
- `AddCircleFilled(center, radius, color, num_segments)` - Draw filled circle

**Important**: ImDrawList draws in **screen space** (absolute screen coordinates), not window-relative.

### 6. **Color Management in ImGui**

```cpp
// Method 1: ImVec4 (RGBA normalized 0.0-1.0)
ImVec4 color = ImVec4(1.0f, 0.84f, 0.0f, 0.9f);  // Gold with 90% alpha
ImU32 col32 = ImGui::GetColorU32(color);          // Convert to packed 32-bit

// Method 2: HSV to RGB conversion (team colors)
auto hsvToRgb = [](float h, float s, float v, float a) -> ImVec4 {
    // h: 0-360 (hue), s: 0-1 (saturation), v: 0-1 (value), a: alpha
    float c = v * s;
    float x = c * (1 - abs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    // ... compute r, g, b and return ImVec4
};

ImU32 teamColor = ImGui::GetColorU32(
    hsvToRgb(plugin_->GetBlueTeamHue(), blueTeamSat, blueTeamVal, alpha)
);
```

Team colors are controlled via CVars:
- `blue_team_hue` (0-360°)
- `orange_team_hue` (0-360°)
- `blueTeamSat`, `blueTeamVal` (saturation/value of blue)
- `orangeTeamSat`, `orangeTeamVal` (saturation/value of orange)

### 7. **Font Rendering**

```cpp
ImGui::GetFont()           // Default ImGui font
mainFontSize = 14.0f       // Size for player names and stats
headerFontSize = 12.0f     // Size for column headers
teamHeaderFontSize = 16.0f // Size for team section titles

dl->AddText(ImGui::GetFont(), mainFontSize,
            ImVec2(x, y), color_u32, "Text");
```

Font is single for entire overlay. Size is controlled via float parameter.

### 8. **Alpha Blending & Fade Effects**

```cpp
// Calculate elapsed time since overlay started
auto now = std::chrono::steady_clock::now();
float elapsed = std::chrono::duration<float>(now - postMatch.start).count();
float postMatchDurationSec = plugin_->GetPostMatchDurationSec();  // Default 15s

// Fade in during first 0.5s
float alpha = plugin_->GetOverlayAlpha();  // Base alpha (0.85)
if (elapsed < fadeInDuration) {
    alpha *= (elapsed / fadeInDuration);   // Linear fade in
}
// Fade out during last 2s
else if (elapsed > postMatchDurationSec - fadeOutDuration) {
    alpha *= std::max(0.0f, (postMatchDurationSec - elapsed) / fadeOutDuration);
}

// Apply alpha to all colors:
ImVec4 colorWithAlpha(1.0f, 1.0f, 1.0f, alpha);
ImU32 finalColor = ImGui::GetColorU32(colorWithAlpha);
```

**Auto-hide logic**:
```cpp
if (postMatch.active && elapsed >= postMatchDurationSec) {
    postMatch.active = false;
    postMatchOverlayWindow->Close();
    return;  // Stop rendering
}
```

---

## Data Structure: PostMatchInfo

Located in SuiteSpot.h lines 37-62:

```cpp
struct PostMatchPlayerRow {
    int teamIndex = -1;      // 0 = blue, 1 = orange
    bool isLocal = false;    // Is this the player running the plugin?
    std::string name;        // Player name
    int score = 0;           // Player's score
    int goals = 0;           // Goals scored
    int assists = 0;         // Assists
    int saves = 0;           // Saves
    int shots = 0;           // Shots on goal
    int ping = 0;            // Network ping
    bool isMVP = false;      // Is this player MVP?
};

struct PostMatchInfo {
    bool active = false;     // Is overlay currently showing?
    std::chrono::steady_clock::time_point start;  // When did overlay start?
    int myScore = 0;         // Team score (blue)
    int oppScore = 0;        // Opponent team score (orange)
    std::string myTeamName;  // "Blue Team" or similar
    std::string oppTeamName; // "Orange Team" or similar
    std::string playlist;    // "Competitive Doubles", etc.
    bool overtime = false;   // Did match go to overtime?
    LinearColor myColor{};   // Team color (if using actual data)
    LinearColor oppColor{};  // Opponent team color
    std::vector<PostMatchPlayerRow> players;  // All 4-8 players
};
```

### Test Data Population (ss_testoverlay command)

```cpp
PostMatchPlayerRow p1;
p1.name = "LocalPlayer";
p1.score = 650;
p1.goals = 2;
p1.isLocal = true;       // Mark as local player
p1.teamIndex = 0;        // Blue team
p1.isMVP = true;         // Give MVP to local player

postMatch.players = { p1, p2, p3, p4 };  // 4 players total
```

---

## Rendering Pipeline: Frame-by-Frame Breakdown

### Frame 1: Test button pressed (ss_testoverlay)

```
1. Notifier handler executes
2. PostMatchInfo populated with test data
3. postMatch.active = true
4. postMatchOverlayWindow->Open() sets isWindowOpen_ = true
```

### Frame 2: PostMatchOverlayWindow::Render() called

```cpp
void PostMatchOverlayWindow::Render() {
    if (!isWindowOpen_) return;  // ← Skip if not open

    // Set window properties
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);  // Position on screen
    ImGui::SetNextWindowSize(overlaySize, ImGuiCond_Always);  // Size
    ImGui::SetNextWindowBgAlpha(0.0f);  // No ImGui background (we draw manually)

    // Create window context
    if (!ImGui::Begin("SuiteSpot Post-Match Overlay", &isWindowOpen_, flags)) {
        ImGui::End();
        return;
    }

    // Render the content
    RenderWindow();  // Calls OverlayRenderer::RenderPostMatchOverlay()

    // Close window context
    ImGui::End();
}
```

### Frame 3-N: OverlayRenderer::RenderPostMatchOverlay()

```cpp
// Validate ImGui context
if (!ImGui::GetCurrentContext()) {
    ImGui::SetCurrentContext(plugin_->imguiCtx);
}

// Get drawing surface
ImDrawList* dl = ImGui::GetWindowDrawList();

// 1. Draw background rectangle
dl->AddRectFilled(
    ImVec2(x, y),           // Top-left
    ImVec2(x + width, y + height),  // Bottom-right
    backgroundColor,        // Black with 40% alpha
    8.0f                    // Rounded corners (8px radius)
);

// 2. Draw header background
dl->AddRectFilled(
    ImVec2(x, y),
    ImVec2(x + width, y + 34),  // Header is 34px tall
    headerBg,               // Black with 80% alpha
    8.0f
);

// 3. Draw title text
dl->AddText(ImGui::GetFont(), 16.0f,  // Font, size
            ImVec2(x + 12, y + 8),    // Position (left margin 12px, top margin 8px)
            titleColor,               // White
            "MATCH COMPLETE");

// 4. Draw match info
dl->AddText(..., "Competitive Doubles | Blue Team 3 - 2 Orange Team");

// 5. Draw column headers (if enabled)
dl->AddText(..., "Player");
dl->AddText(..., "Score");
dl->AddText(..., "Goals");
// ... etc for all columns

// 6. Draw team sections
for (int teamIdx = 0; teamIdx <= 1; teamIdx++) {
    // Determine team color using HSV-to-RGB
    // Draw team header background (colored)
    dl->AddRectFilled(teamHeaderPos, teamHeaderEnd, teamColor, 4.0f);

    // Draw team name and score
    dl->AddText(..., "Blue Team - 3");

    // 7. Draw player rows for this team
    for (const auto& player : postMatch.players) {
        if (player.teamIndex != teamIdx) continue;

        // MVP indicator (gold star)
        if (player.isMVP) {
            dl->AddText(..., mvpCheckmarkSize, ..., "★");
        }

        // Player stats (name, score, goals, assists, saves, shots, ping)
        dl->AddText(..., player.name);
        dl->AddText(..., std::to_string(player.score));
        dl->AddText(..., std::to_string(player.goals));
        // ... etc for all stats
    }
}
```

---

## Common Issues & Fixes

### Issue 1: Overlay Not Showing

**Symptoms**: Run `ss_testoverlay`, nothing appears on screen.

**Root Causes**:

1. **ImGui context not set**
   ```cpp
   // FIX: In OverlayRenderer::RenderPostMatchOverlay()
   if (!ImGui::GetCurrentContext()) {
       if (plugin_->imguiCtx) {
           ImGui::SetCurrentContext(plugin_->imguiCtx);  // ← Add this
       } else {
           return;
       }
   }
   ```

2. **PostMatchOverlayWindow not opened**
   ```cpp
   // FIX: In test button handler
   postMatch.active = true;
   postMatchOverlayWindow->Open();  // ← Ensure this is called
   ```

3. **postMatch.active is false**
   ```cpp
   // Debug: Check in RenderPostMatchOverlay()
   if (!postMatch.active) {
       // Overlay won't display if not active
       return;  // ← This might be executing
   }
   ```

**Verification**:
- Check BakkesMod console for "Test overlay ACTIVATED" message
- Verify `postMatchOverlayWindow` is not null
- Add LOG statements to trace execution

### Issue 2: Overlay Text Invisible or Garbled

**Symptoms**: Overlay shows but text is unreadable.

**Root Causes**:

1. **Wrong color (e.g., black text on black background)**
   ```cpp
   // Check color values:
   ImU32 titleColor = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, alpha));  // Should be white

   // If background is too dark or text is black:
   ImU32 wrongColor = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 1.0f));  // ← This is black!
   ```

2. **Alpha is 0 (transparent)**
   ```cpp
   // FIX: Ensure alpha calculation
   float alpha = plugin_->GetOverlayAlpha();  // Default 0.85
   if (alpha <= 0.0f) alpha = 0.85f;  // ← Safety fallback
   ```

3. **Font size is too small (0)**
   ```cpp
   // FIX:
   float mainFontSize = 14.0f;  // Must be > 0
   float headerFontSize = 12.0f;
   float teamHeaderFontSize = 16.0f;
   ```

**Verification**:
- Print color values: `LOG("Color RGBA: " << r << " " << g << " " << b << " " << a);`
- Print font sizes: `LOG("Font size: " << mainFontSize);`
- Check CVar defaults in SettingsUI.cpp lines 594-596

### Issue 3: Overlay Position Wrong (Off-screen)

**Symptoms**: Overlay shows but is positioned at wrong location.

**Root Causes**:

1. **Display resolution not being read**
   ```cpp
   ImVec2 display = ImGui::GetIO().DisplaySize;
   // If this is (0, 0), the overlay will be at (0, 0) - top-left corner
   if (display.x < 100.0f || display.y < 100.0f) {
       LOG("ERROR: Invalid display size");  // ← Debug message
   }
   ```

2. **Offset calculation wrong**
   ```cpp
   // Current: Center horizontally, 8% from top
   ImVec2 pos = ImVec2(
       (display.x - overlaySize.x) * 0.5f + offsetX,  // Center + offset
       display.y * 0.08f + offsetY                     // 8% from top + offset
   );

   // Verify:
   // - (display.x - overlaySize.x) * 0.5f should center
   // - display.y * 0.08f should be 8% from top
   // - offsetX/Y should be ±range (e.g., ±1000 for X, ±500 for Y)
   ```

3. **SetNextWindowPos not being called**
   ```cpp
   // MUST be called before ImGui::Begin():
   ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
   ImGui::SetNextWindowSize(overlaySize, ImGuiCond_Always);
   // If missing, window position becomes undefined
   ```

**Verification**:
- Log calculated position: `LOG("Overlay pos: " << pos.x << ", " << pos.y);`
- Check display size: `LOG("Display: " << display.x << "x" << display.y);`
- Verify offset values in SettingsUI

### Issue 4: Overlay Doesn't Auto-Hide After 15 Seconds

**Symptoms**: Overlay stays forever, never disappears.

**Root Causes**:

1. **Timer not initialized**
   ```cpp
   // In notifier handler:
   postMatch.start = std::chrono::steady_clock::now();  // ← Must be called
   postMatch.active = true;
   ```

2. **elapsed calculation wrong**
   ```cpp
   auto now = std::chrono::steady_clock::now();
   float elapsed = std::chrono::duration<float>(now - postMatch.start).count();
   // elapsed should increase from 0 to postMatchDurationSec
   ```

3. **Auto-hide check never triggers**
   ```cpp
   float postMatchDurationSec = plugin_->GetPostMatchDurationSec();  // Default 15.0
   if (postMatch.active && elapsed >= postMatchDurationSec) {
       postMatch.active = false;
       postMatchOverlayWindow->Close();
       return;
   }
   // If this condition is never true:
   // - postMatch.active might be false
   // - elapsed might not be increasing
   // - postMatchDurationSec might be 0 or very large
   ```

**Verification**:
- Log elapsed time: `LOG("Elapsed: " << elapsed << "s, Duration: " << postMatchDurationSec << "s");`
- Check `postMatch.active` value: `LOG("Overlay active: " << postMatch.active);`
- Verify CVar `overlay_duration` is set: `LOG("Duration CVar: " << cvarManager->getCvar("overlay_duration").getValue<float>());`

---

## Settings Integration: SettingsUI

The post-match overlay settings are in `SettingsUI.cpp:560-843`:

```cpp
void SettingsUI::RenderPostMatchOverlaySettings() {
    // Get overlay renderer
    auto overlay = plugin_->GetOverlayRenderer();
    if (!overlay) {
        ImGui::TextColored(..., "OverlayRenderer not initialized");
        return;
    }

    // Get current CVar values
    float postMatchDurationSecValue = plugin_->GetPostMatchDurationSec();  // Read CVar
    float overlayWidthValue = plugin_->GetOverlayWidth();
    float overlayHeightValue = plugin_->GetOverlayHeight();
    // ... etc

    // Reset button
    if (ImGui::Button("Reset to Defaults")) {
        overlayWidthValue = 880.0f;
        overlayHeightValue = 400.0f;
        blueTeamHueValue = 240.0f;
        orangeTeamHueValue = 25.0f;
        overlayAlphaValue = 0.85f;
        postMatchDurationSecValue = 15.0f;

        overlay->ResetDefaults();  // Reset OverlayRenderer member variables

        // Sync back to CVars
        plugin_->cvarManager->getCvar("overlay_width").setValue(overlayWidthValue);
        plugin_->cvarManager->getCvar("overlay_height").setValue(overlayHeightValue);
        plugin_->cvarManager->getCvar("overlay_alpha").setValue(overlayAlphaValue);
        plugin_->cvarManager->getCvar("overlay_duration").setValue(postMatchDurationSecValue);
        plugin_->cvarManager->getCvar("blue_team_hue").setValue(blueTeamHueValue);
        plugin_->cvarManager->getCvar("orange_team_hue").setValue(orangeTeamHueValue);
    }

    // Sliders for all settings
    if (ImGui::SliderFloat("Display Time (sec)", &postMatchDurationSecValue, 5.0f, 60.0f, "%.1f")) {
        plugin_->cvarManager->getCvar("overlay_duration").setValue(postMatchDurationSecValue);
    }

    // ... more sliders for width, height, offset, etc.
}
```

**Control Flow**:
1. User moves slider in ImGui
2. Slider value changes `postMatchDurationSecValue` (local variable)
3. Condition checks if slider was changed: `if (ImGui::SliderFloat(...))`
4. If true, update CVar: `cvarManager->getCvar(...).setValue(...)`
5. CVar change triggers callback in RenderPostMatchOverlay() for real-time preview

---

## Step-by-Step: Fixing Post-Match Overlay

### Step 1: Verify Test Command Works

```bash
# In BakkesMod console:
ss_testoverlay

# Expected: "SuiteSpot: Test overlay ACTIVATED via ss_testoverlay" in log
```

If no message appears:
- Check `SuiteSpot::onInit()` line 488 (notifier registration)
- Verify `postMatchOverlayWindow` is created at line 485
- Check for compilation errors

### Step 2: Verify ImGui Context

Add temporary logging in OverlayRenderer::RenderPostMatchOverlay():

```cpp
void OverlayRenderer::RenderPostMatchOverlay() {
    if (!plugin_) {
        LOG("ERROR: plugin_ is null");
        return;
    }

    if (!ImGui::GetCurrentContext()) {
        LOG("WARNING: No ImGui context, setting from plugin_->imguiCtx");
        if (plugin_->imguiCtx) {
            ImGui::SetCurrentContext(plugin_->imguiCtx);
        } else {
            LOG("ERROR: plugin_->imguiCtx is null!");
            return;
        }
    } else {
        LOG("INFO: ImGui context is valid");
    }

    // ... rest of function
}
```

### Step 3: Verify PostMatchInfo Population

Add logging in test button handler (SuiteSpot.cpp:488):

```cpp
cvarManager->registerNotifier("ss_testoverlay", [this](std::vector<std::string> args) {
    LOG("ss_testoverlay called, players.size=" << postMatch.players.size());
    LOG("postMatch.active before=" << postMatch.active);

    if (postMatch.players.empty()) {
        // ... populate data ...
        LOG("Populated test data, players.size=" << postMatch.players.size());
    }

    if (!postMatch.active) {
        postMatch.start = std::chrono::steady_clock::now();
        postMatch.active = true;
        postMatchOverlayWindow->Open();
        LOG("Activated overlay, isWindowOpen_=" << postMatchOverlayWindow->isWindowOpen_);
    } else {
        // ... deactivate ...
    }
}, "Toggle the SuiteSpot test overlay", PERMISSION_ALL);
```

### Step 4: Test Rendering

Run `ss_testoverlay` and check:
1. Log message shows "ACTIVATED"
2. Overlay appears on screen
3. Click "Reset to Defaults" in settings
4. Verify text is readable
5. Wait 15 seconds (or adjust `overlay_duration` lower for faster testing)
6. Overlay should fade out and disappear

---

## RocketStats Documentation Reference

The post-match overlay aligns with RocketStats capability #115-121 (Overlay Rendering):

- **Cap #115**: Render overlay text → Implemented via `dl->AddText()`
- **Cap #118**: Render settings ImGui menu → Implemented via SettingsUI.cpp
- **Cap #120**: Element fill color → Implemented via team HSV colors
- **Cap #121**: Element stroke color → Implemented via border rectangles

Key files from RocketStatsDocs:
- `IMPLEMENTATION_MAPPING.md`: Overlay subsystem (5+ capabilities)
- `Reference/115-render-overlay-text.md`: Text rendering implementation details
- `Reference/118-render-settings-menu.md`: ImGui settings pattern

---

## Quick Reference: ImGui APIs Used in SuiteSpot

| API | Purpose | Example |
|-----|---------|---------|
| `ImGui::GetCurrentContext()` | Check if context exists | `if (!ImGui::GetCurrentContext())` |
| `ImGui::SetCurrentContext(ctx)` | Set rendering context | `ImGui::SetCurrentContext(plugin_->imguiCtx)` |
| `ImGui::GetIO()` | Access input/display info | `ImGui::GetIO().DisplaySize` |
| `ImGui::GetWindowDrawList()` | Get drawing surface | `ImDrawList* dl = ImGui::GetWindowDrawList()` |
| `ImGui::GetFont()` | Get default font | `dl->AddText(ImGui::GetFont(), size, pos, color, text)` |
| `ImGui::GetColorU32()` | Convert ImVec4 to U32 color | `ImU32 col = ImGui::GetColorU32(ImVec4(...))` |
| `ImGui::Begin()` | Open window context | `ImGui::Begin("Title", &open, flags)` |
| `ImGui::End()` | Close window context | `ImGui::End()` |
| `ImGui::SetNextWindowPos()` | Set next window position | `ImGui::SetNextWindowPos(pos, ImGuiCond_Always)` |
| `ImGui::SetNextWindowSize()` | Set next window size | `ImGui::SetNextWindowSize(size, ImGuiCond_Always)` |
| `ImGui::SetNextWindowBgAlpha()` | Set window transparency | `ImGui::SetNextWindowBgAlpha(0.0f)` |
| `ImGui::SliderFloat()` | Float slider widget | `ImGui::SliderFloat("Label", &value, min, max)` |
| `ImGui::Button()` | Button widget | `if (ImGui::Button("Text"))` |
| `ImGui::GetWindowPos()` | Get current window position | `ImVec2 pos = ImGui::GetWindowPos()` |
| `ImGui::GetWindowSize()` | Get current window size | `ImVec2 size = ImGui::GetWindowSize()` |

| ImDrawList API | Purpose | Example |
|----------------|---------|---------|
| `AddRectFilled()` | Draw filled rectangle | `dl->AddRectFilled(p1, p2, color, rounding)` |
| `AddText()` | Draw text | `dl->AddText(font, size, pos, color, text)` |
| `AddLine()` | Draw line | `dl->AddLine(p1, p2, color, thickness)` |
| `AddCircle()` | Draw circle outline | `dl->AddCircle(center, radius, color)` |
| `AddCircleFilled()` | Draw filled circle | `dl->AddCircleFilled(center, radius, color)` |
| `AddQuad()` | Draw quad outline | `dl->AddQuad(p1, p2, p3, p4, color)` |
| `AddQuadFilled()` | Draw filled quad | `dl->AddQuadFilled(p1, p2, p3, p4, color)` |

---

## Testing Checklist

- [ ] Run `ss_testoverlay` in BakkesMod console
- [ ] Overlay appears at center-top of screen
- [ ] Mock player data displays correctly
- [ ] Team headers are colored (blue/orange)
- [ ] MVP indicator (★) shows for one player per team
- [ ] Column headers visible (Player, Score, Goals, etc.)
- [ ] Overlay fades in over 0.5 seconds
- [ ] Overlay stays for ~15 seconds
- [ ] Overlay fades out over last 2 seconds
- [ ] Run `ss_testoverlay` again to close overlay early
- [ ] In settings, adjust overlay parameters (width, height, opacity)
- [ ] Changes apply in real-time while overlay is open
- [ ] Click "Reset to Defaults" button
- [ ] All settings return to default values
- [ ] No crashes or ImGui asserts in BakkesMod console
- [ ] No memory leaks detected

---

## Additional Resources

1. **ImGui Documentation**: https://github.com/ocornut/imgui
2. **BakkesMod SDK**: Plugin API for game wrapper and CVars
3. **RocketStatsDocs**: `Reference/115-121-render-*.md` for overlay pattern
4. **SuiteSpot Source**:
   - `SuiteSpot.h` (data structures)
   - `SuiteSpot.cpp` (initialization and test button)
   - `OverlayRenderer.cpp` (rendering implementation)
   - `SettingsUI.cpp` (configuration interface)
   - `GuiBase.h/cpp` (ImGui window framework)

