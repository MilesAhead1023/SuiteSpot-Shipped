# SuiteSpot Post-Match Overlay Documentation Suite

## Overview

This documentation suite provides comprehensive guidance on understanding, using, and fixing the SuiteSpot post-match overlay system. The overlay displays player statistics, team scores, and MVP indicators after a Rocket League match ends, using ImGui's high-performance low-level drawing API.

**Quick Start**: See [QUICK_REFERENCE.md](QUICK_REFERENCE.md)

---

## Documentation Files

### 1. **QUICK_REFERENCE.md** ⚡
**When to use**: You need fast answers

**Contains**:
- Test button command (`ss_testoverlay`)
- Code file locations with line numbers
- Execution flow overview
- Data structure definitions
- ImGui API cheat sheet
- Common settings reference
- Troubleshooting checklist
- Hue/color reference

**Best for**: Developers actively working on the overlay

---

### 2. **IMGUI_OVERLAY_FIX_GUIDE.md** 📚
**When to use**: You want complete technical understanding

**Contains**:
- Full architecture overview with ASCII diagrams
- Component hierarchy and data flow
- 8 key ImGui concepts with code examples:
  1. Context management
  2. Window lifecycle
  3. Window flags
  4. Positioning & sizing
  5. ImDrawList drawing API
  6. Color management (including HSV→RGB)
  7. Font rendering
  8. Alpha blending & fade effects
- PostMatchInfo data structure (detailed)
- Frame-by-frame rendering breakdown
- 4 common issues with root causes and fixes:
  1. Overlay not showing
  2. Text invisible/garbled
  3. Overlay position wrong
  4. Auto-hide doesn't work
- Step-by-step fix procedure
- Detailed settings integration explanation
- ImGui API quick reference table
- Complete testing checklist

**Best for**: Understanding the system deeply

---

### 3. **ROCKETSTATS_INTEGRATION_PATTERNS.md** 🔗
**When to use**: You want to extend the overlay with RocketStats patterns

**Contains**:
- Overview of RocketStats 195-capability system
- Which RocketStats capabilities are relevant (caps 115-121)
- 5 RocketStats patterns explained:
  1. Variable substitution (template system)
  2. Subsystem organization
  3. Control surface types (12 types documented)
  4. Hook/event system
  5. 4-level aggregation
  6. CVar callback system
  7. JSON persistence
- How each pattern could adapt to SuiteSpot
- File alignment (SuiteSpot vs RocketStats analogs)
- Recommended integration phases:
  - Phase 1: Current implementation
  - Phase 2: RocketStats pattern adoption
  - Phase 3: Advanced features
- Current/future capability mapping

**Best for**: Architects planning feature extensions

---

### 4. **This file (README_OVERLAY_DOCS.md)** 🗺️
**Purpose**: Navigation and file index

---

## Quick Decision Tree

```
Do you want to...?

├─ Run the test overlay now?
│  └─ See: QUICK_REFERENCE.md → Test Button Command

├─ Fix a broken overlay?
│  ├─ Doesn't appear?
│  │  └─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Issue 1
│  ├─ Text is invisible?
│  │  └─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Issue 2
│  ├─ Wrong position?
│  │  └─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Issue 3
│  └─ Doesn't hide?
│     └─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Issue 4

├─ Understand how ImGui works?
│  └─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Architecture Overview + Key Concepts

├─ Customize overlay appearance?
│  ├─ Change colors?
│  │  └─ See: QUICK_REFERENCE.md → Common Customizations + Hue Reference
│  ├─ Resize overlay?
│  │  └─ See: QUICK_REFERENCE.md → Common Customizations
│  └─ Adjust animation timing?
│     └─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Fade Effects + Common Customizations

├─ Add new features to overlay?
│  ├─ Display modes (last match vs. session stats)?
│  │  └─ See: ROCKETSTATS_INTEGRATION_PATTERNS.md → 4-Level Aggregation
│  ├─ Custom data format?
│  │  └─ See: ROCKETSTATS_INTEGRATION_PATTERNS.md → Variable Substitution Pattern
│  ├─ Overlay themes?
│  │  └─ See: ROCKETSTATS_INTEGRATION_PATTERNS.md → Control Surface Types
│  └─ Save/export match data?
│     └─ See: ROCKETSTATS_INTEGRATION_PATTERNS.md → JSON Persistence

├─ Debug a specific issue?
│  └─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Common Issues & Fixes (with LOG statements)

└─ Understand the entire system architecture?
   ├─ See: IMGUI_OVERLAY_FIX_GUIDE.md → Complete Architecture Overview
   ├─ Then: ROCKETSTATS_INTEGRATION_PATTERNS.md → System Design Patterns
   └─ Finally: Code + QUICK_REFERENCE.md → Implementation Details
```

---

## File Locations & Related Code

### SuiteSpot Source Files

| File | Purpose | Key Lines |
|------|---------|-----------|
| `SuiteSpot.h` | Data structures | 37-62 (PostMatchInfo), 169-184 (PostMatchOverlayWindow) |
| `SuiteSpot.cpp` | Initialization & test button | 180-232 (window impl), 485-516 (ss_testoverlay) |
| `OverlayRenderer.h` | Renderer config | 1-138 (settings container) |
| `OverlayRenderer.cpp` | Drawing implementation | 1-217 (RenderPostMatchOverlay) |
| `SettingsUI.cpp` | Configuration UI | 560-843 (overlay settings) |
| `GuiBase.h` | Window framework | 1-30 (PluginWindowBase) |
| `GuiBase.cpp` | Framework implementation | 1-67 (Render lifecycle) |

### Documentation Files (This Suite)

| File | Size | Purpose |
|------|------|---------|
| `README_OVERLAY_DOCS.md` | This file | Navigation & index |
| `QUICK_REFERENCE.md` | ~2 KB | Quick lookup reference |
| `IMGUI_OVERLAY_FIX_GUIDE.md` | ~15 KB | Complete technical guide |
| `ROCKETSTATS_INTEGRATION_PATTERNS.md` | ~12 KB | Architecture patterns |
| `SuiteSpotDocuments/instructions/development/thread-safe-imgui.md` | ~2 KB | Critical thread safety rules |

### External Documentation

| Location | Type | Relevant Sections |
|----------|------|-------------------|
| `RocketStatsDocs/IMPLEMENTATION_MAPPING.md` | Reference | Caps 115-121 (Overlay rendering) |
| `RocketStatsDocs/Reference/115-*.md` | Specs | Text rendering pattern |
| `RocketStatsDocs/Reference/118-*.md` | Specs | Settings menu pattern |
| `RocketStatsDocs/Reference/120-*.md` | Specs | Color management pattern |

---

## Learning Path

### Beginner (New to SuiteSpot)

1. **Start here**: [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
   - ⏱️ 5 minutes
   - Understand: What is the overlay, how do I test it

2. **Learn basics**: [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Architecture Overview
   - ⏱️ 10 minutes
   - Understand: Component hierarchy and data flow

3. **Try it**: Run `ss_testoverlay` in BakkesMod console
   - ⏱️ 2 minutes
   - Verify: Overlay appears with mock data

4. **Customize**: [QUICK_REFERENCE.md](QUICK_REFERENCE.md) → Common Customizations
   - ⏱️ 5 minutes
   - Practice: Change colors, size, duration

**Total time**: ~25 minutes

---

### Intermediate (Want to Fix Issues)

1. **Prerequisites**: Complete Beginner path
   - ⏱️ 25 minutes

2. **Understand ImGui**: [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Key ImGui Concepts (sections 1-8)
   - ⏱️ 15 minutes
   - Learn: Context, window lifecycle, drawing APIs, colors, fonts, alpha blending

3. **Debug**: [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Common Issues & Fixes
   - ⏱️ 10 minutes
   - Practice: Identify and fix rendering issues

4. **Use tools**: [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Step-by-Step Fix Procedure
   - ⏱️ 10 minutes
   - Apply: Logging and verification techniques

**Total time**: ~60 minutes

---

### Advanced (Want to Extend/Architect)

1. **Prerequisites**: Complete Intermediate path
   - ⏱️ 60 minutes

2. **Study patterns**: [ROCKETSTATS_INTEGRATION_PATTERNS.md](ROCKETSTATS_INTEGRATION_PATTERNS.md)
   - ⏱️ 20 minutes
   - Learn: RocketStats subsystem organization, control surfaces, hooks, aggregation

3. **Plan features**: [ROCKETSTATS_INTEGRATION_PATTERNS.md](ROCKETSTATS_INTEGRATION_PATTERNS.md) → Recommended Integration Steps
   - ⏱️ 10 minutes
   - Design: Phase 2 & 3 feature roadmap

4. **Reference code**: All guides + `RocketStatsDocs/`
   - ⏱️ 30 minutes
   - Implement: New features following patterns

**Total time**: ~120 minutes

---

## Quick Troubleshooting

### "Overlay doesn't appear"
→ See [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Issue 1: Overlay Not Showing

### "Text is unreadable"
→ See [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Issue 2: Text Invisible

### "Overlay at wrong position"
→ See [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Issue 3: Position Wrong

### "Overlay doesn't auto-hide"
→ See [IMGUI_OVERLAY_FIX_GUIDE.md](IMGUI_OVERLAY_FIX_GUIDE.md) → Issue 4: No Auto-Hide

### "How do I add a new stat?"
→ See [ROCKETSTATS_INTEGRATION_PATTERNS.md](ROCKETSTATS_INTEGRATION_PATTERNS.md) → Phase 2-3

### "What's the color hue reference?"
→ See [QUICK_REFERENCE.md](QUICK_REFERENCE.md) → Hue Reference

---

## Architecture at a Glance

```
SuiteSpot Plugin
│
├─ PostMatchInfo (data structure)
│  ├─ active : bool
│  ├─ players[] : PostMatchPlayerRow
│  ├─ myScore, oppScore : int
│  ├─ myTeamName, oppTeamName : string
│  └─ start : steady_clock::time_point
│
├─ PostMatchOverlayWindow : PluginWindowBase
│  ├─ Render() : Called by BakkesMod every frame
│  │  ├─ SetNextWindowPos() → Position on screen
│  │  ├─ SetNextWindowSize() → Set dimensions
│  │  ├─ ImGui::Begin() → Create context
│  │  ├─ RenderWindow() → Delegate to renderer
│  │  └─ ImGui::End() → Finalize
│  │
│  └─ RenderWindow()
│     └─ Calls OverlayRenderer::RenderPostMatchOverlay()
│
├─ OverlayRenderer (rendering engine)
│  ├─ RenderPostMatchOverlay() : Main drawing function
│  │  ├─ Validate ImGui context
│  │  ├─ Get ImDrawList* (drawing surface)
│  │  ├─ Draw background rectangle
│  │  ├─ Draw title text
│  │  ├─ Draw match info
│  │  ├─ Draw column headers
│  │  ├─ Loop teams:
│  │  │  ├─ Draw team header
│  │  │  └─ Loop players:
│  │  │     └─ Draw player row
│  │  └─ Calculate fade (elapsed time)
│  │
│  └─ ResetDefaults() : Initialize to default values
│
└─ SettingsUI (configuration interface)
   └─ RenderPostMatchOverlaySettings()
      ├─ "Reset to Defaults" button
      ├─ Window Layout controls (width, height, offset)
      ├─ Team Sections controls (row heights, spacing)
      ├─ Column Positions controls (X positions)
      ├─ Text & Font controls (sizes)
      ├─ Team Colors controls (hue, saturation, value)
      └─ Effects controls (alpha, glow, fade duration)

ImGui Context (per-frame)
│
├─ Frame Setup:
│  ├─ ImGui::SetCurrentContext(plugin_->imguiCtx)
│  ├─ ImGui::GetIO().DisplaySize → Screen resolution
│  └─ ImGui::GetWindowDrawList() → Drawing surface
│
└─ Frame Render:
   ├─ AddRectFilled() → Rectangles
   ├─ AddText() → Text
   └─ GetColorU32() → Color conversion
```

---

## Key Takeaways

1. **ImGui Context**: Must be set before any ImGui operation
   - Stored in `plugin_->imguiCtx`
   - Set via `ImGui::SetCurrentContext()`

2. **Window Lifecycle**: Controlled by `isWindowOpen_` boolean
   - `Open()` sets to true
   - `Close()` sets to false
   - `Render()` checks and returns early if false

3. **Drawing API**: Uses low-level ImDrawList
   - Not traditional ImGui widgets
   - Direct screen-space coordinate drawing
   - Very fast and flexible

4. **Rendering Flow**: Every frame
   - PostMatchOverlayWindow::Render() → RenderWindow() → OverlayRenderer::RenderPostMatchOverlay()
   - Continuously recalculates elapsed time for fade

5. **Settings Integration**: CVar-backed configuration
   - All settings persisted via BakkesMod CVars
   - Real-time preview while overlay is open
   - "Reset to Defaults" syncs all values back

---

## Recommended Reading Order

**For understanding the system**:
1. This file (README_OVERLAY_DOCS.md)
2. QUICK_REFERENCE.md (high-level overview)
3. IMGUI_OVERLAY_FIX_GUIDE.md (detailed understanding)
4. ROCKETSTATS_INTEGRATION_PATTERNS.md (architectural patterns)
5. RocketStatsDocs/IMPLEMENTATION_MAPPING.md (system design reference)

**For fixing a problem**:
1. QUICK_REFERENCE.md (find your issue quickly)
2. IMGUI_OVERLAY_FIX_GUIDE.md → Common Issues (root causes & fixes)
3. Code (line numbers provided)
4. Add logging (DEBUG section in IMGUI guide)

**For extending features**:
1. ROCKETSTATS_INTEGRATION_PATTERNS.md (patterns to follow)
2. RocketStatsDocs/Reference/*.md (specific capability specs)
3. Code (existing implementation)
4. IMGUI_OVERLAY_FIX_GUIDE.md (when stuck on ImGui details)

---

## Support & References

### In-Code Comments
- `SuiteSpot.cpp:15-18` - SetImGuiContext explanation
- `SuiteSpot.cpp:20-41` - RenderSettings explanation
- `Source.cpp:5-50` - Detailed comments on ImGui context handling
- `OverlayRenderer.cpp:49-217` - Comprehensive RenderPostMatchOverlay

### External Documentation
- ImGui: https://github.com/ocornut/imgui
- BakkesMod SDK: Plugin wrapper for ImGui and game events
- RocketStatsDocs: `Reference/115-121-*.md` files

### Quick Commands
```bash
ss_testoverlay        # Toggle test overlay on/off
ss_overlay_duration 15  # Set display duration to 15 seconds
togglemenu SuiteSpot  # Open/close settings menu
```

---

## Document Versions

| Document | Version | Date | Author |
|----------|---------|------|--------|
| README_OVERLAY_DOCS.md | 1.0 | 2025-12-27 | Claude Code |
| QUICK_REFERENCE.md | 1.0 | 2025-12-27 | Claude Code |
| IMGUI_OVERLAY_FIX_GUIDE.md | 1.0 | 2025-12-27 | Claude Code |
| ROCKETSTATS_INTEGRATION_PATTERNS.md | 1.0 | 2025-12-27 | Claude Code |

---

## Next Steps

1. **Read one of the documentation files** above based on what you need
2. **Run** `ss_testoverlay` to see the overlay in action
3. **Experiment** with settings in the overlay configuration panel
4. **Debug** using logging techniques from IMGUI_OVERLAY_FIX_GUIDE.md
5. **Extend** using patterns from ROCKETSTATS_INTEGRATION_PATTERNS.md

---

**Happy coding! 🚀**

