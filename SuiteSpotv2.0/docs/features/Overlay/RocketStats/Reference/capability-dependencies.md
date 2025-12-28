# RocketStats Capability Dependencies & Relationships

## Overview

This document maps the interdependencies between the 195 capabilities in RocketStats, organized by functional domain and control flow.

---

## 1. INITIALIZATION DEPENDENCY CHAIN

### onLoad → onInit → All Systems Active

```
onLoad (plugin load entry point)
  ├─ Create WebSocket thread (Cap #138: websocket-server-init)
  ├─ Check config file exists
  └─ Schedule onInit() async
      │
      └─ onInit (async initialization)
          ├─ Load fonts (Ubuntu-Regular.ttf)
          ├─ Load rank images (Cap #158: load-rank-images)
          │   └─ Load casual image (Cap #159: load-casual-image)
          ├─ LoadThemes() (Cap #122: load-theme-config)
          │   ├─ Load theme fonts (Cap #123: load-theme-fonts)
          │   ├─ Load theme images (Cap #124: load-theme-images)
          │   └─ Parse element expressions (Cap #125: parse-element-expressions)
          ├─ InitRank() → Populate rank[] array
          ├─ InitStats() → Create empty stat containers
          ├─ ReadConfig() (Cap #132: load-config-json)
          │   ├─ Load CVars: rs_mode, rs_theme, rs_x, rs_y, rs_scale, etc.
          │   ├─ Restore per-playlist stats (Cap #134: persist-per-playlist-stats)
          │   ├─ Restore all-time stats (Cap #135: persist-all-time-stats)
          │   └─ Restore all-time per-gamemode stats (Cap #136: persist-all-time-gamemode-stats)
          ├─ ChangeTheme() (Cap #5 or #6)
          ├─ ResetFiles() (Cap #137: create-default-text-files)
          │   └─ Creates 50+ RocketStats_*.txt files
          └─ Register notifiers and CVars
              ├─ rs_toggle_menu (Cap #1: toggle-settings-overlay)
              ├─ rs_reset_stats (Cap #2: reset-all-statistics)
              ├─ rs_menu_pos (Cap #3: reposition-menu-overlay)
              ├─ All overlay CVars (Caps #4-66)
              └─ All file output CVars (Caps #26-46)
```

---

## 2. GAME EVENT FLOW & STAT TRACKING

### Match Lifecycle: Start → Events → End

#### **Match Start** (Cap #67: game-start-detection)

```
GameStart() triggered on Countdown.BeginState
  ├─ Extract current.playlist (Cap #28: toggle-file-output-gamemode)
  ├─ UpdateMMR() (Cap #113: mmr-update)
  ├─ Update current rank/division (Cap #114: track-current-rank-division)
  ├─ ResetBasicStats()
  │   └─ Clear session stats for new game
  ├─ WriteConfig() (Cap #131: save-config-json)
  ├─ UpdateFiles() (Cap #165: update-files-on-change)
  │   └─ Writes all enabled stat files
  ├─ SendGameState("GameStart") (Cap #142: websocket-game-start-notification)
  │   └─ Broadcast via WebSocket (Cap #141: websocket-broadcast-game-state)
  │       └─ Includes game context (Cap #149: websocket-game-context)
  └─ InGameTheme() (Cap #192: auto-theme-switch-on-menu)
      └─ Switch to rs_gameTheme if dualtheme enabled (Cap #191: dual-theme-support)
```

#### **During Match** (Caps #70-75: stat event tracking)

```
onStatEvent() per game event
  ├─ Detect primary player (Cap #172: detect-primary-player)
  ├─ Track stat by type:
  │   ├─ Cap #82: stat-clear → VarShotsClear()
  │   ├─ Cap #83: stat-assist → VarShotsAssist()
  │   ├─ Cap #84-87: other shot stats
  │   ├─ Cap #89-90: save stats
  │   ├─ Cap #91-100: goal stats
  │   ├─ Cap #101-102: dropshot damage stats
  │   ├─ Cap #103: stat-knockout
  │   ├─ Cap #104: stat-death
  │   └─ Cap #105: stat-demolition
  └─ For each stat, update in 4 aggregation levels:
      ├─ session stats
      ├─ stats[current.playlist]
      ├─ always stats
      └─ always_gm[current.playlist] stats
          └─ Incrementally update WriteFiles (Cap #130: write-stats-to-files)

onStatTickerMessage() for accolades
  ├─ Cap #106: stat-hat-trick
  ├─ Cap #107: stat-playmaker
  ├─ Cap #108: stat-savior
  └─ Cap #109: stat-accolade-mvp
      └─ Write via VarAccolades* functions
```

#### **Boost Tracking During Match** (Caps #73-75)

```
OnBoostStart() → Cap #73: boost-activation-tracking
  └─ SocketSend("Boost", true)

OnBoostChanged() → Cap #74: boost-amount-tracking
  ├─ Get boost amount
  ├─ Update current.boost_amount
  └─ SocketSend("Boost", amount)

OnBoostEnd() → Cap #75: boost-end-tracking
  └─ SocketSend("Boost", false)

All send via websocket (Cap #146: websocket-boost-update)
  └─ Update Cap #179: display-boost-amount in overlay
```

#### **Match End** (Cap #68: game-end-detection)

```
GameEnd() triggered on OnMatchWinnerSet
  ├─ Increment game counter (4 aggregation levels)
  ├─ Detect win vs loss
  │   ├─ If won:
  │   │   ├─ Cap #110: stat-win (increment)
  │   │   ├─ Cap #112: stat-streak (increment)
  │   │   └─ SocketSend("GameWon") (Cap #143: websocket-game-won-notification)
  │   └─ If lost:
  │       ├─ Cap #111: stat-loss (increment)
  │       ├─ Cap #112: stat-streak (decrement)
  │       └─ SocketSend("GameLost") (Cap #144: websocket-game-lost-notification)
  ├─ ComputeStreak() (internal streak logic)
  ├─ WriteConfig() (Cap #131: save-config-json) - persists wins/losses
  ├─ UpdateFiles() (Cap #165: update-files-on-change)
  ├─ SendGameState("GameEnd") (Cap #145: websocket-game-end-notification)
  ├─ SetRefresh() → triggers overlay render next frame
  └─ SetTimeout(3s) → UpdateMMR() delayed (Cap #167: mmr-update-delay)
      └─ Updates Cap #183: display-mmr-conditional-rounding

GameDestroyed() triggered on mid-match quit
  ├─ If game not already ended:
  │   ├─ Increment loss (Cap #111)
  │   ├─ Decrement streak (Cap #112)
  │   ├─ WriteConfig()
  │   └─ SocketSend("GameLost")
  ├─ UpdateFiles()
  ├─ SendGameState("GameDestroyed") (Cap #145: websocket-game-end-notification)
  └─ SetRefresh()
```

---

## 3. STATISTICS AGGREGATION & PERSISTENCE

### 4-Level Stats Architecture

```
Cap #4: select-stats-aggregation-mode (rs_mode CVar)
  ├─ rs_mode = 0 → session only (Cap #133: persist-session-stats)
  ├─ rs_mode = 1 → stats[current.playlist] (Cap #134: persist-per-playlist-stats)
  ├─ rs_mode = 2 → always (Cap #135: persist-all-time-stats)
  └─ rs_mode = 3 → always_gm[current.playlist] (Cap #136: persist-all-time-gamemode-stats)
      └─ GetStats() returns selected Stats object

Every stat increment affects multiple aggregation levels:
  ├─ session.Clear++; session.ClearCumul++; (Cap #160: track-cumulative-stats)
  ├─ stats[playlist].Clear++; stats[playlist].ClearCumul++;
  ├─ always.Clear++; always.ClearCumul++;
  └─ always_gm[playlist].Clear++; always_gm[playlist].ClearCumul++;

Team-aware tracking (Cap #161: track-team-stats):
  ├─ If isPrimaryPlayer(): increment player stat
  ├─ If teammate: increment Team* variant
  └─ Always increment Total* variant (Cap #162: track-total-stats)

Derived stats computed per match (Cap #163: track-match-specific-stats):
  ├─ ShootingPercentage = Goals / ShotOnGoal × 100
  └─ WinRatio (Cap #169) / WinPercentage (Cap #170)

WriteConfig() persists all aggregation levels:
  ├─ JSON structure: config["playlists"][playlist_id] = stats[playlist]
  ├─ config["always"] = always
  └─ config["always_gm"][playlist_id] = always_gm[playlist]
```

---

## 4. FILE OUTPUT SYSTEM

### Master Toggle & Individual Toggles

```
Cap #25: enable-file-output (rs_in_file CVar - MASTER TOGGLE)
  ├─ If disabled: NO text file writes occur
  └─ If enabled: check individual toggles
      │
      ├─ Cap #26-30: Basic stats (games, gm, rank, div, mmr, mmrc, mmrcc)
      │   └─ Var* functions called on change
      │       └─ WriteInFile("RocketStats_*.txt")
      │
      ├─ Cap #33-37: Win/loss/streak (win, loss, streak, ratio, percentage)
      │   └─ VarWin(), VarLoss(), VarStreak(), VarWinRatio(), VarWinPercentage()
      │
      ├─ Cap #38: toggle-file-output-score
      │   └─ VarScorePlayer() + VarScoreOpposite()
      │
      ├─ Cap #39-46: Stat category toggles
      │   ├─ rs_file_shots → AllShots() (Caps #82-87)
      │   ├─ rs_file_saves → AllSaves() (Caps #89-90)
      │   ├─ rs_file_goals → AllGoals() (Caps #91-100)
      │   ├─ rs_file_dropshot → AllDropshot() (Caps #101-102)
      │   ├─ rs_file_knockout → AllKnockout() (Cap #103)
      │   ├─ rs_file_miscs → AllMiscs() (Caps #104-108)
      │   ├─ rs_file_accolades → AllAccolades() (Cap #109)
      │   └─ rs_file_boost → VarBoost() (Cap #179)
      │
      └─ UpdateFiles() (Cap #165)
          └─ Writes ALL enabled files synchronously

Cap #137: create-default-text-files
  └─ ResetFiles() creates empty RocketStats_*.txt on init:
      ├─ RocketStats_Games.txt
      ├─ RocketStats_GameMode.txt
      ├─ RocketStats_Clear.txt
      ├─ RocketStats_Goal.txt
      ├─ ... (50+ total files)
      └─ All located in {BakkesMod}/data/RocketStats/

Cap #195: obs-text-file-export
  └─ OBS can read any RocketStats_*.txt as text source
      └─ Updates in real-time on every stat event
```

---

## 5. OVERLAY RENDERING PIPELINE

### Theme Loading → Variable Substitution → Rendering

```
LoadThemes() (Cap #122: load-theme-config)
  ├─ Scan RocketStats_themes/ folder
  ├─ For each theme folder:
  │   ├─ Read config.json (theme metadata)
  │   ├─ Cap #123: load-theme-fonts
  │   │   └─ Load .ttf files specified in fonts[] array
  │   ├─ Cap #124: load-theme-images
  │   │   └─ Load .tga images referenced in elements
  │   └─ Parse elements array
  │       ├─ Cap #125: parse-element-expressions
  │       │   └─ Use TinyExpr to evaluate width/height formulas
  │       └─ Store as Theme struct
  └─ Populate themes[] vector

ChangeTheme(idx) (Cap #5 or #6)
  ├─ Validate theme index
  ├─ Load theme_render = themes[idx]
  ├─ Apply per-theme overrides from themes_values JSON
  │   ├─ Cap #127: theme-position-override (x, y)
  │   ├─ Cap #128: theme-scale-override
  │   └─ Cap #129: theme-opacity-override
  └─ SetRefresh(RefreshFlags_RefreshAndImages)

Render() called every frame (WindowManagement.cpp)
  ├─ Check render conditions:
  │   ├─ Cap #12: toggle-overlay-display (rs_disp_overlay)
  │   ├─ Cap #13: show-overlay-in-menu (rs_enable_inmenu)
  │   ├─ Cap #14: show-overlay-ingame (rs_enable_ingame)
  │   └─ Cap #15: show-overlay-in-scoreboard (rs_enable_inscoreboard)
  ├─ Apply global transforms:
  │   ├─ Cap #7: set-overlay-x-position (rs_x)
  │   ├─ Cap #8: set-overlay-y-position (rs_y)
  │   ├─ Cap #9: set-overlay-scale (rs_scale)
  │   ├─ Cap #10: set-overlay-rotation-angle (rs_rotate)
  │   └─ Cap #11: set-overlay-opacity (rs_opacity)
  ├─ VarsReplace() (Cap #126: substitute-theme-variables)
  │   ├─ Call ReplaceOther() → populate {{Games}}, {{MMR}}, {{Rank}}, {{Div}}, etc.
  │   ├─ Call ReplaceShots() → {{Clear}}, {{Assist}}, {{Center}}, etc.
  │   ├─ Call ReplaceSaves() → {{Save}}, {{EpicSave}}, etc.
  │   ├─ Call ReplaceGoals() → {{Goal}}, {{LongGoal}}, {{AerialGoal}}, etc.
  │   ├─ Call ReplaceDropshot() → {{BreakoutDamage}}, etc.
  │   ├─ Call ReplaceKnockout() → {{KnockoutKO}}, etc.
  │   ├─ Call ReplaceMiscs() → {{Death}}, {{HatTrick}}, {{Playmaker}}, etc.
  │   └─ Call ReplaceAccolades() → {{MVP}}, {{FastestGoal}}, etc.
  ├─ For each Element in theme_render.elements:
  │   ├─ CalculateElement() (Cap #125: parse-element-expressions)
  │   │   └─ Evaluate TinyExpr expressions for position/size
  │   ├─ RenderElement() (Caps #115-121)
  │   │   ├─ Cap #115: render-overlay-text (if type="text")
  │   │   │   ├─ Substitute {{Variable}} → stat value
  │   │   │   ├─ Apply cap #174: apply-element-scale
  │   │   │   ├─ Apply cap #175: apply-text-alignment
  │   │   │   ├─ Apply cap #176: apply-text-valign
  │   │   │   └─ Cap #188: render-text-custom-font
  │   │   ├─ Cap #116: render-overlay-rectangle (if type="rectangle")
  │   │   │   ├─ Cap #120: apply-element-fill-color
  │   │   │   ├─ Cap #121: apply-element-stroke-color
  │   │   │   └─ Cap #189: render-rectangle-with-rounding
  │   │   └─ Cap #117: render-overlay-image (if type="image")
  │   ├─ Cap #119: apply-element-rotation-transform
  │   │   └─ ImGui::Rotate (imgui_rotate.h)
  │   ├─ Apply dynamic colors:
  │   │   ├─ Cap #185: apply-chameleon-color (based on stat value)
  │   │   └─ Cap #186: apply-sign-color (green for +, red for -)
  │   └─ Apply alpha:
  │       └─ Cap #190: apply-transparency-to-overlay (rs_opacity)
  ├─ RenderSettings() (Cap #118: render-settings-imgui-menu)
  │   └─ If settings_open, draw ImGui window with all CVars
  └─ UpdateUIScale() handles resolution changes (Cap #81: ui-scale-update)
```

---

## 6. CVar DEPENDENCY GRAPH

### Control Surface Dependencies

#### **Aggregation Mode (Cap #4)**
```
rs_mode CVar change
  └─ RefreshTheme() callback
      └─ SetRefresh() → next frame render uses GetStats() based on new mode
```

#### **Overlay Position/Size/Rotation (Caps #7-11)**
```
rs_x, rs_y, rs_scale, rs_rotate, rs_opacity CVars change
  └─ RefreshTheme() callback
      └─ SetRefresh() → recalculate overlay transform
          └─ Store per-theme override in themes_values[theme_name]
              └─ WriteConfig() on settings close (Cap #194: auto-save-on-settings-close)
```

#### **File Output Master Toggle (Cap #25)**
```
rs_in_file CVar change
  ├─ If true:
  │   └─ UpdateFiles(force=true) → write all enabled stat files
  └─ RefreshTheme() → update overlay if in-game
```

#### **Individual File Toggles (Caps #26-46)**
```
rs_file_* CVars change (each one)
  └─ Specific Var*() callback
      └─ WriteInFile("RocketStats_*.txt")
```

#### **Hide Toggles (Caps #47-66)**
```
rs_hide_* CVars change
  └─ Specific Var*() callback
      └─ WriteInFile() with rs_hide_* check
          └─ If hidden, write theme_hide_value ("##") instead of stat
```

#### **Theme Selection (Caps #5, #6)**
```
rs_theme CVar change (menu theme)
  └─ ChangeTheme(idx) → loads theme and applies overrides

rs_gameTheme CVar change (in-game theme)
  └─ ChangeTheme(idx) → loads theme and applies overrides
      └─ Only used if dualtheme enabled (Cap #191)
```

#### **MMR Display Options (Caps #16-24)**
```
rs_enable_float CVar → VarMMR() outputs float vs int
rs_replace_mmr* CVars → swap which variable displays in VarMMR()
rs_preview_rank CVar → VarRank() shows next rank if enabled
rs_roman_numbers CVar → VarDiv() formats division number
```

---

## 7. GAME STATE TRACKING & CONTEXT

### Game Context Flags (used everywhere)

```
is_in_game (Cap #180: detect-offline-game for offline, Cap #181: detect-offline-game)
is_in_menu (Cap #13-15: context checks for overlay visibility)
is_in_freeplay (Cap #180: detect-freeplay-mode)
is_in_scoreboard (Caps #79-80: scoreboard detection)
is_online_game (Cap #182: detect-online-ranked-game)
is_offline_game (Cap #181: detect-offline-game)
is_boosting (internal, used in OnBoost* functions)
is_game_started (gates stat tracking in onStatEvent)
is_game_ended (gates GameEnd vs GameDestroyed logic)

Hook Dependencies:
  ├─ Cap #76: main-menu-detection (is_in_MainMenu=true)
  │   └─ BacktoMenu() switches to menu theme (Cap #192)
  ├─ Cap #77-78: menu-stack-detection (is_in_menu tracking)
  │   └─ Used to hide overlay in menus (unless Cap #13 enabled)
  ├─ Cap #79-80: scoreboard detection
  │   └─ Used to hide overlay in scoreboard (unless Cap #15 enabled)
  └─ Cap #81: ui-scale-update
      └─ Recalculates overlay position on resolution/scale change
          └─ SetRefresh() → rerender with new scale

current struct (Cap #114: track-current-rank-division):
  ├─ current.playlist → determines which stats[] map entry
  ├─ current.rank, current.division → displayed in overlay
  ├─ current.ranked → affects Cap #183: display-mmr-conditional-rounding
  ├─ current.score_player, current.score_opposite
  │   └─ Caps #177-178: display-player/opponent-score
  └─ current.boost_amount
      └─ Cap #179: display-boost-amount
```

---

## 8. WEBSOCKET STREAMING PIPELINE

### Initialization → Client Management → Broadcasting

```
onLoad() spawns server_thread
  └─ InitWebSocket() (Cap #138: websocket-server-init)
      ├─ m_server.listen(8085)
      ├─ Set handlers:
      │   ├─ SocketOpen → SocketOpen() (Cap #139: websocket-client-connection)
      │   ├─ SocketClose → SocketClose() (Cap #140: websocket-client-disconnection)
      │   └─ SocketReceive → SocketReceive() (Cap #148: websocket-request-handler)
      └─ m_server.run() (blocking, in background thread)

SocketOpen() (Cap #139)
  ├─ m_connections.insert(hdl)
  └─ SendGameState("Initialization") (Cap #141: websocket-broadcast-game-state)
      └─ SocketBroadcast() → all connected clients

SocketReceive(msg) (Cap #148)
  └─ If msg == "request":
      └─ Send SocketData("GameState", GetGameState()) back to sender

SocketData() (Cap #149: websocket-game-context)
  ├─ Wraps stat data in JSON object:
  │   ├─ name, type, data fields
  │   └─ states object:
  │       ├─ IsInGame
  │       ├─ IsInMenu
  │       ├─ IsInFreeplay
  │       ├─ IsInScoreboard
  │       ├─ IsOnlineGame
  │       └─ IsOfflineGame
  └─ Broadcast to all clients

Game Event Broadcasting:
  ├─ GameStart() → SocketSend("GameStart") (Cap #142)
  ├─ GameWon → SocketSend("GameWon") (Cap #143)
  ├─ GameLost → SocketSend("GameLost") (Cap #144)
  ├─ GameEnd() → SocketSend("GameEnd") (Cap #145)
  ├─ OnBoostStart/Changed/End → SocketSend("Boost", ...) (Cap #146)
  ├─ onStatEvent() → SocketSend(...) (Cap #147: websocket-stat-event)
  └─ onStatTickerMessage() → SocketSend(...)

GetGameState() (Cap #141)
  └─ Returns JSON with:
      ├─ Ranked, Playlist, Rank, Div, MMR
      ├─ All stats from VarsWrite()
      └─ Team colors
```

---

## 9. CONFIGURATION PERSISTENCE

### Read/Write Cycle

```
ReadConfig() (Cap #132: load-config-json)
  ├─ Load from {BakkesMod}/data/rocketstats.json
  ├─ Deserialize CVars:
  │   ├─ rs_mode, rs_theme, rs_gameTheme
  │   ├─ rs_x, rs_y, rs_scale, rs_rotate, rs_opacity
  │   ├─ rs_in_file, all rs_file_* toggles
  │   └─ all rs_hide_* toggles
  ├─ Load themes_values (per-theme overrides for x/y/scale/opacity)
  ├─ VarsRead() all aggregation levels:
  │   ├─ For each playlist in config["playlists"]:
  │   │   └─ Load into stats[playlist_id]
  │   ├─ Load config["always"] → always
  │   └─ Load config["always_gm"][playlist_id] → always_gm[playlist_id]
  └─ Return true if successful

WriteConfig() (Cap #131: save-config-json)
  ├─ Serialize all CVars
  ├─ Serialize themes_values (Caps #127-129)
  ├─ VarsWrite() all aggregation levels:
  │   ├─ For each stats[playlist_id]:
  │   │   └─ Write to config["playlists"][playlist_id]
  │   ├─ Write always → config["always"]
  │   └─ Write always_gm[playlist_id] → config["always_gm"][playlist_id]
  └─ json.dump() pretty-printed to file

Trigger Points for WriteConfig():
  ├─ GameStart() (Cap #67) - begin of match
  ├─ GameEnd() (Cap #68) - end of match
  ├─ GameDestroyed() (Cap #69) - mid-match quit
  ├─ OnClose() of settings window (Cap #194)
  └─ onUnload() on plugin unload

Per-Playlist Persistence (Cap #134):
  └─ Each playlist has independent stats["playlist_id"] entry
      └─ e.g., stats[10] = Ranked Duel, stats[13] = Ranked Standard

All-Time Aggregation (Cap #135):
  └─ always keeps running total across all playlists
      └─ Independent of rs_mode selection

All-Time Per-Gamemode (Cap #136):
  └─ always_gm[playlist_id] = per-gamemode all-time
      └─ Separate from session/per-playlist split
```

---

## 10. LANGUAGE & LOCALIZATION

### Dynamic Language System (not CVar-based, hooks Rocket League's UI language)

```
onLoad() (Caps #150-151)
  ├─ Detect gameWrapper->GetUILanguage() (Cap #157: detect-ui-language)
  │   ├─ If "FRA" → ChangeLang(IDB_LANG_FRA) (Cap #151)
  │   └─ Else → ChangeLang(IDB_LANG_INT) (Cap #150)
  └─ Load language strings into rs_lang array

ChangeLang(lang_id)
  ├─ Load embedded resource (INT.json or FRA.json)
  │   └─ Caps #152-154: extract-*-resource
  └─ Populate rs_lang with translations

GetLang(StringId)
  └─ Return localized string for UI, CVar help text, etc.

CVar help text:
  ├─ rs_toggle_menu → GetLang(LANG_TOGGLE_MENU)
  ├─ rs_reset_stats → GetLang(LANG_RESET_STATS)
  └─ ... all CVars use GetLang() for localization

UI strings in RenderSettings():
  └─ All ImGui labels use GetLang() calls
```

---

## 11. RECOVERY & MIGRATION

### Legacy Version Support (v3 → v4)

```
onLoad() checks config existence
  ├─ If config/rocketstats.json exists:
  │   ├─ ReadConfig() (Cap #132)
  │   └─ rs_recovery = RecoveryFlags_Off
  └─ Else if RocketStats/ folder exists (v3 data):
      ├─ rs_recovery = RecoveryFlags_Files (Cap #156: migrate-old-config)
      └─ Schedule display of welcome screen
          └─ Caps #152-154: extract welcome resource

RecoveryOldVars() (Cap #156)
  ├─ Check for old RS_* CVars:
  │   ├─ RS_session → rs_mode
  │   ├─ RS_Use_v1/v2 → rs_theme
  │   ├─ RS_x_position → rs_x
  │   ├─ RS_scale → rs_scale
  │   └─ ... other v3 CVars
  └─ SetTheme() with migrated value
      └─ WriteConfig() (Cap #131) in new format

Recovery State Machine (Cap #193: recovery-mode)
  ├─ RecoveryFlags_Off (normal operation)
  ├─ RecoveryFlags_Files (migrating data)
  ├─ RecoveryFlags_Welcome (show welcome screen)
  ├─ RecoveryFlags_Process (executing migration)
  └─ RecoveryFlags_Finish (done)
```

---

## 12. TIMING & ASYNC OPERATIONS

### Timer Dependencies

```
Cap #166: 30fps-timer
  └─ FPSTimer(30fps)
      └─ Limits render frequency for performance

Cap #167: mmr-update-delay
  └─ GameEnd() → SetTimeout(3.0s) → UpdateMMR()
      └─ Allows Rocket League server to update rank before reading
          └─ Updates current.rank, current.division, current.myMMR

Cap #168: websocket-background-thread
  └─ server_thread spawned in onLoad()
      └─ Runs InitWebSocket() in separate thread
          ├─ m_server.run() blocks in loop
          └─ Doesn't block main game thread

Settings Window Timer:
  └─ rs_menu_pos notifier
      ├─ ToggleSettings(hide) immediately
      └─ SetTimeout(0.2s) → ToggleSettings(show) with overlay_move=true
          └─ Allows overlay to be positioned while settings hidden
```

---

## 13. DEPENDENCY CHAINS BY USE CASE

### Use Case: "User changes MMR display format"

```
User sets rs_replace_mmr CVar
  ├─ CVarWrapper callback → RefreshFiles()
  ├─ VarMMR(write=true) called
  │   └─ Checks rs_replace_mmr to decide which field to output
  ├─ WriteInFile("RocketStats_MMR.txt")
  └─ Next frame Render() calls VarsReplace()
      └─ {{MMR}} variable updated in theme
          └─ RenderElement() redraw

Dependencies:
  ├─ CVar system (rs_replace_mmr exists)
  ├─ File I/O (rs_in_file must be true)
  ├─ Overlay rendering (re-rendered next frame)
  └─ Config persistence (saved on settings close)
```

### Use Case: "User switches playlist mid-session"

```
GameStart() on new playlist
  ├─ current.playlist = new value
  ├─ GetStats() now returns stats[new_playlist]
  │   └─ Different from previous playlist
  ├─ UpdateFiles() writes from new playlist's stats
  ├─ SendGameState() broadcasts new stats via WebSocket
  └─ Render() shows new playlist's stats in overlay

Dependencies:
  ├─ Game event hook (GameStart detects playlist change)
  ├─ Stat aggregation (stats[new_playlist] previously populated)
  ├─ File output (WriteInFile uses GetStats() which changed)
  └─ Overlay rendering (VarsReplace gets new stat values)
```

### Use Case: "User enables file output for first time"

```
User sets rs_in_file = true
  ├─ CVarWrapper callback
  ├─ UpdateFiles(force=true) called
  │   └─ For each enabled rs_file_* toggle:
  │       └─ Call corresponding Var*() → WriteInFile()
  ├─ RefreshTheme() → render update
  └─ Config persisted on settings close

Dependencies:
  ├─ File system (files created by ResetFiles() in onInit)
  ├─ Stat tracking (all stats up to date)
  ├─ Individual toggles (rs_file_games, rs_file_shots, etc.)
  └─ Config persistence (rs_in_file saved)
```

### Use Case: "Crash mid-match recovery"

```
Plugin reloaded while is_game_started = true
  └─ onLoad() → onInit() → ReadConfig()
      ├─ Load persisted stats from previous sessions
      ├─ Load current.playlist (empty/old value)
      └─ Don't restore session (new session starts on next GameStart)

Next GameStart() event
  ├─ ResetBasicStats() clears session
  ├─ UpdateFiles() writes fresh stats
  └─ All 4 aggregation levels already persisted

Dependencies:
  ├─ JSON persistence (all-time stats survive crash)
  ├─ Game event timing (session resets on next GameStart, not on load)
  └─ Stat recovery logic (session vs persistent separation)
```

---

## 14. CRITICAL DEPENDENCY PATHS

### Stat Change → Visible Update (Complete Chain)

```
1. onStatEvent() hook fired
   ├─ isPrimaryPlayer() check
   └─ Increment stats[playlist].Clear (and 3 other aggregation levels)

2. UpdateFiles() called
   ├─ rs_in_file check (master toggle)
   ├─ rs_file_shots check (category toggle)
   └─ VarShotsClear() called
       └─ WriteInFile("RocketStats_Clear.txt")

3. SocketSend() called
   ├─ SocketBroadcast() to all WebSocket clients
   └─ Client receives stat update

4. Next frame Render()
   ├─ VarsReplace() calls ReplaceShots()
   │   └─ {{Clear}} = VarShotsClear() reads from GetStats()
   ├─ theme_vars["Clear"] = "42" (new value)
   ├─ For each Element in theme_render.elements:
   │   └─ Element.value = "Clears: {{Clear}}" → "Clears: 42"
   └─ RenderElement() draws updated text

5. WriteConfig() on game end
   ├─ VarsWrite() serializes updated stats
   └─ JSON saved to disk

Critical dependency: is_game_started flag
  └─ If false, onStatEvent() exits early (no stats tracked)
```

### Overlay Position Change → Persisted

```
1. User drags overlay in rs_menu_pos mode
   ├─ ImGui detects mouse drag
   └─ rs_x and rs_y CVars updated

2. CVarWrapper callback triggered
   ├─ RefreshTheme() called
   └─ SetRefresh() queues next frame render

3. Next frame Render()
   ├─ New rs_x, rs_y applied to transform
   └─ Overlay renders at new position

4. Settings window closed
   ├─ cl_rocketstats_settings CVar callback
   └─ WriteConfig() called
       ├─ themes_values[theme_name]["x"] = rs_x
       ├─ themes_values[theme_name]["y"] = rs_y
       └─ Saved to JSON

5. Plugin reloaded
   ├─ ReadConfig() loads themes_values
   └─ SetCVar() restores rs_x and rs_y

Critical dependency: Per-theme override storage
  └─ Different themes can have different positions
```

---

## 15. CONTROL FLOW ISOLATION

### Independent Subsystems (can fail independently)

```
Subsystem: WebSocket Streaming (Caps #138-149)
  ├─ Runs in background thread
  ├─ Failure: clients don't receive updates
  └─ Does NOT affect:
      ├─ Stat tracking (onStatEvent still increments stats)
      ├─ File output (WriteInFile still works)
      └─ Overlay rendering (Render still shows stats)

Subsystem: File Output (Caps #25-46, #130, #137, #195)
  ├─ Writes to text files synchronously
  ├─ Failure: OBS doesn't see updates
  └─ Does NOT affect:
      ├─ Overlay rendering (overlay shows stats in-game)
      ├─ WebSocket streaming (clients get updates)
      └─ Config persistence (stats still saved)

Subsystem: Overlay Rendering (Caps #5-24, #115-190)
  ├─ ImGui-based UI drawing
  ├─ Failure: overlay invisible
  └─ Does NOT affect:
      ├─ Stat tracking (stats still increment)
      ├─ File output (files still written)
      └─ WebSocket streaming (clients still updated)

Subsystem: Configuration Persistence (Caps #131-136, #156)
  ├─ JSON serialization/deserialization
  ├─ Failure: stats lost on plugin unload
  └─ Does NOT affect:
      ├─ Active session stats (in-memory still work)
      ├─ Overlay rendering (current session visible)
      └─ File output (current session files written)
```

---

## 16. SUMMARY: Dependency Topology

### Initialization Order
```
onLoad
  ├─ WebSocket thread start (background, non-blocking)
  └─ Schedule onInit()
      ├─ Load images, themes, fonts
      ├─ ReadConfig()
      ├─ Register CVars and notifiers
      ├─ Register game hooks
      └─ Hook callbacks ready for events
```

### Event-Driven Execution
```
Game Event (Hook) → Stat Update → File Write + WebSocket Send + Config Save
  ├─ Stat change propagates through 4 aggregation levels
  ├─ File output independent (can be disabled)
  ├─ WebSocket broadcast independent (clients optional)
  └─ Config save deferred to game end or settings close
```

### Rendering Loop
```
Every Frame → Check visibility conditions → Substitute variables → Render elements
  ├─ No strict dependency on stat events (uses GetStats() for current value)
  ├─ Theme selection affects which elements render
  ├─ Position/scale/opacity CVars applied each frame
  └─ Refresh flags optimize when recalculation needed
```

### Critical Bottleneck: stat.games Counter
```
All 4 aggregation levels depend on this being incremented correctly:
  ├─ WinRatio calculation depends on it
  ├─ WinPercentage calculation depends on it
  ├─ Per-playlist grouping depends on matching playlist ID at time of event
  └─ Persistence depends on all increments committed before WriteConfig()
```

---

## 17. DATA FLOW DIAGRAM

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        INITIALIZATION                                    │
└─────────────────────────────────────────────────────────────────────────┘
              onLoad → onInit → ReadConfig → LoadThemes
                         ↓
                   RegisterCVars & Hooks
                         ↓
            ┌────────────────────────────┐
            │   Ready for Game Events    │
            └────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                      GAME EVENT HANDLING                                 │
└─────────────────────────────────────────────────────────────────────────┘
GameStart Hook
    ↓
ExtractPlaylist → UpdateMMR → ResetSession → UpdateFiles → SendGameState
    ↓
InGameTheme

onStatEvent Hook (per-tick during match)
    ↓
CheckPrimaryPlayer → IncrementStats(4 levels) → UpdateFiles → SocketSend
    ↓
               ┌─────────────────────┐
               │  Overlay Renders    │
               │  (uses GetStats())   │
               └─────────────────────┘

GameEnd Hook
    ↓
IncrementWin/Loss → ComputeStreak → WriteConfig → UpdateFiles → SendGameState
    ↓
        SetTimeout(3s) → UpdateMMR

┌─────────────────────────────────────────────────────────────────────────┐
│                      FRAME RENDERING                                     │
└─────────────────────────────────────────────────────────────────────────┘
Render() called every frame
    ↓
CheckVisibilityConditions (in game? in menu? overlay enabled?)
    ↓
VarsReplace() (substitute {{Variable}} with stat values)
    ↓
ForEach Element in Theme
    ├─ CalculatePosition/Size (TinyExpr)
    ├─ ApplyTransforms (x, y, scale, rotate, opacity)
    ├─ RenderElement (text, rectangle, or image)
    └─ ApplyColors (chameleon, sign-based)
    ↓
RenderSettings (if window open)

┌─────────────────────────────────────────────────────────────────────────┐
│                      PERSISTENCE                                         │
└─────────────────────────────────────────────────────────────────────────┘
CVar Change
    ↓
    ├─ Immediate: RefreshTheme() or RefreshFiles()
    └─ Deferred: WriteConfig() on settings close or game end
    ↓
ReadConfig() on next plugin load
    ↓
AllDataRestored (stats, CVars, theme overrides)

WebSocket (parallel, independent)
    ├─ Client connects → SendGameState("Initialization")
    ├─ Client sends "request" → SocketData response
    └─ Game events → SocketBroadcast to all clients
```

---

## Conclusion

**195 capabilities** organized into **17 subsystems** with explicit dependencies:

- **Initialization chain** must complete before any events
- **Game events** flow through stat aggregation → file output & WebSocket → overlay rendering
- **Subsystems are loosely coupled** (WebSocket failure ≠ stat tracking failure)
- **Critical paths** are: stat increment → 4-level aggregation → file write → persistence
- **CVars drive** overlay positioning, visibility, file output, and stats aggregation mode
- **Render loop** is independent of events (uses GetStats() read-only)
- **Persistence is deferred** (WriteConfig on game end or settings close, not per-event)
