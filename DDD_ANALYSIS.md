# SuiteSpot BakkesMod Plugin: Domain-Driven Design Analysis

**Version:** 1.0
**Date:** 2025-01-31
**Status:** Strategic Architecture Document

---

## Executive Summary

SuiteSpot is organized around a **Hub-and-Spoke pattern** in its current implementation, with `SuiteSpot.cpp` as the central orchestrator. However, from a DDD perspective, the codebase exhibits characteristics of **four distinct bounded contexts** that should be modeled more explicitly:

1. **Training Pack Management** - Managing the library of training packs
2. **Map & Venue Discovery** - Finding and cataloging available maps
3. **Automation & Orchestration** - Coordinating post-match actions
4. **Settings & Configuration** - Managing user preferences
5. **Loadout Management** - Switching car presets

This document provides a strategic blueprint for evolving the architecture using Domain-Driven Design principles.

---

## 1. BOUNDED CONTEXTS

### 1.1 Core Domains

#### **Domain: Training Pack Management**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Discover, catalog, search, and manage 2000+ training packs |
| **Business Value** | Core feature - enables players to find and load practice scenarios |
| **Key Entity** | `TrainingEntry` (training pack) |
| **Root Aggregate** | `TrainingPackCatalog` (proposed) |
| **Current Implementation** | `TrainingPackManager.h/cpp`, embedded in `MapList.h` |

**Dependencies:**
- External: RLMAPS API / Prejump training database
- Internal: `PackUsageTracker`, `SettingsSync`

**Key Operations:**
- Load packs from persistent storage (`training_packs.json`)
- Download/sync fresh pack data from web sources
- Search and filter by difficulty, tags, shot count
- CRUD operations on custom packs
- Track usage statistics for favorites

**Data Model:**
```cpp
struct TrainingEntry {
    std::string code;              // Unique identifier (e.g., "XXXX-XXXX-XXXX-XXXX")
    std::string name;              // Display name
    std::string creator;           // Creator's name
    std::string creatorSlug;       // Creator's username
    std::string difficulty;        // Bronze, Gold, Platinum, Diamond, Champion, Supersonic Legend
    std::vector<std::string> tags; // Categorization (Defense, Offense, etc.)
    int shotCount;                 // Number of shots in pack
    std::string staffComments;     // Official notes
    std::string notes;             // Creator's notes
    std::string videoUrl;          // YouTube link
    int likes, plays;              // Engagement metrics
    int status;                    // Pack status (active/inactive)
    std::string source;            // "prejump" or "custom"
    bool isModified;               // Track user edits
};
```

**Invariants Enforced:**
- Pack code must be unique within the catalog
- Difficulty must be one of the predefined values
- Shot count must be ≥ 0
- Source must be either "prejump" or "custom"

---

#### **Domain: Map & Venue Discovery**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Discover available maps (freeplay, workshop) on player's system |
| **Business Value** | Enables access to diverse training environments |
| **Key Entity** | `MapEntry` (freeplay), `WorkshopEntry` (custom maps) |
| **Root Aggregate** | `MapRepository` (proposed) |
| **Current Implementation** | `MapManager.h/cpp`, `MapList.h` |

**Dependencies:**
- File system discovery (workshop folder scanning)
- Metadata parsing (JSON, image files)

**Key Operations:**
- Discover workshop maps on disk
- Parse workshop metadata (title, author, description)
- Load preview images
- Resolve configured workshop folders
- Maintain in-memory catalog with disk scanning

**Data Model:**
```cpp
struct MapEntry {
    std::string code;              // Map code (e.g., "beckwith_park_p")
    std::string name;              // Display name
};

struct WorkshopEntry {
    std::string filePath;          // UPK file path
    std::string name;              // Display name
    std::string author;            // Map creator
    std::string description;       // Map description
    std::filesystem::path folder;  // Folder containing map
    std::filesystem::path previewPath;  // Preview image
    std::shared_ptr<ImageWrapper> previewImage;
    bool isImageLoaded;            // Lazy loading flag
};
```

**Invariants Enforced:**
- File path must point to valid UPK/UDK file
- Workshop entries must have valid folder paths
- Code must be non-empty for freeplay maps

---

#### **Domain: Automation & Match-End Orchestration**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Coordinate post-match load sequence based on user settings |
| **Business Value** | Core automation - seamlessly transitions players to training |
| **Key Entity** | `MatchEndEvent`, `LoadSequence` |
| **Root Aggregate** | `AutomationOrchestrator` (proposed) |
| **Current Implementation** | `AutoLoadFeature.h/cpp` |

**Dependencies:**
- Game events (Match Ended)
- Settings (what to load, delay times)
- Map/pack catalogs (what to load from)
- BakkesMod commands (the actual loading)

**Key Operations:**
- Listen for match-end events
- Calculate appropriate delays per load type
- Determine which map/pack to load
- Execute scheduled load commands
- Handle error states gracefully

**Event Flow:**
```
MatchEnded
    ↓
CheckAutoLoadSettings
    ↓ (if enabled)
DetermineLoadTarget (based on settings)
    ↓
CalculateDelay (queue/freeplay/training/workshop specific)
    ↓
ScheduleLoadCommand (gameWrapper->SetTimeout)
    ↓
ExecuteLoadCommand
    ↓
UpdateUsageStatistics
```

**Key Design Detail:** Deferred execution is **critical** - loading immediately during match-end crashes the game. Uses `gameWrapper->SetTimeout()` to schedule commands.

---

#### **Domain: Settings & Configuration Management**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Manage all user preferences and CVars persistently |
| **Business Value** | Ensures user preferences are remembered across sessions |
| **Key Entity** | `Setting`, `SettingValue` |
| **Root Aggregate** | `UserConfiguration` (proposed) |
| **Current Implementation** | `SettingsSync.h/cpp` |

**Dependencies:**
- BakkesMod CVar manager (persistence layer)

**Key Operations:**
- Register all CVars with BakkesMod on startup
- Maintain local cache of settings for fast access
- Sync changes from BakkesMod console to local state
- Provide type-safe getters/setters

**Current Settings Schema:**
```cpp
bool enabled;                      // Master enable/disable
int mapType;                       // 0=Freeplay, 1=Training, 2=Workshop
bool autoQueue;                    // Auto-queue after load
int quickPicksListType;            // 0=Flicks, 1=Favorites
int quickPicksCount;               // Number of quick picks
std::string quickPicksSelected;    // Selected quick pick code

// Delay times (seconds)
int delayQueueSec;
int delayFreeplaySec;
int delayTrainingSec;
int delayWorkshopSec;

// Current selections
std::string currentFreeplayCode;   // e.g., "beckwith_park_p"
std::string currentTrainingCode;   // e.g., "XXXX-XXXX-XXXX-XXXX"
std::string currentWorkshopPath;   // File path to UPK
```

**CVar Naming Convention:**
All settings use `suitespot_` prefix: `suitespot_enabled`, `suitespot_delay_queue`, etc.

---

#### **Domain: Loadout Management**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Switch car presets/loadouts before/after matches |
| **Business Value** | Optional QoL feature - seamless preset switching |
| **Key Entity** | `Loadout`, `LoadoutPreset` |
| **Root Aggregate** | `LoadoutRegistry` (proposed) |
| **Current Implementation** | `LoadoutManager.h/cpp` |

**Dependencies:**
- BakkesMod loadout API (Game Thread)
- Game wrapper for thread-safe execution

**Key Operations:**
- Query available loadouts from game
- Cache loadout names for UI
- Switch to specific loadout by name or index
- Execute on Game Thread for safety

**Thread Safety Pattern:**
Uses `gameWrapper->Execute()` to run loadout operations on the Game Thread, preventing crashes from concurrent access.

---

### 1.2 Supporting Domains

#### **Domain: Usage Analytics & Favorites**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Track pack loads and identify top-used packs |
| **Business Value** | Powers "Your Favorites" feature via usage statistics |
| **Key Entity** | `PackUsageRecord` |
| **Root Aggregate** | `UsageStatistics` (proposed) |
| **Current Implementation** | `PackUsageTracker.h/cpp` |

**Dependencies:**
- File system (JSON persistence)

**Key Operations:**
- Load usage stats from disk
- Increment load count for used packs
- Query top N packs by usage
- Persist stats to `pack_usage_stats.json`

**Data Model:**
```cpp
struct PackUsageStats {
    std::string code;              // Pack code
    int loadCount;                 // Times loaded
    int64_t lastLoadedTimestamp;   // Last load time
};
```

---

#### **Domain: Workshop Map Sourcing & Downloads**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Search and download workshop maps from RLMAPS API |
| **Business Value** | Expands available maps beyond installed workshops |
| **Key Entity** | `RemoteWorkshopMap`, `DownloadJob` |
| **Root Aggregate** | `WorkshopMarketplace` (proposed) |
| **Current Implementation** | `WorkshopDownloader.h/cpp` |

**Dependencies:**
- External: RLMAPS API (https://celab.jetfox.ovh/api/v4/projects/)
- HTTP client (BakkesMod)
- File system (downloads)
- PowerShell (zip extraction)

**Key Operations:**
- Search for maps by keyword
- Download map preview images
- Download and extract map files
- Parse release information
- Create local workshop metadata

**API Integration:**
```cpp
struct RLMAPS_MapResult {
    std::string ID;
    std::string Name;
    std::string Size;
    std::string Description;
    std::string PreviewUrl;
    std::string Author;
    std::vector<RLMAPS_Release> releases;
};

struct RLMAPS_Release {
    std::string name;
    std::string tag_name;
    std::string description;
    std::string zipName;
    std::string downloadLink;
    std::string pictureLink;
};
```

---

### 1.3 Infrastructure Domains

#### **Domain: User Interface & Presentation**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Render and manage all UI menus and windows |
| **Key Components** | `SettingsUI`, `TrainingPackUI`, `LoadoutUI`, `StatusMessageUI` |
| **Current Implementation** | `SettingsUI.cpp/h`, `TrainingPackUI.cpp/h`, `LoadoutUI.cpp/h`, `StatusMessageUI.cpp/h`, `HelpersUI.cpp/h` |

**Architecture:**
- ImGui-based immediate-mode rendering
- Hybrid rendering for persistent windows (browser stays open when F2 closes)
- Status messages for user feedback

**Dependencies:**
- BakkesMod plugin window interface
- ImGui (immediate-mode GUI library)
- All domain contexts (reads from them for display)

---

#### **Domain: Plugin Lifecycle & Integration**
| Aspect | Details |
|--------|---------|
| **Responsibility** | Plugin initialization, event hooks, central coordination |
| **Key Component** | `SuiteSpot` (hub) |
| **Current Implementation** | `SuiteSpot.h/cpp` |

**Lifecycle Events:**
- `onLoad()` - Initialize all managers and UI on startup
- `onUnload()` - Clean up resources on shutdown
- `RenderSettings()` - Render F2 menu
- `Render()` - Render floating windows

**Hub Responsibilities:**
- Create and manage all manager instances
- Register game event hooks
- Ensure data directories exist
- Coordinate between managers
- Provide access to common paths/getters

---

## 2. DOMAIN ENTITIES & AGGREGATES

### 2.1 Current Entity Model

#### **Aggregate: TrainingPackCatalog**
**Aggregate Root:** `TrainingPackManager` (proposed refactoring)

```
TrainingPackCatalog
├── identity: CatalogId
├── packs: PackCollection
│   └── TrainingEntry (Entity)
│       ├── identity: PackCode
│       ├── name: String
│       ├── creator: Creator (Value Object)
│       ├── metadata: PackMetadata (Value Object)
│       └── engagement: Engagement (Value Object)
├── cachedTags: TagCollection
├── lastUpdated: Timestamp
└── operations:
    ├── LoadFromDisk(path) → void
    ├── SyncWithWeb() → void
    ├── Search(criteria) → PackList
    ├── AddCustom(pack) → void
    ├── UpdatePackMetadata(code, metadata) → void
    └── Heal(code, shots) → void
```

**Current Problem:** `TrainingPackManager` holds local vector of `RLTraining` (global), mixing concerns.

---

#### **Aggregate: MapRepository**
**Aggregate Root:** `MapManager` (proposed refactoring)

```
MapRepository
├── identity: RepositoryId
├── freeplayMaps: MapList
│   └── MapEntry (Value Object - no identity)
│       ├── code: String
│       └── name: String
├── workshopMaps: WorkshopMapCollection
│   └── WorkshopEntry (Entity)
│       ├── identity: FilePath
│       ├── name: String
│       ├── metadata: MapMetadata (Value Object)
│       └── preview: PreviewImage (Value Object)
├── dataRoot: Path
└── operations:
    ├── DiscoverWorkshopMaps(dir) → void
    ├── LoadWorkshopMetadata(path) → MapMetadata
    ├── FindPreviewImage(folder) → ImagePath
    └── ResolveWorkshopRoot() → Path
```

**Current Problem:** Maps are scattered between `MapList.h` globals and `MapManager` logic.

---

#### **Aggregate: AutomationOrchestrator**
**Aggregate Root:** `AutoLoadFeature` (exact name, good design)

```
AutomationOrchestrator
├── identity: OrchestrationId
├── eventHandler: MatchEndEventHandler
│   └── MatchEndedEvent (Domain Event)
└── operations:
    ├── OnMatchEnded(context) → void
        └── GetLoadTarget(settings) → LoadTarget
        └── CalculateDelay(target, settings) → DelayMs
        └── ScheduleLoadCommand(target, delay) → void
```

**Good Design:** Already encapsulated as a focused class. Minimal refactoring needed.

---

#### **Aggregate: UserConfiguration**
**Aggregate Root:** `SettingsSync` (current implementation)

```
UserConfiguration
├── identity: UserId (implicit - single instance)
├── enabled: Boolean
├── mapType: MapTypeSelection
├── autoQueue: Boolean
├── delays: DelayConfiguration (Value Object)
│   ├── queueDelay: Int (seconds)
│   ├── freeplayDelay: Int (seconds)
│   ├── trainingDelay: Int (seconds)
│   └── workshopDelay: Int (seconds)
├── selections: CurrentSelections (Value Object)
│   ├── currentFreeplayCode: String
│   ├── currentTrainingCode: String
│   └── currentWorkshopPath: String
├── quickPicks: QuickPicksConfig (Value Object)
│   ├── listType: ListType (Flicks|Favorites)
│   ├── count: Int
│   └── selected: String
└── operations:
    ├── Load() → void
    ├── Save() → void
    ├── Get*(key) → Value
    └── Set*(key, value) → void
```

**Current Problem:** Weak type safety - everything is primitive types with no validation.

---

#### **Aggregate: LoadoutRegistry**
**Aggregate Root:** `LoadoutManager`

```
LoadoutRegistry
├── identity: LoadoutId
├── availableLoadouts: LoadoutList
│   └── Loadout (Entity)
│       ├── identity: LoadoutName
│       ├── index: Int
│       └── carPreset: CarPresetData
├── operations:
    ├── QueryAvailableLoadouts() → LoadoutList
    ├── GetCurrent(callback) → void
    ├── Switch(loadoutName, callback) → void
    └── Refresh() → void
```

**Current Issue:** Heavy reliance on game callbacks, hard to test in isolation.

---

#### **Aggregate: UsageStatistics**
**Aggregate Root:** `PackUsageTracker`

```
UsageStatistics
├── identity: StatisticsId
├── records: Map<PackCode, PackUsageRecord>
│   └── PackUsageRecord (Entity)
│       ├── identity: PackCode
│       ├── loadCount: Int
│       └── lastLoadedTimestamp: Int64
├── filePath: Path
└── operations:
    ├── Load() → void
    ├── Save() → void
    ├── IncrementLoadCount(code) → void
    ├── GetTopUsed(count) → CodeList
    └── IsFirstRun() → Boolean
```

**Current State:** Well-designed, minimal issues.

---

### 2.2 Proposed Value Objects

| Value Object | Use | Components |
|--------------|-----|------------|
| `PackCode` | Training pack identifier | String (XXXX-XXXX-XXXX-XXXX format) |
| `MapCode` | Freeplay map identifier | String (e.g., "beckwith_park_p") |
| `CreatorIdentity` | Pack/map creator info | Name, Slug |
| `PackMetadata` | Training pack details | Difficulty, Tags, ShotCount, Video, StaffComments |
| `Engagement` | Social metrics | Likes, Plays, Status |
| `DelayConfiguration` | All delay settings | QueueDelay, FreeplayDelay, TrainingDelay, WorkshopDelay |
| `CurrentSelections` | What's currently selected | FreeplayCode, TrainingCode, WorkshopPath |
| `QuickPicksConfig` | Quick picks settings | ListType, Count, Selected |
| `MapMetadata` | Workshop map details | Title, Author, Description |
| `PreviewImage` | Map/pack preview | FilePath, LoadedImage |
| `Difficulty` | Pack difficulty | Enum: Bronze, Gold, Platinum, Diamond, Champion, Legend |
| `FilePath` | Workshop file path | String (absolute path) |

---

## 3. DOMAIN SERVICES

### 3.1 Services Crossing Aggregate Boundaries

| Service | Aggregates Involved | Operations | Current Implementation |
|---------|---------------------|-----------|----------------------|
| **Training Pack Loader** | TrainingPackCatalog → Automation | Load pack from catalog by code | `AutoLoadFeature::OnMatchEnded` |
| **Map Selection Service** | MapRepository → Automation | Find map to load by type | `AutoLoadFeature::OnMatchEnded` |
| **Load Command Executor** | Automation → BakkesMod | Execute load command with delay | `gameWrapper->SetTimeout`, `Execute` |
| **Loadout Applier** | LoadoutRegistry → BakkesMod | Apply loadout during automation | Part of `AutoLoadFeature` |
| **Usage Tracker** | UsageStatistics → TrainingPackCatalog | Record pack load for favorites | Called from `AutoLoadFeature` |
| **Configuration Provider** | UserConfiguration → All contexts | Provide settings to all systems | `SettingsSync` (via SuiteSpot) |

### 3.2 Anti-Corruption Layers

**Current Weaknesses:**
1. **No layer** between BakkesMod types and domain types
   - `ImageWrapper`, `GameWrapper`, `CVarManagerWrapper` leak throughout
   - Domain logic coupled to BakkesMod SDK versions

2. **No layer** between file system and domains
   - `std::filesystem::path` used directly in `WorkshopEntry`
   - No abstraction for different storage backends

**Recommended ACL:**
```cpp
// BakkesMod Adapter Layer
namespace Infrastructure {
    class BakkesModGameAdapter {
        void ExecuteLoadCommand(const LoadCommand& cmd, DelayMs delay);
        void ApplyLoadout(const LoadoutName& name);
    };

    class BakkesModSettingsAdapter {
        void RegisterCVars(const SettingDefinition[] defs);
        void PersistSetting(const SettingKey& key, const Value& val);
    };
}

// File System Adapter Layer
namespace Infrastructure {
    class FileSystemMapRepository {
        void DiscoverMaps(const MapPath& dir);
        MapMetadata LoadMetadata(const MapPath& path);
    };
}
```

---

## 4. DOMAIN EVENTS

### 4.1 Significant Domain Events

```typescript
// Training Pack Management Events
interface PackLibraryInitialized {
    type: 'PackLibraryInitialized';
    packCount: number;
    timestamp: Date;
}

interface PackLibraryUpdatedFromWeb {
    type: 'PackLibraryUpdatedFromWeb';
    newPackCount: number;
    addedPacks: number;
    removedPacks: number;
    timestamp: Date;
}

interface CustomPackAdded {
    type: 'CustomPackAdded';
    pack: TrainingEntry;
    timestamp: Date;
}

interface CustomPackModified {
    type: 'CustomPackModified';
    packCode: string;
    changes: Record<string, any>;
    timestamp: Date;
}

interface CustomPackDeleted {
    type: 'CustomPackDeleted';
    packCode: string;
    timestamp: Date;
}

// Map Discovery Events
interface WorkshopMapDiscovered {
    type: 'WorkshopMapDiscovered';
    mapEntry: WorkshopEntry;
    timestamp: Date;
}

interface WorkshopMetadataLoaded {
    type: 'WorkshopMetadataLoaded';
    mapPath: string;
    metadata: MapMetadata;
    timestamp: Date;
}

// Automation Events
interface MatchEnded {
    type: 'MatchEnded';
    timestamp: Date;
    matchDetails?: Record<string, any>;
}

interface AutoLoadScheduled {
    type: 'AutoLoadScheduled';
    targetType: 'freeplay' | 'training' | 'workshop';
    targetId: string;
    delayMs: number;
    timestamp: Date;
}

interface MapLoadingStarted {
    type: 'MapLoadingStarted';
    mapType: string;
    mapId: string;
    timestamp: Date;
}

interface MapLoadingCompleted {
    type: 'MapLoadingCompleted';
    mapType: string;
    mapId: string;
    timestamp: Date;
}

// Settings Events
interface ConfigurationChanged {
    type: 'ConfigurationChanged';
    settingKey: string;
    oldValue: any;
    newValue: any;
    timestamp: Date;
}

interface AutoLoadEnabledToggled {
    type: 'AutoLoadEnabledToggled';
    enabled: boolean;
    timestamp: Date;
}

// Usage Events
interface PackLoaded {
    type: 'PackLoaded';
    packCode: string;
    timestamp: Date;
}

interface FavoritePacksList {
    type: 'FavoritesUpdated';
    topPacks: string[];
    timestamp: Date;
}

// Loadout Events
interface LoadoutSwitched {
    type: 'LoadoutSwitched';
    fromLoadout: string;
    toLoadout: string;
    timestamp: Date;
}

// Workshop Sourcing Events
interface WorkshopSearchInitiated {
    type: 'WorkshopSearchInitiated';
    query: string;
    timestamp: Date;
}

interface WorkshopSearchCompleted {
    type: 'WorkshopSearchCompleted';
    query: string;
    resultCount: number;
    timestamp: Date;
}

interface WorkshopMapDownloaded {
    type: 'WorkshopMapDownloaded';
    mapId: string;
    filePath: string;
    timestamp: Date;
}
```

### 4.2 Event Publishing Recommendations

| Event | Publisher | Subscribers | When |
|-------|-----------|-------------|------|
| `PackLibraryUpdatedFromWeb` | TrainingPackCatalog | UI (refresh), Analytics | After web sync completes |
| `CustomPackAdded` | TrainingPackCatalog | UI (refresh), Analytics | After custom pack creation |
| `MatchEnded` | Game hooks | AutomationOrchestrator | Game event received |
| `AutoLoadScheduled` | AutomationOrchestrator | Analytics, UI (status) | Command scheduled |
| `MapLoadingCompleted` | AutomationOrchestrator | Analytics, UsageStatistics | Load completes |
| `ConfigurationChanged` | UserConfiguration | All contexts (reload) | After CVar update |
| `PackLoaded` | AutomationOrchestrator | UsageStatistics | Pack executes |
| `LoadoutSwitched` | LoadoutRegistry | Analytics | Loadout change completes |
| `WorkshopMapDownloaded` | WorkshopMarketplace | MapRepository (refresh) | Download completes |

---

## 5. UBIQUITOUS LANGUAGE

### 5.1 Core Domain Vocabulary

| Term | Definition | Example |
|------|-----------|---------|
| **Training Pack** | A preset collection of shots/scenarios for practice | "Aerial Pack PRO v2" |
| **Pack Code** | Unique identifier for a training pack | "XXXX-XXXX-XXXX-XXXX" |
| **Difficulty** | Skill level required for a pack | Bronze, Gold, Platinum, Diamond, Champion, Supersonic Legend |
| **Freeplay Map** | Standard Rocket League arena without matches | Beckwith Park, Narwhal Beach |
| **Workshop Map** | Custom-created map, typically downloaded | "Curved Ceiling Training" |
| **Map Type** | User's choice of load target | Freeplay, Training, Workshop |
| **Auto-Load** | Automatic transition to chosen venue after match | Feature enabling post-match automation |
| **Delay** | Wait time before loading next arena | 5 seconds before queue, 2 seconds before freeplay |
| **Quick Picks** | User's curated list of favorite packs | "Your Favorites" or "Flicks Picks" |
| **Shuffle Bag** | Rotation of related training packs | (Currently removed, was: Defense Bag, Offense Bag, etc.) |
| **Loadout** | Car preset/configuration | "Fennec Setup", "Octane Default" |
| **Usage Statistics** | Historical record of pack loads | Used to determine user's most-used packs |
| **Pack Healing** | Updating a pack's shot count after community fixes | "Heal pack from 50 to 52 shots" |
| **Custom Pack** | Manually added training pack not from Prejump | User-created or edited pack |
| **Pack Source** | Origin of training pack | "prejump" (official) or "custom" (user-added) |
| **Match End Event** | Game event triggered when a match completes | Hooks into `GameEndedEvent` |
| **RLMAPS** | External API for discovering workshop maps | Maps sourced from RLMAPS marketplace |

### 5.2 Context-Specific Language

#### **Training Pack Management Context**
- **Pack Catalog** - The complete library of training packs
- **Engagement Metrics** - Likes and plays counts
- **Staff Comments** - Official notes from Prejump curators
- **Creator** - User who created/submitted the pack
- **Shot** - Individual scenario within a pack
- **Pack Status** - Active/inactive indicator

#### **Map Discovery Context**
- **Map Repository** - Catalog of all available maps
- **Workshop Root** - Configured folder where custom maps are stored
- **Preview Image** - Thumbnail image for map visualization
- **Map Metadata** - Author, description, preview for workshop maps
- **UPK/UDK File** - Unreal package file format for maps

#### **Automation Context**
- **Load Target** - Which map/pack to load
- **Load Sequence** - Ordered steps for loading (e.g., switch loadout → load map → queue)
- **Orchestration** - Coordination of post-match actions
- **Deferred Execution** - Scheduling commands to run after delay
- **Load Command** - Instruction to game to load specific map

#### **Settings Context**
- **CVar** - Console Variable (BakkesMod term for setting)
- **Setting Key** - CVar name (e.g., "suitespot_enabled")
- **Configuration Snapshot** - All current settings
- **Persistence** - Saving settings to disk

#### **Loadout Context**
- **Loadout Registry** - List of available car presets
- **Current Loadout** - Actively selected car preset
- **Loadout Switch** - Action of changing to different preset

---

## 6. CONTEXT MAP

### 6.1 Bounded Context Relationships

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                         SUITESPOT BOUNDED CONTEXT MAP                        │
└──────────────────────────────────────────────────────────────────────────────┘

                    ┌─────────────────────────────────┐
                    │   USER INTERFACE CONTEXT        │
                    │  (UI & Presentation Layer)      │
                    │                                 │
                    │  - SettingsUI                   │
                    │  - TrainingPackUI               │
                    │  - LoadoutUI                    │
                    │  - StatusMessageUI              │
                    │  - HelpersUI                    │
                    └────────────┬────────────────────┘
                                 │ Reads All
                                 │ Updates Settings
                                 ▼
       ┌─────────────────────────────────────────────────────────────────┐
       │           PLUGIN LIFECYCLE CONTEXT (Hub)                       │
       │                    SuiteSpot.h/cpp                             │
       │                                                                 │
       │  - Central coordinator                                         │
       │  - Owns all manager instances                                  │
       │  - Listens for game events                                     │
       │  - Ensures data directories                                    │
       └─────────────────────────────────────────────────────────────────┘
                    │                │                  │
                    │                │                  │
        ┌───────────┘                │                  └──────────────┐
        │                            │                                │
        │                            │                                │
        ▼                            ▼                                ▼
    ┌────────────┐          ┌────────────────┐          ┌──────────────────┐
    │ TRAINING   │          │ AUTOMATION &   │          │ SETTINGS &       │
    │ PACK MGMT  │          │ ORCHESTRATION  │          │ CONFIGURATION    │
    │ CONTEXT    │          │ CONTEXT        │          │ CONTEXT          │
    │            │          │                │          │                  │
    │ TPM.h/cpp  │          │ AutoLoad.h/cpp │          │ Settings.h/cpp   │
    │ MapList.h  │          │                │          │ SettingsSync.h   │
    │            │          │ - OnMatchEnded │          │                  │
    │ - Load     │          │ - Determine    │          │ - Register CVars │
    │   packs    │          │   target       │          │ - Provide settings
    │ - Search   │◄─────────│ - Schedule     │─────────►│ - Sync changes   │
    │ - Filter   │   ACL    │   load         │   ACL    │                  │
    │ - CRUD     │          │ - Exec command │          │ - Persist        │
    │ - Update   │          │                │          │                  │
    │   web      │          └────────────────┘          └──────────────────┘
    │            │                                                │
    │            │          ┌─────────────────────────────────────┘
    │            │          │
    │            │          ▼
    │            │      ┌──────────────┐
    │            │      │ LOADOUT MGMT │
    │            │      │ CONTEXT      │
    │            │      │              │
    │            │      │LoadoutMgr.h  │
    │            │      │              │
    │            │      │ - Query      │
    │            │      │ - Get current│
    │            │      │ - Switch     │
    │            │      │ - Refresh    │
    │            │      └──────────────┘
    │            │              ▲
    │            │              │ Uses
    │            │              │ (for loadout
    │            │              │  switching during
    │            │              │  auto-load)
    └────────────┼──────────────┘
                 │
                 ▼
    ┌─────────────────────────────┐
    │ MAP & VENUE DISCOVERY       │
    │ CONTEXT                     │
    │                             │
    │ MapManager.h/cpp            │
    │ MapList.h                   │
    │                             │
    │ - Discover workshop         │
    │ - Load metadata             │
    │ - Find preview              │
    │ - Resolve workshop root     │
    └─────────────────────────────┘
            ▲         ▲
            │         │
            │         └─────────────┐
            │                       │
    ┌───────┴──────────┐    ┌──────────────┐
    │  USAGE ANALYTICS │    │ WORKSHOP     │
    │  CONTEXT         │    │ SOURCING     │
    │                  │    │ CONTEXT      │
    │ PackUsageTracker │    │              │
    │                  │    │Workshop      │
    │ - Load stats     │    │Downloader.h  │
    │ - Increment      │    │              │
    │ - Get top used   │    │ - Search API │
    │ - Persist        │    │ - Download   │
    │                  │    │ - Extract    │
    └──────────────────┘    │ - Metadata   │
                            │              │
                            └──────────────┘
```

### 6.2 Integration Patterns

| From Context | To Context | Pattern | Mechanism | Data Flow |
|--------------|-----------|---------|-----------|-----------|
| UI | Settings | Customer-Supplier | UI reads/updates settings | CVars |
| UI | Training Pack Mgmt | Conformist | UI displays packs | REST: GetPacks() |
| UI | Map Discovery | Conformist | UI displays maps | REST: GetMaps() |
| UI | Loadout Mgmt | Conformist | UI lists loadouts | REST: GetLoadouts() |
| UI | Workshop Sourcing | Conformist | UI shows downloads | REST: GetResults() |
| Settings | Automation | Partner | Both must agree on behavior | Function calls via Settings getters |
| Training Pack Mgmt | Automation | Partner | TPM provides packs to load | Function calls via GetPackByCode() |
| Map Discovery | Automation | Partner | Maps provides maps to load | Function calls via GetMapByCode() |
| Automation | Usage Analytics | Publisher-Subscriber | Publishes load events | Event: PackLoaded |
| Automation | Loadout Mgmt | Customer | Automation needs to switch | Function calls via SwitchLoadout() |
| Workshop Sourcing | Map Discovery | Published Language | JSON metadata files | File system: JSON metadata |
| All Contexts | Settings | Anti-Corruption Layer | Settings adapter shields others | CVar registration, sync |
| All Contexts | BakkesMod SDK | Anti-Corruption Layer | BakkesMod adapter hides SDK | Wrapper classes |

---

## 7. REPOSITORIES

### 7.1 Current Repository Pattern

**Problem:** Repositories are not explicitly designed. Data access is scattered.

**Current Implementations:**

| Data | Current Storage | Access Pattern | Issues |
|------|-----------------|-----------------|--------|
| Training Packs | `training_packs.json` | `TrainingPackManager::LoadPacksFromFile()` | Global vector, unsafe for concurrency |
| Freeplay Maps | `MapList.h` global | `extern std::vector<MapEntry> RLMaps` | No abstraction, hardcoded list |
| Workshop Maps | Disk scanning | `MapManager::DiscoverWorkshopInDir()` | Mutable on-demand, not persisted |
| Usage Stats | `pack_usage_stats.json` | `PackUsageTracker::LoadStats()` | Good encapsulation |
| Settings/CVars | BakkesMod CVar system | `SettingsSync` | Good abstraction |

### 7.2 Recommended Repository Interfaces

```cpp
// Training Pack Repository
class ITrainingPackRepository {
    virtual ~ITrainingPackRepository() = default;
    virtual std::vector<TrainingEntry> GetAll() const = 0;
    virtual TrainingEntry* GetByCode(const std::string& code) = 0;
    virtual void Add(const TrainingEntry& pack) = 0;
    virtual void Update(const TrainingEntry& pack) = 0;
    virtual void Delete(const std::string& code) = 0;
    virtual void SaveAll() = 0;
};

class FileSystemTrainingPackRepository : public ITrainingPackRepository {
    std::filesystem::path filePath_;
    std::vector<TrainingEntry> cache_;
    mutable std::mutex cacheMutex_;
    // implementation...
};

// Freeplay Map Repository
class IFreeplayMapRepository {
    virtual ~IFreeplayMapRepository() = default;
    virtual std::vector<MapEntry> GetAll() const = 0;
    virtual MapEntry* GetByCode(const std::string& code) = 0;
};

class StaticFreeplayMapRepository : public IFreeplayMapRepository {
    // Returns hardcoded or embedded list
    std::vector<MapEntry> maps_;  // From DefaultPacks.h
    // implementation...
};

// Workshop Map Repository
class IWorkshopMapRepository {
    virtual ~IWorkshopMapRepository() = default;
    virtual std::vector<WorkshopEntry> GetDiscovered() const = 0;
    virtual void Discover(const std::filesystem::path& root) = 0;
    virtual WorkshopEntry* GetByPath(const std::filesystem::path& path) = 0;
};

class FileSystemWorkshopMapRepository : public IWorkshopMapRepository {
    std::filesystem::path workshopRoot_;
    std::vector<WorkshopEntry> discovered_;
    mutable std::mutex discoverMutex_;
    // implementation...
};

// Usage Statistics Repository
class IUsageStatisticsRepository {
    virtual ~IUsageStatisticsRepository() = default;
    virtual std::vector<std::string> GetTopUsedCodes(int count) const = 0;
    virtual void IncrementLoadCount(const std::string& code) = 0;
    virtual void SaveAll() = 0;
};

class FileSystemUsageStatisticsRepository : public IUsageStatisticsRepository {
    // Current PackUsageTracker is already well-designed
};

// Settings Repository
class ISettingsRepository {
    virtual ~ISettingsRepository() = default;
    virtual std::string Get(const std::string& key) const = 0;
    virtual void Set(const std::string& key, const std::string& value) = 0;
    virtual void Persist() = 0;
};

class BakkesModSettingsRepository : public ISettingsRepository {
    // Wraps SettingsSync and CVarManager
};
```

---

## 8. RECOMMENDED REFACTORING

### 8.1 Phase 1: Extract Explicit Aggregates (Low Risk)

**Goal:** Make aggregate boundaries explicit without changing behavior.

**Changes:**

1. **Create `TrainingPackCatalog` Aggregate Root**
   ```cpp
   // Current
   class TrainingPackManager { ... };

   // Proposed
   class TrainingPackCatalog {
       std::vector<TrainingEntry> packs_;
       std::mutex packMutex_;

       // Aggregate root methods
       TrainingEntry* GetByCode(const std::string& code);
       void AddCustomPack(const TrainingEntry& pack);
       void UpdatePack(const TrainingEntry& pack);
       void DeletePack(const std::string& code);
       void SyncWithWebSource();
       void LoadFromDisk(const std::filesystem::path& filePath);
       void SaveToDisk(const std::filesystem::path& filePath);
   };
   ```

2. **Create `MapRepository` Aggregate Root**
   ```cpp
   class MapRepository {
       std::vector<MapEntry> freeplayMaps_;
       std::vector<WorkshopEntry> workshopMaps_;

       // Aggregate root methods
       MapEntry* GetFreeplayByCode(const std::string& code);
       WorkshopEntry* GetWorkshopByPath(const std::filesystem::path& path);
       void DiscoverWorkshop(const std::filesystem::path& root);
       void LoadWorkshopMetadata(WorkshopEntry& entry);
   };
   ```

3. **Create `AutomationOrchestrator` Wrapper** (already exists as `AutoLoadFeature`)
   - Rename for clarity: `AutoLoadFeature` → `AutomationOrchestrator` or keep name but document it

4. **Create `UserConfiguration` Aggregate Root**
   ```cpp
   class UserConfiguration {
       bool enabled_;
       int mapType_;
       DelayConfiguration delays_;
       CurrentSelections selections_;
       QuickPicksConfig quickPicks_;

       // Aggregate root methods
       bool IsEnabled() const;
       DelayConfiguration GetDelays() const;
       CurrentSelections GetCurrentSelections() const;
       void SetSelection(const CurrentSelection& selection);
   };
   ```

**Benefits:**
- Clear ownership of data
- Explicit transaction boundaries
- Testable domain logic
- No behavior change (drop-in replacement)

---

### 8.2 Phase 2: Create Anti-Corruption Layers (Medium Risk)

**Goal:** Isolate domain from infrastructure dependencies.

**Changes:**

1. **Create BakkesMod Adapter Layer**
   ```cpp
   namespace Infrastructure {
       class BakkesModGameAdapter {
           std::shared_ptr<GameWrapper> gameWrapper_;

           void ExecuteLoadCommand(const LoadCommand& cmd, DelayMs delay);
           void ApplyLoadout(const LoadoutName& name);
           void RegisterSetting(const SettingDefinition& def);
       };
   }
   ```

2. **Create File System Adapter Layer**
   ```cpp
   namespace Infrastructure {
       class FileSystemAdapter {
           std::filesystem::path GetBakkesModDataPath() const;
           std::filesystem::path GetSuiteSpotDataPath() const;

           Json LoadJson(const std::filesystem::path& path);
           void SaveJson(const std::filesystem::path& path, const Json& data);

           std::vector<std::filesystem::path> FindFiles(
               const std::filesystem::path& dir,
               const std::string& extension);
       };
   }
   ```

3. **Create Settings Adapter**
   ```cpp
   namespace Infrastructure {
       class BakkesModSettingsAdapter : public ISettingsRepository {
           std::shared_ptr<CVarManagerWrapper> cvarManager_;

           std::string Get(const std::string& key) const override;
           void Set(const std::string& key, const std::string& value) override;
       };
   }
   ```

**Benefits:**
- Domain logic decoupled from BakkesMod SDK
- Easier to mock in tests
- Clearer dependency flow
- Reduced coupling to external APIs

---

### 8.3 Phase 3: Implement Explicit Repository Interfaces (Medium Risk)

**Goal:** Provide domain layer with data access abstractions.

**Changes:**

1. **Extract Repositories**
   - `ITrainingPackRepository` with `FileSystemTrainingPackRepository`
   - `IMapRepository` with `FileSystemMapRepository`
   - `IUsageStatisticsRepository` with `FileSystemUsageStatisticsRepository`
   - `ISettingsRepository` with `BakkesModSettingsRepository`

2. **Inject Repositories into Aggregates**
   ```cpp
   class TrainingPackCatalog {
       std::unique_ptr<ITrainingPackRepository> repository_;

       TrainingPackCatalog(std::unique_ptr<ITrainingPackRepository> repo)
           : repository_(std::move(repo)) {}
   };
   ```

3. **Update SuiteSpot Hub**
   ```cpp
   class SuiteSpot {
       std::unique_ptr<ITrainingPackRepository> packRepo_;
       std::unique_ptr<IMapRepository> mapRepo_;
       std::unique_ptr<ISettingsRepository> settingsRepo_;
       std::unique_ptr<IUsageStatisticsRepository> statsRepo_;

       void onLoad() {
           packRepo_ = std::make_unique<FileSystemTrainingPackRepository>(...);
           mapRepo_ = std::make_unique<FileSystemMapRepository>(...);
           // ...
       }
   };
   ```

**Benefits:**
- Testability (mock repositories)
- Pluggable implementations
- Dependency clarity
- Supports multiple storage backends

---

### 8.4 Phase 4: Add Domain Events (Low-Medium Risk)

**Goal:** Enable loose coupling via domain events.

**Changes:**

1. **Create Event System**
   ```cpp
   namespace Domain {
       class DomainEvent {
           virtual ~DomainEvent() = default;
       };

       class EventPublisher {
           using Handler = std::function<void(const DomainEvent&)>;

           void Subscribe(const std::type_info& eventType, Handler handler);
           void Publish(const DomainEvent& event);
       };
   }
   ```

2. **Define Domain Events**
   - `PackLibraryUpdatedEvent`
   - `CustomPackAddedEvent`
   - `MatchEndedEvent`
   - `AutoLoadScheduledEvent`
   - `ConfigurationChangedEvent`
   - `PackLoadedEvent`

3. **Publish Events from Aggregates**
   ```cpp
   void TrainingPackCatalog::SyncWithWeb() {
       // ... sync logic ...
       PublishEvent(PackLibraryUpdatedEvent{
           newPackCount, addedCount, removedCount, DateTime::Now()
       });
   }
   ```

4. **Subscribe in Infrastructure**
   ```cpp
   publisher.Subscribe<PackLoadedEvent>([this](const auto& evt) {
       usageTracker_->IncrementLoadCount(evt.packCode);
   });
   ```

**Benefits:**
- Decoupled domains
- Clear audit trail
- Easier to add features (e.g., analytics, notifications)
- Supports event sourcing in future

---

### 8.5 Phase 5: Strengthen Value Objects (Low Risk)

**Goal:** Add type safety and domain logic to primitives.

**Changes:**

1. **Create Value Objects**
   ```cpp
   // Before
   std::string packCode = "XXXX-XXXX-XXXX-XXXX";

   // After
   class PackCode {
       std::string value_;

   public:
       explicit PackCode(const std::string& code) {
           if (!IsValidFormat(code)) {
               throw InvalidPackCodeError();
           }
           value_ = code;
       }

       const std::string& ToString() const { return value_; }

       bool operator==(const PackCode& other) const {
           return value_ == other.value_;
       }

   private:
       static bool IsValidFormat(const std::string& code);
   };
   ```

2. **Common Value Objects**
   ```cpp
   class Difficulty final {
       enum Level { Bronze, Gold, Platinum, Diamond, Champion, Legend };
       Level level_;

       // Validation, comparison, formatting
   };

   class Creator final {
       std::string name_;
       std::string slug_;

       // Never changes once set
       bool operator==(const Creator& other) const;
   };

   class DelayConfiguration final {
       int queueDelay_;
       int freeplayDelay_;
       int trainingDelay_;
       int workshopDelay_;

       bool IsValid() const;
       // All fields required in constructor (forces completeness)
   };
   ```

**Benefits:**
- Type safety (can't pass wrong type by accident)
- Domain validation (invalid values rejected at creation)
- Self-documenting code
- Reduced bugs

---

### 8.6 Risk Assessment & Implementation Order

| Phase | Risk | Implementation Time | Breaking Changes | Dependencies |
|-------|------|--------------------|-----------------|----|
| 1: Aggregates | Low | 3-4 days | None | None |
| 2: ACL | Medium | 4-5 days | Minimal | Phase 1 |
| 3: Repositories | Medium | 5-7 days | Moderate | Phase 1, 2 |
| 4: Domain Events | Low-Med | 3-4 days | None | Phase 1, 3 |
| 5: Value Objects | Low | 2-3 days | None | Phase 1 |

**Recommended Order:**
1. Start with **Phase 1** (Aggregates) - safe, minimal risk
2. Proceed to **Phase 5** (Value Objects) - complementary, improves Phase 1
3. Add **Phase 4** (Domain Events) - enhances architecture
4. Implement **Phase 2** (ACL) - improves testability
5. Complete with **Phase 3** (Repositories) - maximum benefit, most work

---

## 9. ARCHITECTURAL PATTERNS & PRINCIPLES

### 9.1 Current Patterns

| Pattern | Usage | Assessment |
|---------|-------|------------|
| Hub-and-Spoke | SuiteSpot as central coordinator | ✓ Good for plugin architecture |
| Thread Safety (Mutex) | Protecting concurrent access to collections | ✓ Appropriate |
| Deferred Execution | Using SetTimeout for game thread safety | ✓ Required for BakkesMod |
| Immediate-Mode UI | ImGui for rendering settings | ✓ Works well |
| Lazy Loading | Preview images loaded on demand | ✓ Good for performance |
| Data Persistence | JSON files in APPDATA folder | ✓ Appropriate |

### 9.2 Recommended Patterns to Adopt

| Pattern | Purpose | Example |
|---------|---------|---------|
| **Aggregate Pattern** | Enforce consistency boundaries | TrainingPackCatalog, MapRepository |
| **Repository Pattern** | Abstract data access | ITrainingPackRepository, IMapRepository |
| **Anti-Corruption Layer** | Isolate from external APIs | BakkesModAdapter, FileSystemAdapter |
| **Domain Events** | Loose coupling between contexts | PackLoadedEvent, ConfigurationChanged |
| **Value Objects** | Type-safe domain concepts | PackCode, Difficulty, Creator |
| **Service Layer** | Cross-aggregate operations | AutomationOrchestrator (already exists) |
| **Factory Pattern** | Complex object creation | TrainingEntryFactory, WorkshopMapFactory |
| **Specification Pattern** | Encapsulate query logic | DifficultySpecification, TagSpecification |

---

## 10. TESTING IMPLICATIONS

### 10.1 Current Testing Challenges

1. **No Unit Tests** - Domain logic tightly coupled to BakkesMod SDK
2. **Manual Integration** - Hard to test without running the plugin
3. **Global State** - `RLTraining`, `RLMaps`, `RLWorkshop` vectors difficult to mock
4. **Game Thread Dependency** - Loadout and command execution tied to GameWrapper

### 10.2 Post-Refactoring Testing Strategy

**Unit Tests** (after Phase 1-2):
```cpp
// Mock repository
class MockTrainingPackRepository : public ITrainingPackRepository {
    // ... implementation ...
};

// Test aggregate in isolation
TEST(TrainingPackCatalogTests, SearchFiltersCorrectly) {
    auto mockRepo = std::make_unique<MockTrainingPackRepository>();
    mockRepo->AddPack(TrainingEntry{...});

    TrainingPackCatalog catalog(std::move(mockRepo));
    auto results = catalog.Search("aerial", Difficulty::Platinum, {});

    ASSERT_EQ(results.size(), 1);
    ASSERT_EQ(results[0].code, "XXXX-XXXX-XXXX-XXXX");
}
```

**Integration Tests** (after Phase 3):
```cpp
// Real repository, mock BakkesMod
TEST(AutomationOrchestratorTests, OnMatchEndedLoadsTraining) {
    auto realPackRepo = std::make_unique<FileSystemTrainingPackRepository>(...);
    auto mockGameAdapter = std::make_unique<MockBakkesModGameAdapter>();

    AutomationOrchestrator orchestrator(
        std::move(realPackRepo),
        std::move(mockGameAdapter)
    );

    // ... setup expectations ...
    orchestrator.OnMatchEnded(/* parameters */);
    // ... verify ExecuteLoadCommand was called ...
}
```

**Event Testing** (after Phase 4):
```cpp
TEST(TrainingPackCatalogTests, PublishesEventOnAdd) {
    auto eventCapture = std::make_unique<MockEventPublisher>();

    TrainingPackCatalog catalog(...);
    catalog.SetEventPublisher(std::move(eventCapture));

    catalog.AddCustomPack(TrainingEntry{...});

    ASSERT_TRUE(eventCapture->WasEventPublished<CustomPackAddedEvent>());
}
```

---

## 11. GLOSSARY OF DDD TERMS

| Term | Definition | SuiteSpot Example |
|------|-----------|------------------|
| **Aggregate** | Cluster of domain objects bound by invariants | TrainingPackCatalog (root) containing TrainingEntry objects |
| **Aggregate Root** | Entity that controls invariants for an aggregate | TrainingPackCatalog (no external access to internal packs) |
| **Bounded Context** | Boundary within which a domain model is valid | Training Pack Management (TrainingEntry is a pack, elsewhere might mean something different) |
| **Context Map** | Diagram showing relationships between bounded contexts | Visualization of how UI, Automation, and Settings interact |
| **Domain Event** | Something that happened in the domain | PackLoaded, MatchEnded |
| **Domain Model** | Representation of domain concepts and relationships | TrainingEntry, MapEntry, AutomationOrchestrator |
| **Entity** | Object with identity that persists over time | TrainingEntry (identity: packCode) |
| **Repository** | Collection-like interface for accessing aggregates | ITrainingPackRepository |
| **Ubiquitous Language** | Shared vocabulary between domain experts and developers | "Training Pack", "Pack Code", "Auto-Load" |
| **Value Object** | Object without identity, defined by its attributes | PackCode, Difficulty, DelayConfiguration |
| **Anti-Corruption Layer** | Facade between bounded contexts to prevent contamination | BakkesModAdapter (shields domain from SDK changes) |
| **Published Language** | Language for communication between bounded contexts | Domain events published as JSON |
| **Service** | Operation that doesn't belong to an entity or value object | AutomationOrchestrator (coordinates multiple aggregates) |

---

## 12. CONCLUSION & NEXT STEPS

### 12.1 Current State Assessment

**Strengths:**
- ✓ Clear separation of concerns (Managers, UI, Core logic)
- ✓ Good thread-safety practices (mutexes where needed)
- ✓ Appropriate use of deferred execution (SetTimeout)
- ✓ Reasonable data persistence (JSON files)

**Weaknesses:**
- ✗ No explicit aggregate boundaries
- ✗ Global state (`RLTraining`, `RLMaps`, `RLWorkshop`)
- ✗ Tight coupling to BakkesMod SDK
- ✗ Limited testability
- ✗ Weak type safety (primitives instead of value objects)
- ✗ No event-driven communication

### 12.2 Strategic Recommendation

**Adopt a Phased DDD Refactoring:**

1. **Months 1-2: Phase 1 + Phase 5**
   - Extract explicit aggregates
   - Introduce value objects
   - Zero breaking changes
   - Improves code clarity immediately

2. **Months 2-3: Phase 4 + Phase 2**
   - Add domain events
   - Create anti-corruption layers
   - Enables future extensibility

3. **Months 3-4: Phase 3**
   - Implement repository interfaces
   - Finalize testing strategy
   - Achieve full DDD architecture

### 12.3 Success Metrics

After refactoring:
- ✓ 80%+ unit test coverage
- ✓ Zero dependency on SDK types in domain layer
- ✓ Each aggregate has explicit boundaries
- ✓ Events published for all significant state changes
- ✓ All value objects have domain validation
- ✓ Repository implementations pluggable

### 12.4 Files to Create/Modify

**New Files to Create:**
- `Domain/Aggregates/TrainingPackCatalog.h/cpp`
- `Domain/Aggregates/MapRepository.h/cpp`
- `Domain/Aggregates/UserConfiguration.h/cpp`
- `Domain/Aggregates/LoadoutRegistry.h/cpp`
- `Domain/ValueObjects/PackCode.h/cpp`
- `Domain/ValueObjects/Difficulty.h/cpp`
- `Domain/ValueObjects/Creator.h/cpp`
- `Domain/ValueObjects/DelayConfiguration.h/cpp`
- `Domain/Events/DomainEvent.h`, `EventPublisher.h/cpp`
- `Application/Services/AutomationOrchestrator.h/cpp`
- `Infrastructure/Adapters/BakkesModGameAdapter.h/cpp`
- `Infrastructure/Adapters/BakkesModSettingsAdapter.h/cpp`
- `Infrastructure/Adapters/FileSystemAdapter.h/cpp`
- `Infrastructure/Repositories/TrainingPackRepository.h/cpp`
- `Infrastructure/Repositories/MapRepository.h/cpp`
- `Tests/Unit/TrainingPackCatalogTests.cpp`
- `Tests/Integration/AutomationOrchestratorTests.cpp`

**Files to Refactor:**
- `SuiteSpot.h/cpp` - Simplify hub, inject dependencies
- `AutoLoadFeature.h/cpp` - Rename to AutomationOrchestrator
- `SettingsSync.h/cpp` - Implement UserConfiguration aggregate
- `TrainingPackManager.h/cpp` - Extract to aggregate + repository
- `MapManager.h/cpp` - Extract to aggregate + repository
- `LoadoutManager.h/cpp` - Wrap as LoadoutRegistry aggregate

**Files to Retire (gradually):**
- `MapList.h` - Replace with repository pattern
- Global vectors in various files - Encapsulate in aggregates

---

## APPENDIX: CONTEXT MAP VISUALIZATION (ASCII)

```
┌────────────────────────────────────────────────────────────────────────┐
│                    EXTERNAL SYSTEMS                                    │
├────────────────────────────────────────────────────────────────────────┤
│  BakkesMod SDK        RLMAPS API           File System              │
│  (GameWrapper,       (Workshop maps)      (JSON files)             │
│   CVarManager)                                                       │
└────────────────────────────────────────────────────────────────────────┘
         ▲                   ▲                      ▲
         │                   │                      │
         │ ACL               │ ACL                  │ ACL
         │                   │                      │
┌────────┴──────────────────┴──────────────────────┴──────────────────────┐
│                    INFRASTRUCTURE LAYER                                  │
├────────────────────────────────────────────────────────────────────────┤
│  BakkesModGameAdapter    WorkshopMarketplace    FileSystemAdapter     │
│  BakkesModSettingsAdapter                      FileSystemRepository   │
└────────────────────────────────────────────────────────────────────────┘
         ▲         ▲              ▲                      ▲
         │         │              │                      │
┌────────┼─────────┼──────────────┼──────────────────────┴──────────────┐
│        │ Uses    │              │ Uses                                │
│        │         │              │                                    │
│   ┌────▼────┐ ┌──▼──────────┐ ┌──▼──────────┐  ┌─────────────────┐ │
│   │Automation│ │User         │ │Training Pack│  │Map              │ │
│   │          │ │Configuration│ │Catalog      │  │Repository       │ │
│   │Orch.     │ │             │ │             │  │                 │ │
│   └────┬─────┘ └──┬──────────┘ └──┬──────────┘  └────┬────────────┘ │
│        │ Reads    │               │ Uses             │               │
│        │          │               │                  │               │
│        └──────────┼───────────────┼──────────────────┘               │
│                   │               │                                  │
│              ┌────▼───────────────▼──────────┐                      │
│              │  Usage Analytics              │                      │
│              │  (PackUsageTracker)           │                      │
│              └──────────────────────────────┘                       │
│                                                                      │
│                    DOMAIN LAYER                                     │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
         ▲                                              ▲
         │ Calls/Reads                                 │ Reads
         │                                             │
┌────────┴─────────────────────────────────────────────┴──────────────┐
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              PRESENTATION LAYER (UI)                        │  │
│  │  ┌─────────────────────────────────────────────────────┐   │  │
│  │  │  SettingsUI    TrainingPackUI    LoadoutUI        │   │  │
│  │  │  StatusMessageUI    HelpersUI    WorkshopUI       │   │  │
│  │  └─────────────────────────────────────────────────────┘   │  │
│  │              ImGui Rendering                               │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              PLUGIN LIFECYCLE (Hub)                         │  │
│  │              SuiteSpot.h/cpp                               │  │
│  │              - Owns all managers                            │  │
│  │              - Listens for game events                      │  │
│  │              - Coordinates initialization                   │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

---

**Document prepared for strategic architecture planning.**
**Recommend review with team before implementation.**

