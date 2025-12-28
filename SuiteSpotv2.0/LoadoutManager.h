#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <algorithm>

// LoadoutManager: Utility class for managing car loadout operations
// 
// Purpose: Encapsulates all LoadoutSaveWrapper operations for loadout switching
// and management. Provides a clean interface for the main plugin to interact
// with the BakkesMod loadout system without directly handling wrapper null
// checks and thread safety concerns.
//
// Design Principles:
// - Always get fresh wrapper references (never store)
// - Thread-safe operations using gameWrapper->Execute()
// - Null checks for all wrapper operations
// - Return bool for success/failure to enable error handling
//
// Usage Example:
//   LoadoutManager loadoutMgr(gameWrapper);
//   auto names = loadoutMgr.GetLoadoutNames();
//   bool success = loadoutMgr.SwitchLoadout("My Loadout");
//
// BakkesMod API Used:
// - GameWrapper::GetUserLoadoutSave() - Access LoadoutSaveWrapper
// - LoadoutSaveWrapper::GetPresets() - Get all saved loadouts
// - LoadoutSaveWrapper::EquipPreset() - Switch active loadout
// - LoadoutSetWrapper::GetName() - Get loadout preset name
//
// API Reference: See .github/instructions/context/items-cosmetics-api.instructions.md
class LoadoutManager
{
public:
    // Constructor: Takes gameWrapper for accessing BakkesMod API
    explicit LoadoutManager(std::shared_ptr<GameWrapper> gameWrapper);
    
    // Get all available loadout names
    // Returns: Vector of loadout name strings (empty if none available)
    // Note: This queries the LoadoutSaveWrapper for saved loadout presets
    std::vector<std::string> GetLoadoutNames();
    
    // Get the currently active loadout name
    // Returns: Current loadout name (empty string if not available)
    std::string GetCurrentLoadoutName();
    
    // Switch to a loadout by name
    // Parameters:
    //   loadoutName - Name of the loadout to switch to
    // Returns: true if successful, false otherwise
    // Note: Uses gameWrapper->Execute() for thread safety
    bool SwitchLoadout(const std::string& loadoutName);
    
    // Switch to a loadout by index
    // Parameters:
    //   index - Index in the loadout names list (0-based)
    // Returns: true if successful, false otherwise
    bool SwitchLoadout(int index);
    
    // Refresh the cached loadout list
    // Returns: true if refresh successful, false otherwise
    // Note: Call this after creating/deleting loadouts externally
    bool RefreshLoadoutCache();
    
    // Check if LoadoutManager is ready (initialized)
    // Returns: true if initialization completed, false otherwise
    bool IsReady() const;

private:
    std::shared_ptr<GameWrapper> gameWrapper_;
    
    // Cached loadout names (refreshed on demand)
    std::vector<std::string> cachedLoadoutNames_;
    
    // Thread safety
    mutable std::mutex cacheMutex_;
    std::atomic<bool> initialized_{false};
    
    // Internal helper: Query loadout names from LoadoutSaveWrapper
    // Populates cachedLoadoutNames_
    void QueryLoadoutNamesInternal();
};
