# SuiteSpot Feature Roadmap

A comprehensive tracking file for planned features, UI improvements, and future enhancements.

**Vision:** Transform SuiteSpot into an all-in-one Rocket League companion plugin.

---

## Part 1: UI Organization Tasks

### High Priority - Code Quality

- [ ] **Split RenderMapSelectionTab()** *(SettingsUI.cpp:212-585)*
  - Extract `RenderFreeplayMode()` (~60 lines)
  - Extract `RenderTrainingMode()` (~170 lines)
  - Extract `RenderWorkshopMode()` (~140 lines)
  - Benefit: Each mode independently maintainable

- [ ] **Create UI Helper Functions** *(New: UIHelpers.h/cpp)*
  - `InputIntWithRange()` - input with clamping, cvar, tooltip
  - `ComboWithTooltip()` - dropdown with tooltip
  - `StatusMessage()` - auto-fade status display
  - Benefit: Reduce 20+ repeated patterns to single-line calls

- [ ] **Centralize Configuration Constants** *(New: UIConstants.h)*
  - Widget widths (220, 300, etc.)
  - Delay ranges (0-300)
  - Difficulty levels array
  - Map mode labels

### Medium Priority - Consistency

- [ ] **Standardize Status Message System**
  - Create `StatusMessage` class with auto-fade
  - Replace 3 different timer implementations

- [ ] **Add ListClipper to SettingsUI Dropdowns**
  - Apply pattern from TrainingPackUI
  - Better performance with 500+ training packs

- [ ] **Extract Form Components**
  - Workshop path configuration section
  - Shuffle bag manager section

### Nice-to-Have

- [ ] **Validation Framework**
  - Shared `ValidatePackCode()`, `ValidatePath()` functions

- [ ] **Consistent State Management**
  - Choose "read all at start" vs "read on-demand" pattern

---

## Part 2: Future Features

### Session Statistics & Tracking
*SDK: PriWrapper::GetMatchGoals(), GetMatchSaves(), GetMatchAssists(), etc.*

- [ ] **Session Stats Tracker**
  - Track goals, assists, saves, demos, shots per session
  - Win/loss streak counter
  - MMR gain/loss display (via MMRWrapper)
  - Session duration tracking
  - *Reference: RocketStats (1.83M downloads)*

- [ ] **Match History Logger**
  - Save match stats to CSV/JSON
  - Export for analysis
  - *Reference: RLCSVPlugin*

- [ ] **Personal Bests Tracker**
  - Fastest goal speed
  - Longest shot distance
  - Best session streaks

### Quality of Life Features
*SDK: GameWrapper::IsInGame(), IsInFreeplay(), Toast()*

- [ ] **Enhanced Auto-Queue** *(expand existing)*
  - Conditional auto-queue (only after wins/losses)
  - Playlist-specific settings
  - "Take a break" reminder after X games

- [ ] **Quick Actions Menu**
  - Hotkey-triggered radial menu
  - Quick loadout switch
  - Quick map change
  - Quick playlist select

- [ ] **Game State Notifications**
  - Toast notifications for game events
  - "Match found" alert when alt-tabbed
  - "Back to menu" after AFK timeout

- [ ] **FPS Cap Manager**
  - Menu FPS cap (save GPU when idle)
  - In-game FPS cap

### Car & Loadout Features
*SDK: CarWrapper, GetItemsWrapper(), GetUserLoadoutSave()*

- [ ] **Smart Loadout Switching**
  - Auto-switch loadout by playlist (casual vs ranked)
  - Auto-switch by map
  - Random loadout option

- [ ] **Loadout Presets**
  - Save/load complete loadout presets
  - Quick swap between favorites
  - JSON export for backup

- [ ] **Car Customization Viewer**
  - Preview items on car
  - *Reference: AlphaConsole features*

### Training Enhancements
*SDK: ServerWrapper::SpawnBall(), GenerateShot(), GetGoalLocation()*

- [ ] **Training Pack Analytics**
  - Success rate per shot
  - Time spent on each shot
  - Progress tracking over time

- [ ] **Smart Training Suggestions**
  - Recommend packs based on weak areas
  - Track which packs you've completed

- [ ] **Custom Shot Generator**
  - Generate random shots to goal
  - Ball prediction line
  - *Reference: Predictdator (474k downloads)*

- [ ] **Freeplay Enhancements**
  - Ball speed display
  - Shot power indicator
  - Reset ball position hotkey

### Display & Overlay Features
*SDK: RegisterDrawable(), ImGui, GetScreenSize()*

- [ ] **In-Game HUD Elements**
  - Boost amount (larger/custom position)
  - Ball speed indicator
  - Game clock (alternative position)

- [ ] **Scoreboard Enhancements**
  - Show MMR next to names
  - Show platform icons
  - *Reference: Rank Viewer (2.26M downloads)*

- [ ] **OBS Integration**
  - Export stats for streaming overlays
  - Session info display
  - *Reference: RocketStats OBS mode*

### Sound & Notification Features
*SDK: Event hooks for goals, saves, demos*

- [ ] **Custom Sound Events**
  - Goal scored sound
  - Demo sound
  - Save sound
  - *Reference: Crossbar Sound Plugin (2.2M downloads)*

- [ ] **Match Alerts**
  - Queue pop notification sound
  - Match end chime
  - Overtime warning

### Replay & Content Creation
*SDK: ReplayManagerWrapper, PlayReplay()*

- [ ] **Replay Auto-Save**
  - Auto-save replays for close games
  - Auto-save highlight moments

- [ ] **Quick Clip Marker**
  - Mark moments during gameplay
  - Easy export timestamps

### Social & Party Features
*SDK: GetPartyLeaderID(), GetClubID(), GetClubDetails()*

- [ ] **Party Management**
  - Quick party invite
  - Party stats summary
  - Club info display

- [ ] **Recent Players**
  - Track recent teammates
  - Note system for players

---

## Implementation Priority Matrix

| Feature | User Value | SDK Support | Complexity | Priority |
|---------|-----------|-------------|------------|----------|
| Session Stats Tracker | High | Full | Medium | P1 |
| Split UI Functions | Medium | N/A | Low | P1 |
| Smart Loadout Switch | High | Full | Medium | P1 |
| UI Helpers | Medium | N/A | Low | P1 |
| Training Analytics | High | Full | High | P2 |
| Custom Sounds | Medium | Events | Medium | P2 |
| In-Game HUD | Medium | ImGui | Medium | P2 |
| Replay Features | Low | Partial | High | P3 |

---

## Completed Features

- [x] **Auto-Load System** - Automatically loads maps after matches
- [x] **Auto-Queue** - Queues for next match after loading
- [x] **Shuffle Bag Algorithm** - No map repeats until all played
- [x] **Training Pack Management** - Browse, filter, sort packs from prejump.com
- [x] **Loadout Switching** - Switch between saved car loadouts
- [x] **Workshop Map Discovery** - Scans workshop directories

---

## Research Sources

- [BakkesPlugins Repository](https://bakkesplugins.com/plugins)
- [Best BakkesMod Plugins - esports.gg](https://esports.gg/guides/rocket-league/the-best-bakkesmod-plugins-to-be-better-at-rocket-league/)
- [Best BakkesMod Plugins - dotesports](https://dotesports.com/rocket-league/news/best-bakkesmod-plugins-for-rocket-league)
- [BakkesMod SDK - GitHub](https://github.com/bakkesmodorg/BakkesModSDK)
- [RocketStats Plugin](https://github.com/Lyliya/RocketStats)
- [BMSessionPlugin](https://github.com/MeineZ/BMSessionPlugin)
- Local SDK: `%APPDATA%\bakkesmod\bakkesmod\bakkesmodsdk`

---

**Last Updated:** 2025-12-31
