# SuiteSpot BakkesMod Plugin - Comprehensive Improvement Specification

**Version**: 1.0  
**Generated**: January 19, 2026  
**Purpose**: Actionable improvement roadmap for coding agents and developers

---

## Executive Summary

This document provides a comprehensive analysis of the SuiteSpot BakkesMod plugin based on thorough examination of the codebase, architecture documentation, and UI/UX analysis. It identifies bugs, improvement areas, and provides detailed implementation specifications that can be directly used by coding agents.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Critical Bugs](#2-critical-bugs)
3. [Memory Management Issues](#3-memory-management-issues)
4. [UI/UX Improvements](#4-uiux-improvements)
5. [Performance Optimizations](#5-performance-optimizations)
6. [Code Quality Improvements](#6-code-quality-improvements)
7. [New Feature Specifications](#7-new-feature-specifications)
8. [Testing Requirements](#8-testing-requirements)
9. [Implementation Priority Matrix](#9-implementation-priority-matrix)

---

## 1. Architecture Overview

### Current Structure
```
SuiteSpot (Plugin Hub)
    ├── Managers (Business Logic)
    │   ├── MapManager           - Map discovery and persistence
    │   ├── TrainingPackManager  - Training pack data and bag system
    │   ├── SettingsSync         - CVar registration and persistence
    │   ├── AutoLoadFeature      - Post-match automation logic
    │   └── LoadoutManager       - Car preset switching
    │
    └── UI Layer (Presentation)
        ├── SettingsUI           - F2 settings tab
        ├── TrainingPackUI       - Floating browser window
        └── LoadoutUI            - Loadout management controls
```

### Key Files Involved
| File | Lines (Est.) | Responsibility |
|------|--------------|----------------|
| SuiteSpot.h/cpp | ~400 | Plugin lifecycle and window management |
| SettingsUI.h/cpp | ~300 | Main settings interface |
| TrainingPackUI.h/cpp | ~800 | Training pack browser |
| TrainingPackManager.h/cpp | ~600 | Pack data and bag operations |
| LoadoutManager.h/cpp | ~200 | Car preset switching |
| SettingsSync.h/cpp | ~250 | CVar management |

---

## 2. Critical Bugs

### BUG-001: Raw Pointer Memory Management in onUnload

**Location**: `SuiteSpot.cpp` - `onUnload()` method

**Problem**: Managers are allocated with `new` but use raw pointers. The current cleanup in `onUnload()` uses manual `delete` which is error-prone and inconsistent with the `LoadoutManager` which properly uses `std::unique_ptr`.

**Current Code**:
```cpp
void SuiteSpot::onUnload() {
    delete settingsUI;
    settingsUI = nullptr;
    delete trainingPackUI;
    trainingPackUI = nullptr;
    delete loadoutUI;
    loadoutUI = nullptr;
    delete trainingPackMgr;
    trainingPackMgr = nullptr;
    delete autoLoadFeature;
    autoLoadFeature = nullptr;
    delete settingsSync;
    settingsSync = nullptr;
    delete mapManager;
    mapManager = nullptr;
}
```

**Fix Required**: Convert all manager pointers to `std::unique_ptr` as per CODING_STANDARDS.md:
```cpp
// In SuiteSpot.h - Change declarations to:
std::unique_ptr<MapManager> mapManager;
std::unique_ptr<SettingsSync> settingsSync;
std::unique_ptr<AutoLoadFeature> autoLoadFeature;
std::unique_ptr<TrainingPackManager> trainingPackMgr;
std::unique_ptr<SettingsUI> settingsUI;
std::unique_ptr<LoadoutUI> loadoutUI;

// In SuiteSpot.cpp onLoad() - Change allocations to:
mapManager = std::make_unique<MapManager>();
settingsSync = std::make_unique<SettingsSync>();
// ... etc

// In SuiteSpot.cpp onUnload() - Simply:
void SuiteSpot::onUnload() {
    // unique_ptrs automatically clean up
    LOG("SuiteSpot unloaded");
}
```

**Priority**: HIGH  
**Complexity**: MEDIUM  
**Estimated Hours**: 5  
**Risk**: Memory leaks on plugin unload/reload

---

### BUG-002: Potential Race Condition in TrainingPackUI State

**Location**: `TrainingPackUI.h/cpp`

**Problem**: Multiple state variables (`filteredPacks`, `selectedPackCode`, search filters) are modified during `Render()` which runs on the render thread, but could potentially be accessed if `TrainingPackManager` operations are scheduled via `gameWrapper->Execute()`.

**Current Issue**:
```cpp
// In TrainingPackUI - state modified without synchronization
std::vector<TrainingEntry> filteredPacks;  // No mutex protection
std::string selectedPackCode;              // No mutex protection
```

**Fix Required**: Add mutex protection for shared state that could be accessed from both threads:
```cpp
// In TrainingPackUI.h
private:
    mutable std::mutex uiStateMutex;
    std::vector<TrainingEntry> filteredPacks;
    
// In TrainingPackUI.cpp - When accessing filteredPacks from callbacks
void TrainingPackUI::SomeCallback() {
    std::lock_guard<std::mutex> lock(uiStateMutex);
    // Safe access
}
```

**Priority**: MEDIUM  
**Complexity**: LOW  
**Risk**: Potential crashes during rapid filtering/selection

---

### BUG-003: Inconsistent Null Checking Pattern

**Location**: Multiple files

**Problem**: Some code paths don't follow the null safety patterns documented in CODING_STANDARDS.md.

**Examples Found**:

1. In `SettingsUI.cpp`:
```cpp
// Potential issue - no null check before accessing trainingPackMgr
const auto& trainingPacks = plugin_->trainingPackMgr ? 
    plugin_->trainingPackMgr->GetPacks() : RLTraining;
// Good pattern, but inconsistently applied elsewhere
```

2. In `TrainingPackUI.cpp`:
```cpp
const auto* manager = plugin_->trainingPackMgr;
// Later code sometimes uses manager without checking
```

**Fix Required**: Audit all files and ensure consistent null checking:
```cpp
// Standard pattern to enforce
if (!plugin_) return;
if (!plugin_->trainingPackMgr) return;
// Now safe to use
```

**Priority**: MEDIUM  
**Complexity**: LOW  
**Files to Audit**: SettingsUI.cpp, TrainingPackUI.cpp, LoadoutUI.cpp

---

### BUG-004: Workshop Path Buffer Overflow Potential

**Location**: `SettingsUI.h`

**Problem**: Fixed-size buffer for workshop path:
```cpp
char workshopPathBuf[512] = {0};
```

Workshop paths on some systems (especially with long Steam library paths) can exceed 512 characters.

**Fix Required**: 
```cpp
// Option 1: Increase buffer size
char workshopPathBuf[1024] = {0};

// Option 2: Use std::string (preferred)
std::string workshopPathBuf;
// With ImGui::InputText using callback for std::string
```

**Priority**: LOW  
**Complexity**: LOW  
**Risk**: Truncated paths on systems with deep directory structures

---

## 3. Memory Management Issues

### MEM-001: Inefficient Vector Copying in Getters

**Location**: `TrainingPackManager.h/cpp`, `LoadoutManager.h/cpp`

**Problem**: Some getter methods return vectors by value, causing unnecessary copying:
```cpp
std::vector<std::string> GetLoadoutNames();  // Returns copy
```

**Fix Required**: Return const references where mutation isn't needed:
```cpp
const std::vector<std::string>& GetLoadoutNames() const;

// Or if thread safety is needed, return copy but document it:
/// @note Returns copy for thread safety
std::vector<std::string> GetLoadoutNamesCopy() const;
```

**Priority**: LOW  
**Complexity**: LOW  

---

### MEM-002: ImGuiListClipper Not Used Consistently

**Location**: `SettingsUI.cpp` freeplay maps dropdown

**Problem**: The freeplay maps combo uses `ImGuiListClipper` correctly, but other dropdowns with potentially large lists may not.

**Audit Required**: Check all `ImGui::BeginCombo()` usages to ensure large lists use clipper:
```cpp
// Pattern to apply everywhere with >20 items
ImGuiListClipper clipper;
clipper.Begin(static_cast<int>(items.size()));
while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
        // Render item
    }
}
```

**Priority**: MEDIUM  
**Complexity**: LOW  

---

## 4. UI/UX Improvements

### UX-001: Color Accessibility Issues

**Location**: `ConstantsUI.h`

**Problem**: Current color scheme may have accessibility issues:
- Green status text (0.0f, 1.0f, 0.0f) is very bright
- Low contrast in some difficulty level colors
- No consideration for color-blind users

**Current Colors**:
```cpp
inline const ImVec4 STATUS_ENABLED_TEXT_COLOR = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // Pure green
inline const ImVec4 STATUS_DISABLED_TEXT_COLOR = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Pure red
```

**Fix Required**: Implement WCAG 2.1 compliant colors:
```cpp
namespace UI {
    namespace Accessibility {
        // High contrast alternatives (4.5:1 ratio minimum)
        inline const ImVec4 SUCCESS_COLOR = ImVec4(0.2f, 0.8f, 0.4f, 1.0f);    // Softer green
        inline const ImVec4 ERROR_COLOR = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);      // Softer red
        inline const ImVec4 WARNING_COLOR = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);    // Amber
        inline const ImVec4 INFO_COLOR = ImVec4(0.4f, 0.7f, 0.9f, 1.0f);       // Blue
        
        // Color-blind friendly alternatives (distinguishable by shape/icon too)
        inline const ImVec4 CB_SUCCESS = ImVec4(0.0f, 0.6f, 0.5f, 1.0f);       // Teal
        inline const ImVec4 CB_ERROR = ImVec4(0.8f, 0.4f, 0.0f, 1.0f);         // Orange
    }
}
```

**Priority**: MEDIUM  
**Complexity**: LOW  

---

### UX-002: Missing Search/Filter in Plugin List

**Location**: `TrainingPackUI.cpp`

**Problem**: When users have 2000+ training packs, the current search only filters by name. Missing features:
- No quick filter buttons for common operations
- No saved filter presets
- No "recently used" section

**Enhancement Required**:
```cpp
// Add to TrainingPackUI.h
struct FilterPreset {
    std::string name;
    std::string searchText;
    std::string difficulty;
    std::string tag;
    int minShots;
    int maxShots;
    bool videoOnly;
};

std::vector<FilterPreset> savedFilters;
std::vector<std::string> recentlyUsedCodes;  // Last 10 used packs

// Add quick filter buttons
void RenderQuickFilters() {
    if (ImGui::Button("Bronze Only")) ApplyDifficultyFilter("Bronze");
    ImGui::SameLine();
    if (ImGui::Button("With Video")) { packVideoFilter = true; ApplyFilters(); }
    ImGui::SameLine();
    if (ImGui::Button("Recent")) ShowRecentlyUsed();
}
```

**Priority**: HIGH  
**Complexity**: MEDIUM  

---

### UX-003: Lack of Keyboard Navigation

**Location**: `TrainingPackUI.cpp`

**Problem**: Training pack browser requires mouse for all operations. No keyboard shortcuts.

**Enhancement Required**:
```cpp
// Add keyboard handling in Render()
void TrainingPackUI::HandleKeyboardInput() {
    if (!ImGui::IsWindowFocused()) return;
    
    // Arrow keys for navigation
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        SelectNextPack();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        SelectPreviousPack();
    }
    // Enter to load selected pack
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !selectedPackCode.empty()) {
        LoadSelectedPack();
    }
    // Ctrl+F to focus search
    if (ImGui::IsKeyPressed(ImGuiKey_F) && ImGui::GetIO().KeyCtrl) {
        ImGui::SetKeyboardFocusHere();  // Focus search box
    }
}
```

**Priority**: MEDIUM  
**Complexity**: MEDIUM  

---

### UX-004: Information Density Improvements

**Location**: `SettingsUI.cpp`, `TrainingPackUI.cpp`

**Problem**: Some sections feel cramped, others have too much empty space. Inconsistent visual hierarchy.

**Improvements Required**:

1. Add visual grouping with subtle backgrounds:
```cpp
// Helper function to add
void BeginGroupBox(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
    ImGui::BeginChild(label, ImVec2(0, 0), true, ImGuiWindowFlags_AutoResize);
    ImGui::TextColored(UI::HEADER_COLOR, "%s", label);
    ImGui::Separator();
}

void EndGroupBox() {
    ImGui::EndChild();
    ImGui::PopStyleColor();
}
```

2. Standardize spacing:
```cpp
// Add to ConstantsUI.h
namespace Spacing {
    constexpr float SECTION_GAP = 16.0f;
    constexpr float ITEM_GAP = 8.0f;
    constexpr float GROUP_PADDING = 12.0f;
}
```

**Priority**: LOW  
**Complexity**: MEDIUM  

---

### UX-005: Tooltips Lack Consistency

**Location**: Multiple UI files

**Problem**: Some controls have tooltips, others don't. Tooltip content varies in helpfulness.

**Fix Required**: Create tooltip constant file and ensure all interactive elements have tooltips:
```cpp
// New file: TooltipsUI.h
namespace UI {
namespace Tooltips {
    // Settings Tab
    constexpr const char* ENABLE_SUITESPOT = 
        "Enable or disable all SuiteSpot auto-loading features.\n"
        "When disabled, no automatic map loading or queuing occurs.";
    
    constexpr const char* MAP_TYPE_FREEPLAY = 
        "Load an official stadium map after matches.\n"
        "Good for casual free play practice.";
    
    constexpr const char* MAP_TYPE_TRAINING = 
        "Load a custom training pack after matches.\n"
        "Use Bag Rotation to cycle through multiple packs.";
    
    constexpr const char* BAG_ROTATION = 
        "Automatically cycle through packs in your bags.\n"
        "Packs are loaded sequentially across enabled bags.";
    
    // ... etc for all controls
}
}
```

**Priority**: LOW  
**Complexity**: LOW  

---

## 5. Performance Optimizations

### PERF-001: Filtering Optimization

**Location**: `TrainingPackUI.cpp`

**Problem**: Filtering recalculates on every frame when filters haven't changed.

**Current Pattern**:
```cpp
void Render() {
    // Filters applied every frame
    ApplyFilters();  // Expensive with 2000+ packs
}
```

**Fix Required**: Cache filter results and only recalculate when inputs change:
```cpp
// Add dirty flag pattern
bool filtersDirty = true;

void OnFilterChanged() {
    filtersDirty = true;
}

void Render() {
    if (filtersDirty) {
        ApplyFilters();
        filtersDirty = false;
    }
    // Use cached filteredPacks
}
```

**Priority**: HIGH  
**Complexity**: LOW  

---

### PERF-002: Column Width Calculation Optimization

**Location**: `TrainingPackUI.cpp`

**Problem**: `CalculateOptimalColumnWidths()` recalculates frequently.

**Current Flags**:
```cpp
bool columnWidthsDirty = true;
bool columnWidthsInitialized = false;
```

**Fix Required**: Only recalculate on significant changes:
```cpp
void Render() {
    float currentWidth = ImGui::GetWindowWidth();
    
    // Only recalculate if window width changed significantly (>10px)
    if (!columnWidthsInitialized || 
        std::abs(currentWidth - lastWindowWidth) > 10.0f) {
        CalculateOptimalColumnWidths();
        lastWindowWidth = currentWidth;
        columnWidthsInitialized = true;
    }
}
```

**Priority**: MEDIUM  
**Complexity**: LOW  

---

### PERF-003: Lazy Loading for Training Packs

**Location**: `TrainingPackManager.cpp`

**Problem**: All 2500+ packs are loaded into memory at startup.

**Enhancement Required**: Implement pagination/lazy loading:
```cpp
class TrainingPackManager {
    // Instead of loading all packs:
    // std::vector<TrainingEntry> allPacks;
    
    // Use lazy loading:
    std::vector<TrainingEntry> visiblePacks;  // Current view
    int totalPackCount;
    int currentPage = 0;
    int packsPerPage = 100;
    
    void LoadPage(int page);
    void EnsurePackLoaded(const std::string& code);
};
```

**Priority**: LOW (only if memory issues observed)  
**Complexity**: HIGH  

---

## 6. Code Quality Improvements

### CODE-001: Missing Error Handling in JSON Parsing

**Location**: `TrainingPackManager.cpp` (assumed)

**Problem**: JSON parsing for training packs may not handle malformed data gracefully.

**Fix Required**: Add comprehensive error handling:
```cpp
bool TrainingPackManager::LoadPacksFromFile(const std::filesystem::path& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG("Failed to open file: {}", path.string());
            return false;
        }
        
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(file);
        } catch (const nlohmann::json::parse_error& e) {
            LOG("JSON parse error at byte {}: {}", e.byte, e.what());
            return false;
        }
        
        for (const auto& item : j) {
            try {
                TrainingEntry entry;
                entry.code = item.value("code", "");
                entry.name = item.value("name", "Unknown");
                // ... etc with defaults for all fields
                
                if (entry.code.empty()) {
                    LOG("Skipping pack with empty code");
                    continue;
                }
                
                packs.push_back(std::move(entry));
            } catch (const std::exception& e) {
                LOG("Error parsing pack entry: {}", e.what());
                // Continue with next entry
            }
        }
        
        return !packs.empty();
    } catch (const std::exception& e) {
        LOG("Unexpected error loading packs: {}", e.what());
        return false;
    }
}
```

**Priority**: MEDIUM  
**Complexity**: LOW  

---

### CODE-002: Logging Inconsistency

**Location**: Multiple files

**Problem**: Inconsistent log message formatting and levels.

**Fix Required**: Create logging guidelines and apply consistently:
```cpp
// Logging conventions:
// LOG() for info level (normal operations)
// DEBUGLOG() for debug level (verbose)

// Format: "[Component] Action: details"
// Examples:
LOG("[TrainingPackManager] Loaded {} packs", count);
LOG("[SettingsUI] Map type changed to: {}", mapType);
LOG("[LoadoutManager] Switched to loadout: '{}'", name);

// For errors, include context:
LOG("[TrainingPackManager] Failed to load pack '{}': {}", code, error.what());
```

**Priority**: LOW  
**Complexity**: LOW  

---

### CODE-003: Magic Numbers

**Location**: Multiple files

**Problem**: Some hardcoded values aren't documented or centralized.

**Examples Found**:
```cpp
// In TrainingPackUI.cpp
ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

// In various validation
if (strlen(code) != 19) // XXXX-XXXX-XXXX-XXXX
```

**Fix Required**: Centralize in ConstantsUI.h:
```cpp
namespace UI {
namespace TrainingPackUI {
    // Window sizing
    constexpr float DEFAULT_WINDOW_WIDTH = 800.0f;
    constexpr float DEFAULT_WINDOW_HEIGHT = 600.0f;
    constexpr float MIN_WINDOW_WIDTH = 600.0f;
    constexpr float MIN_WINDOW_HEIGHT = 400.0f;
    
    // Pack code validation
    constexpr int PACK_CODE_LENGTH = 19;  // XXXX-XXXX-XXXX-XXXX
    constexpr int PACK_CODE_SEGMENT_LENGTH = 4;
}
}
```

**Priority**: LOW  
**Complexity**: LOW  

---

## 7. New Feature Specifications

### FEAT-001: Performance Dashboard

**Description**: Add a tab showing plugin performance metrics.

**Specification**:
```cpp
// New file: PerformanceUI.h
class PerformanceUI {
public:
    void Render();
    
private:
    struct Metrics {
        float lastRenderTime;
        float averageRenderTime;
        int packCount;
        int bagCount;
        size_t memoryUsage;
        int cvarCount;
    };
    
    Metrics currentMetrics;
    std::deque<float> renderTimeHistory;  // Last 120 frames
    
    void UpdateMetrics();
    void RenderMetricsPanel();
    void RenderRenderTimeGraph();
};
```

**UI Layout**:
```
┌─────────────────────────────────────┐
│ Performance Dashboard               │
├─────────────────────────────────────┤
│ Render Time: 0.8ms (avg: 0.6ms)     │
│ [████████░░░░░░░░░░░░] Graph        │
│                                     │
│ Training Packs: 2,347 loaded        │
│ Bags: 6 (4 enabled, 47 packs)       │
│ Memory: ~2.4 MB                     │
│                                     │
│ CVars Registered: 14                │
│ Hooks Active: 3                     │
└─────────────────────────────────────┘
```

**Priority**: LOW  
**Complexity**: MEDIUM  

---

### FEAT-002: Import/Export Configuration

**Description**: Allow users to export/import their settings and bags.

**Specification**:
```cpp
// New file: ConfigIO.h
class ConfigIO {
public:
    // Export all user configuration to JSON
    static bool ExportConfig(const std::filesystem::path& path,
                            const SettingsSync& settings,
                            const TrainingPackManager& packMgr);
    
    // Import configuration from JSON
    static bool ImportConfig(const std::filesystem::path& path,
                            SettingsSync& settings,
                            TrainingPackManager& packMgr);
    
private:
    static nlohmann::json SerializeBags(const TrainingPackManager& mgr);
    static nlohmann::json SerializeSettings(const SettingsSync& settings);
    static bool DeserializeBags(const nlohmann::json& j, TrainingPackManager& mgr);
};

// JSON format:
{
    "version": "1.0",
    "exportDate": "YYYY-MM-DDTHH:MM:SSZ",
    "settings": {
        "enabled": true,
        "mapType": 1,
        "autoQueue": true,
        "bagRotation": true,
        "delays": {
            "freeplay": 3,
            "training": 5,
            "workshop": 10,
            "queue": 0
        }
    },
    "bags": [
        {
            "name": "Warmup",
            "enabled": true,
            "color": [0.2, 0.6, 0.8, 1.0],
            "packs": ["XXXX-XXXX-XXXX-XXXX", ...]
        }
    ],
    "customPacks": [
        {
            "code": "...",
            "name": "...",
            "creator": "...",
            "difficulty": "Gold",
            "shotCount": 10
        }
    ]
}
```

**Priority**: MEDIUM  
**Complexity**: MEDIUM  

---

### FEAT-003: Loadout Auto-Switch on Map Type

**Description**: Automatically switch car loadout based on map type or training pack.

**Specification**:
```cpp
// New feature in SettingsSync
std::string freeplayLoadout;    // Loadout to use for freeplay
std::string trainingLoadout;    // Loadout to use for training
std::string workshopLoadout;    // Loadout to use for workshop

// Map-specific loadouts (optional)
std::unordered_map<std::string, std::string> packToLoadout;

// In AutoLoadFeature::OnMatchEnded
void OnMatchEnded(...) {
    // Determine target loadout
    std::string targetLoadout;
    
    if (mapType == 1 && !trainingLoadout.empty()) {
        // Check for pack-specific loadout first
        if (packToLoadout.count(selectedPack.code)) {
            targetLoadout = packToLoadout[selectedPack.code];
        } else {
            targetLoadout = trainingLoadout;
        }
    } else if (mapType == 0) {
        targetLoadout = freeplayLoadout;
    } else if (mapType == 2) {
        targetLoadout = workshopLoadout;
    }
    
    // Switch loadout before loading map
    if (!targetLoadout.empty()) {
        loadoutManager->SwitchLoadout(targetLoadout);
    }
    
    // Then load map...
}
```

**Priority**: LOW  
**Complexity**: MEDIUM  

---

## 8. Testing Requirements

### Test Cases for BUG-001 (Memory Management)

```cpp
// Test: Plugin loads and unloads without memory leaks
TEST_CASE("Plugin lifecycle") {
    // Requires: Valgrind or similar memory checker
    // 1. Load plugin
    // 2. Exercise all features
    // 3. Unload plugin
    // 4. Verify no leaked allocations
}
```

### Test Cases for BUG-002 (Thread Safety)

```cpp
// Test: Rapid filter changes don't crash
TEST_CASE("Concurrent filter changes") {
    // 1. Start render thread
    // 2. Rapidly change filters from multiple threads
    // 3. Verify no crashes or data corruption
}
```

### Test Cases for UX-002 (Search/Filter)

```
Manual Test: Search Performance
1. Load 2500+ training packs
2. Type in search box rapidly
3. Verify: No lag, results update smoothly
4. Verify: Filter combinations work correctly
```

---

## 9. Implementation Priority Matrix

| ID | Name | Priority | Complexity | Estimated Hours |
|----|------|----------|------------|-----------------|
| BUG-001 | Memory Management | HIGH | MEDIUM | 4-6 |
| BUG-002 | Thread Safety | MEDIUM | LOW | 2-3 |
| BUG-003 | Null Checking | MEDIUM | LOW | 2-4 |
| BUG-004 | Buffer Overflow | LOW | LOW | 1 |
| UX-001 | Color Accessibility | MEDIUM | LOW | 2-3 |
| UX-002 | Enhanced Filtering | HIGH | MEDIUM | 8-12 |
| UX-003 | Keyboard Navigation | MEDIUM | MEDIUM | 4-6 |
| UX-004 | Visual Hierarchy | LOW | MEDIUM | 6-8 |
| UX-005 | Consistent Tooltips | LOW | LOW | 2-3 |
| PERF-001 | Filter Caching | HIGH | LOW | 2-3 |
| PERF-002 | Column Width Opt | MEDIUM | LOW | 1-2 |
| CODE-001 | Error Handling | MEDIUM | LOW | 3-4 |
| CODE-002 | Logging | LOW | LOW | 2-3 |
| CODE-003 | Magic Numbers | LOW | LOW | 2-3 |
| FEAT-001 | Performance Dashboard | LOW | MEDIUM | 8-12 |
| FEAT-002 | Config Import/Export | MEDIUM | MEDIUM | 6-8 |
| FEAT-003 | Auto Loadout Switch | LOW | MEDIUM | 6-8 |

### Recommended Implementation Order

**Phase 1 - Critical Fixes (Week 1)**
1. BUG-001: Memory Management
2. PERF-001: Filter Caching
3. BUG-002: Thread Safety

**Phase 2 - UX Polish (Week 2)**
4. UX-001: Color Accessibility
5. UX-002: Enhanced Filtering
6. UX-005: Consistent Tooltips

**Phase 3 - Quality (Week 3)**
7. BUG-003: Null Checking Audit
8. CODE-001: Error Handling
9. PERF-002: Column Width Optimization

**Phase 4 - Features (Week 4+)**
10. UX-003: Keyboard Navigation
11. FEAT-002: Config Import/Export
12. Remaining items as time permits

---

## Appendix A: File Change Summary

| File | Changes Required |
|------|------------------|
| SuiteSpot.h | Convert raw pointers to unique_ptr |
| SuiteSpot.cpp | Update onLoad/onUnload for smart pointers |
| ConstantsUI.h | Add accessibility colors, spacing constants |
| TrainingPackUI.h | Add mutex, filter state, keyboard handling |
| TrainingPackUI.cpp | Implement caching, keyboard nav, improved filtering |
| SettingsUI.cpp | Add null checks, use new constants |
| HelpersUI.h/cpp | Add group box helpers |
| TooltipsUI.h | New file - centralized tooltip strings |
| ConfigIO.h/cpp | New file - import/export functionality |
| PerformanceUI.h/cpp | New file - performance dashboard |

---

## Appendix B: Build Verification Checklist

After implementing changes, verify:

- [ ] Plugin compiles without warnings
- [ ] Plugin loads in Rocket League
- [ ] F2 menu shows SuiteSpot tab
- [ ] Training Pack Browser opens correctly
- [ ] Bag rotation works after match
- [ ] Loadout switching functions
- [ ] No crashes on rapid interaction
- [ ] Settings persist across restarts
- [ ] Memory usage stable over time

---

*Document End*
