# SuiteSpot Architecture

Complete architecture documentation for understanding how SuiteSpot is organized and how components interact.

## Overview

SuiteSpot is a BakkesMod plugin that provides automated map loading and queuing functionality for Rocket League. It supports three map types: Freeplay, Training Packs, and Workshop maps.

## Project Structure

```
SuiteSpotv2.0/
├── SuiteSpot.h                 # Main plugin class header
├── SuiteSpot.cpp               # Core plugin implementation
├── Source.cpp                  # Settings UI rendering
├── MapList.h/cpp               # Map data structures and global vectors
├── GuiBase.h/cpp               # Base settings window class
├── LoadoutManager.h/cpp        # Car loadout management
├── pch.h/cpp                   # Precompiled headers
├── logging.h                   # Logging utilities
├── version.h                   # Version macros (auto-updated)
├── resource.h                  # Windows resource definitions
├── SuiteSpot.rc                # Windows resource script
├── BakkesMod.props             # SDK path configuration (registry-based)
├── SuiteSpot.sln               # Visual Studio solution
├── SuiteSpot.vcxproj           # Visual Studio project file
├── IMGUI/                      # ImGui rendering library
├── plugins/                    # Output folder for compiled DLL
├── build/                      # Build intermediates
├── SuiteSpotDocuments/         # AI agent documentation
└── Scripts:
    ├── update_version.ps1      # Pre-build version updater
    ├── generate_settings.ps1   # Post-build settings generator
    └── scrape_prejump.ps1      # Training pack scraper
```

## Core Components

### 1. Main Plugin Class (SuiteSpot)

**File:** `SuiteSpot.h` / `SuiteSpot.cpp`

**Inherits from:**
- `BakkesMod::Plugin::BakkesModPlugin` - SDK base class
- `SettingsWindowBase` - Settings UI interface (F2 menu in BakkesMod)

**NOTE:** SuiteSpot does NOT inherit from `PluginWindowBase`. All UI is rendered in the BakkesMod settings window only.

**Key Responsibilities:**
- Plugin lifecycle management (`onLoad()`, `onUnload()`)
- Map persistence (loading/saving training maps)
- Settings UI rendering
- Event hook registration
- Auto-loading and auto-queueing logic

**State Variables:**
- `enabled` - Toggle plugin on/off
- `mapType` - 0=Freeplay, 1=Training, 2=Workshop
- `autoQueue` - Auto-queue after map load
- `currentIndex`, `currentTrainingIndex`, `currentWorkshopIndex` - Map selection
- `delayQueueSec`, `delayFreeplaySec`, `delayTrainingSec`, `delayWorkshopSec` - Delays
- `trainingShuffleEnabled` - Shuffle training maps
- `trainingShuffleBag` - Selected maps for shuffle

### 2. Map Data (MapList)

**File:** `MapList.h` / `MapList.cpp`

**Global Vectors:**
```cpp
extern std::vector<MapEntry> RLMaps;           // Freeplay maps (53 total)
extern std::vector<TrainingEntry> RLTraining;  // Custom training packs
extern std::vector<WorkshopEntry> RLWorkshop;  // Workshop maps
```

**Structures:**
```cpp
struct MapEntry {
    std::string code;  // Load command code (e.g., "Stadium_P")
    std::string name;  // Display name
};

struct TrainingEntry {
    std::string code;  // Pack code (e.g., "555F-7503-BBB9-E1E3")
    std::string name;  // Display name
    // Plus metadata for Prejump integration
};

struct WorkshopEntry {
    std::string filePath;  // Full path to map file
    std::string name;      // Display name
};
```

### 3. Settings UI (Source.cpp)

**File:** `Source.cpp`

**Function:** `RenderSettings()` - ImGui-based settings UI

**Sections:**
- Enable/Disable toggle with status display
- Map mode selection (radio buttons)
- Auto-Queue configuration
- Map-specific selection and load buttons
- Type-specific settings (delays, shuffle options)
- **Prejump Packs Tab** - Training pack browser with filtering, sorting, and shuffle integration

**Prejump Packs Tab Implementation:**
- **IMPORTANT:** Uses `ImGui::Columns()` directly WITHOUT `BeginChild` wrapper
- Columns fill full tab width automatically (no child window needed)
- 8 columns: Name, Creator, Difficulty, Shots, Tags, Likes, Plays, Actions
- Sortable headers with visual indicators (▲▼) using `SortableColumnHeader()` helper
- Tag filter dropdown (dynamically populated from loaded packs)
- Difficulty filter and minimum shots slider
- Load Now button - direct pack loading via `load_training` command
- +Shuffle button - adds/removes packs from shuffle bag (green highlight when selected)
- Shuffle bag status display with clear button
- All filtering/sorting cached for performance with 2000+ packs

### 4. Loadout Management (LoadoutManager)

**File:** `LoadoutManager.h` / `LoadoutManager.cpp`

**Responsibilities:**
- Car loadout management and customization
- Integration with BakkesMod's loadout system

### 5. Persistence Layer

**Methods:**
- `LoadTrainingMaps()` / `SaveTrainingMaps()` - CSV file format
- `LoadWorkshopMaps()` / `DiscoverWorkshopInDir()` - Directory scanning
- `LoadShuffleBag()` / `SaveShuffleBag()` - Shuffle state persistence

**Storage Locations:**
- Training: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\SuiteSpotTrainingMaps.txt`
- Shuffle Bag: `%APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\SuiteShuffleBag.txt`

### 6. Prejump Integration

**Methods:**
- `ScrapeAndLoadPrejumpPacks()` - Run PowerShell scraper
- `LoadPrejumpPacksFromFile()` - Load from cache
- `IsPrejumpCacheStale()` - Check if 7+ days old
- `FormatLastUpdatedTime()` - Display cache timestamp

**File:** `scrape_prejump.ps1` - PowerShell script that generates JSON cache
**Note:** Script located in project root directory

## Plugin Lifecycle

### onLoad() - Plugin Startup
1. Load training maps from disk
2. Load workshop maps from directories
3. Load shuffle bag state
4. Register CVars (console variables)
5. Register event hooks (event listeners)
6. Load cached Prejump data
7. Initialize LoadoutManager and other components

### onUnload() - Plugin Shutdown
- Automatic cleanup by BakkesMod
- CVars and notifiers unregistered
- Event hooks removed

## Event Hooks

**Match End Events:**
```cpp
"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded"
"Function TAGame.AchievementManager_TA.HandleMatchEnded"
```

Triggers auto-load and auto-queue logic.

**Canvas Drawable:**
- Registered to render training pack browser overlay
- Called each frame if drawer is active

## Control Flow: Auto-Load After Match

1. Player finishes match
2. `GameEndedEvent()` is triggered
3. Plugin checks if enabled
4. Plugin dispatches based on `mapType`:
   - **Freeplay:** Execute `load_freeplay <code>`
   - **Training:** Execute `load_training <code>` (optionally from shuffle)
   - **Workshop:** Execute `load_workshop "<filepath>"`
5. If auto-queue enabled, execute `queue` command after delay
6. Commands executed with configurable delays via `SetTimeout()`

## CVars (Console Variables)

**Enable/Mode:**
- `suitespot_enabled` - Enable plugin (0/1)
- `suitespot_map_type` - Map type (0=Freeplay, 1=Training, 2=Workshop)

**Auto-Queue:**
- `suitespot_auto_queue` - Enable auto-queue (0/1)
- `suitespot_delay_queue_sec` - Queue delay (0-300s)

**Delays:**
- `suitespot_delay_freeplay_sec` - Freeplay load delay
- `suitespot_delay_training_sec` - Training load delay
- `suitespot_delay_workshop_sec` - Workshop load delay

**Selection:**
- `suitespot_current_freeplay_index` - Selected freeplay map index
- `suitespot_current_training_index` - Selected training pack index
- `suitespot_current_workshop_index` - Selected workshop map index

**Shuffle:**
- `suitespot_training_shuffle` - Enable shuffle (0/1)
- `suitespot_training_bag_size` - Shuffle bag size (1, 5, 10, 15)

## Build Configuration

**Target:** Release|x64
**Output:** `plugins\SuiteSpot.dll`
**Platform Toolset:** v143 (Visual Studio 2022)
**SDK Version:** BAKKESMOD_PLUGIN_API_VERSION 95

## Design Patterns Used

1. **Singleton Pattern** - Plugin class instantiation via `BAKKESMOD_PLUGIN` macro
2. **PIMPL Pattern** - SDK wrappers hide implementation details
3. **Observer Pattern** - Event hooks for game state changes
4. **Settings Persistence** - CVars and file-based storage
5. **CSV Format** - Human-editable map lists
6. **Delayed Execution** - `SetTimeout()` for asynchronous commands

## Common Development Tasks

### Adding a New CVar
```cpp
cvarManager->registerCvar("suitespot_new_var", "0", "Description", 
    true, true, 0, true, 100)
    .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        // Update member variable
    });
```

### Adding a New Event Hook
```cpp
gameWrapper->HookEvent("Function Path.To.Event", 
    bind(&SuiteSpot::OnEventHandler, this, placeholders::_1));
```

### Delayed Command Execution
```cpp
gameWrapper->SetTimeout([this, cmd](GameWrapper* gw) {
    cvarManager->executeCommand(cmd);
}, delaySec);
```

## Known Issues / Notes

- Prejump scraper requires PowerShell and network access
- Workshop maps require Steam/Epic installation with mod folders
- Training shuffle uses `std::mt19937` for random selection
- Global map vectors should be accessed carefully in multi-threaded contexts
- BakkesMod SDK path is resolved via Windows registry (see BakkesMod.props)

## Testing

To test the plugin:
1. Build the solution (Release|x64)
2. DLL is automatically copied to BakkesMod plugins folder
3. In Rocket League, open console (Ctrl + F6)
4. Type: `plugin load suitespot`
5. Open settings menu to configure
6. Play a match and test auto-load/queue

To unload:
```
plugin unload suitespot
```

## Build System

**Build Configuration:**
- Project uses `BakkesMod.props` property sheet to configure SDK paths
- SDK path resolved from registry: `HKEY_CURRENT_USER\Software\BakkesMod\AppPath`
- No local SDK copy required - references installed BakkesMod SDK
- Pre-build: `update_version.ps1` updates version numbers
- Post-build: `generate_settings.ps1` generates settings files
- Post-build: BakkesMod patcher (`bakkesmod-patch.exe`) patches DLL for hot-reload

## Future Enhancements

- Phase 3: Training Pack Manager with Prejump UI integration
- Real-time shot progress tracking in training
- Custom map grouping and playlists
- Scheduled map rotations
- Integration with replay system
