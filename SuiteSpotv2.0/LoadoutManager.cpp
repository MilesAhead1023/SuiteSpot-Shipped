#include "pch.h"
#include "LoadoutManager.h"
#include "logging.h"

// LoadoutManager Implementation
// 
// Purpose: Encapsulates BakkesMod LoadoutSaveWrapper operations for car loadout
// switching and management. Provides thread-safe interface using gameWrapper->Execute()
// to ensure all wrapper access happens on the game thread.
//
// BakkesMod API Flow:
//   GameWrapper::GetUserLoadoutSave() -> LoadoutSaveWrapper
//     -> GetPresets() -> ArrayWrapper<LoadoutSetWrapper>
//     -> GetEquippedLoadout() -> LoadoutSetWrapper
//     -> EquipPreset(preset) -> void (switches loadout)
//
// Thread Safety: All public methods use gameWrapper->Execute() to wrap BakkesMod
// API calls, ensuring they execute on the game thread regardless of caller context.
//
// Error Handling: All wrapper operations are null-checked. Methods return bool
// for success/failure to enable caller error handling. Failures are logged.
//
// Design Pattern: Never store wrapper references - always get fresh references
// within Execute() blocks to avoid using invalid/stale wrappers.

LoadoutManager::LoadoutManager(std::shared_ptr<GameWrapper> gameWrapper)
    : gameWrapper_(gameWrapper)
{
    // Use deferred initialization to ensure game state is ready
    if (gameWrapper_) {
        gameWrapper_->SetTimeout([this](GameWrapper* gw) {
            QueryLoadoutNamesInternal();
            initialized_.store(true);
            // Thread-safe access to cache size for logging
            size_t cacheSize;
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                cacheSize = cachedLoadoutNames_.size();
            }
            LOG("[LoadoutManager] Initialization complete, found {} loadout(s)", cacheSize);
        }, 0.5f); // Small delay to ensure BakkesMod is fully loaded
    } else {
        LOG("[LoadoutManager] ERROR: GameWrapper is null during construction");
    }
}

void LoadoutManager::QueryLoadoutNamesInternal()
{
    // Internal helper that queries LoadoutSaveWrapper for all loadout preset names
    // Populates cachedLoadoutNames_ for use by GetLoadoutNames() and SwitchLoadout(index)
    
    if (!gameWrapper_) {
        LOG("[LoadoutManager] Cannot query loadouts: GameWrapper is null");
        return;
    }

    // Execute on game thread for thread-safe wrapper access
    gameWrapper_->Execute([this](GameWrapper* gw) {
        std::vector<std::string> tempNames;
        
        try {
            auto loadoutSave = gw->GetUserLoadoutSave();
            if (loadoutSave.IsNull()) {
                LOG("[LoadoutManager] GetUserLoadoutSave() returned null");
                return;
            }

            auto presets = loadoutSave.GetPresets();
            if (presets.IsNull()) {
                LOG("[LoadoutManager] GetPresets() returned null");
                return;
            }

            // Iterate through presets and extract names
            int presetCount = presets.Count();
            LOG("[LoadoutManager] Found {} preset(s)", presetCount);
            
            // Reserve capacity to avoid reallocations
            tempNames.reserve(presetCount);
            
            for (int i = 0; i < presetCount; ++i) {
                auto preset = presets.Get(i);
                if (!preset.IsNull()) {
                    std::string name = preset.GetName();
                    if (!name.empty()) {
                        tempNames.push_back(name);
                    } else {
                        LOG("[LoadoutManager] Preset at index {} has empty name", i);
                    }
                } else {
                    LOG("[LoadoutManager] Preset at index {} is null", i);
                }
            }
            
            // Update cache with thread safety (inside Execute to ensure completion)
            {
                std::lock_guard<std::mutex> lock(cacheMutex_);
                cachedLoadoutNames_ = std::move(tempNames);
            }
        }
        catch (const std::exception& e) {
            LOG("[LoadoutManager] Exception in QueryLoadoutNamesInternal: {}", e.what());
        }
        catch (...) {
            LOG("[LoadoutManager] Unknown exception in QueryLoadoutNamesInternal");
        }
    });
}

std::vector<std::string> LoadoutManager::GetLoadoutNames()
{
    // Thread-safe access to cached loadout names
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    // Check initialization status under the same lock for consistency
    if (cachedLoadoutNames_.empty()) {
        bool isInitialized = initialized_.load();
        if (!isInitialized) {
            LOG("[LoadoutManager] GetLoadoutNames called before initialization complete");
        }
    }
    
    return cachedLoadoutNames_;
}

std::string LoadoutManager::GetCurrentLoadoutName()
{
    // Safely queries BakkesMod for the currently equipped loadout name
    // Returns empty string if unable to determine current loadout or if no preset is active

    if (!gameWrapper_) {
        LOG("[LoadoutManager] GetCurrentLoadoutName: GameWrapper is null");
        return "";
    }

    std::string currentName;

    // Execute on game thread for thread-safe wrapper access
    // IMPORTANT: Capture currentName by value through a shared_ptr to avoid reference issues
    auto result = std::make_shared<std::string>();

    gameWrapper_->Execute([this, result](GameWrapper* gw) {
        try {
            auto loadoutSave = gw->GetUserLoadoutSave();
            if (loadoutSave.IsNull()) {
                LOG("[LoadoutManager] GetCurrentLoadoutName: GetUserLoadoutSave() returned null");
                return;
            }

            // GetEquippedLoadout() returns a LoadoutSetWrapper with the currently active preset
            auto equippedLoadout = loadoutSave.GetEquippedLoadout();
            if (equippedLoadout.IsNull()) {
                LOG("[LoadoutManager] GetCurrentLoadoutName: GetEquippedLoadout() returned null");
                return;
            }

            // Get the name of the equipped loadout preset
            std::string equippedName = equippedLoadout.GetName();
            if (equippedName.empty()) {
                LOG("[LoadoutManager] GetCurrentLoadoutName: Equipped loadout has empty name");
                return;
            }

            // Store result in shared_ptr (safe across thread boundaries)
            *result = equippedName;
        }
        catch (const std::exception& e) {
            LOG("[LoadoutManager] Exception in GetCurrentLoadoutName: {}", e.what());
        }
        catch (...) {
            LOG("[LoadoutManager] Unknown exception in GetCurrentLoadoutName");
        }
    });

    return *result;
}

bool LoadoutManager::SwitchLoadout(const std::string& loadoutName)
{
    if (!gameWrapper_) {
        LOG("[LoadoutManager] SwitchLoadout: GameWrapper is null");
        return false;
    }
    
    if (loadoutName.empty()) {
        LOG("[LoadoutManager] SwitchLoadout: Loadout name is empty");
        return false;
    }

    bool success = false;

    // Capture by value to prevent reference issues
    gameWrapper_->Execute([this, loadoutName, &success](GameWrapper* gw) {
        try {
            auto loadoutSave = gw->GetUserLoadoutSave();
            if (loadoutSave.IsNull()) {
                LOG("[LoadoutManager] SwitchLoadout: GetUserLoadoutSave() returned null");
                return;
            }

            auto presets = loadoutSave.GetPresets();
            if (presets.IsNull()) {
                LOG("[LoadoutManager] SwitchLoadout: GetPresets() returned null");
                return;
            }

            // Search for matching preset by name
            bool found = false;
            for (int i = 0; i < presets.Count(); ++i) {
                auto preset = presets.Get(i);
                if (!preset.IsNull()) {
                    std::string presetName = preset.GetName();
                    if (presetName == loadoutName) {
                        loadoutSave.EquipPreset(preset);
                        success = true;
                        found = true;
                        LOG("[LoadoutManager] Successfully switched to loadout: '{}'", loadoutName);
                        return;
                    }
                }
            }
            
            if (!found) {
                LOG("[LoadoutManager] SwitchLoadout: Loadout '{}' not found in presets", loadoutName);
            }
        }
        catch (const std::exception& e) {
            LOG("[LoadoutManager] Exception in SwitchLoadout: {}", e.what());
        }
        catch (...) {
            LOG("[LoadoutManager] Unknown exception in SwitchLoadout");
        }
    });

    return success;
}

bool LoadoutManager::SwitchLoadout(int index)
{
    // Switches to the loadout at the specified index in cachedLoadoutNames_
    // Returns true if successful, false otherwise
    // Index must be valid (0 <= index < cachedLoadoutNames_.size())
    
    std::string loadoutName;
    
    // Thread-safe access to cache
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        
        if (index < 0 || index >= static_cast<int>(cachedLoadoutNames_.size())) {
            LOG("[LoadoutManager] Invalid loadout index: {} (cache size: {})", index, cachedLoadoutNames_.size());
            return false;
        }
        
        loadoutName = cachedLoadoutNames_[index];
    }

    // Use the by-name method with the cached name
    return SwitchLoadout(loadoutName);
}

bool LoadoutManager::RefreshLoadoutCache()
{
    // Refreshes the cached loadout list by re-querying LoadoutSaveWrapper
    // Call this after creating or deleting loadouts externally
    // Returns true if refresh successful (at least able to query), false otherwise
    
    if (!gameWrapper_) {
        LOG("[LoadoutManager] RefreshLoadoutCache: GameWrapper is null");
        return false;
    }

    LOG("[LoadoutManager] Refreshing loadout cache...");
    
    // Re-query the loadout names
    QueryLoadoutNamesInternal();
    
    // Log the results (capture size outside mutex for minimal lock time)
    size_t cacheSize;
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cacheSize = cachedLoadoutNames_.size();
    }
    LOG("[LoadoutManager] Cache refresh complete, found {} loadout(s)", cacheSize);
    
    // Success if we have a valid gameWrapper (QueryLoadoutNamesInternal logs its own errors)
    return true;
}

bool LoadoutManager::IsReady() const
{
    return initialized_.load();
}
