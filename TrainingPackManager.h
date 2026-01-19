#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "MapList.h"
#include "logging.h"
#include "IMGUI/json.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

/*
 * ======================================================================================
 * TRAINING PACK MANAGER: THE LIBRARY
 * ======================================================================================
 * 
 * WHAT IS THIS?
 * This is the librarian of the plugin. It keeps track of every training pack you have,
 * whether they came from the internet or you added them yourself.
 *
 * WHY IS IT HERE?
 * Managing 2000+ training packs requires a lot of organization. We need to save them
 * to a file, load them back up, search them, and handle the "Shuffle Bag" logic.
 *
 * HOW DOES IT WORK?
 * 1. `LoadPacksFromFile()`: Reads `training_packs.json` and turns it into a list of `TrainingEntry` objects.
 * 2. `UpdateTrainingPackList()`: Runs a PowerShell script to download the latest packs from the web.
 * 3. `FilterAndSortPacks()`: When you type in the search bar, this function decides which packs to show.
 * 4. `Categorized Bags`: Organize packs into categories (Defense, Offense, etc.) for structured training rotations.
 */

class TrainingPackManager
{
public:
    // Core data operations
    void LoadPacksFromFile(const std::filesystem::path& filePath);
    bool IsCacheStale(const std::filesystem::path& filePath) const;
    std::string GetLastUpdatedTime(const std::filesystem::path& filePath) const;
    
    // Downloads fresh data from online source
    void UpdateTrainingPackList(const std::filesystem::path& outputPath,
                                  const std::shared_ptr<GameWrapper>& gameWrapper);

    // Search and Sort logic
    void FilterAndSortPacks(const std::string& searchText,
                          const std::string& difficultyFilter,
                          const std::string& tagFilter,
                          int minShots,
                          bool videoFilter,
                          int sortColumn,
                          bool sortAscending,
                          std::vector<TrainingEntry>& out) const;

    // Helper for the UI tag filter
    void BuildAvailableTags(std::vector<std::string>& out) const;

    // Management (CRUD) operations
    bool AddCustomPack(const TrainingEntry& pack);
    bool UpdatePack(const std::string& code, const TrainingEntry& updatedPack);
    void HealPack(const std::string& code, int shots);
    bool DeletePack(const std::string& code);
    
    // Categorized Bag Logic
    const std::vector<TrainingBag>& GetAvailableBags() const { return availableBags; }
    const TrainingBag* GetBag(const std::string& bagName) const;
    std::vector<TrainingEntry> GetPacksInBag(const std::string& bagName) const;
    int GetBagPackCount(const std::string& bagName) const;

    // Pack-to-bag assignment
    void AddPackToBag(const std::string& code, const std::string& bagName);
    void AddPacksToBag(const std::vector<std::string>& codes, const std::string& bagName);
    void RemovePackFromBag(const std::string& code, const std::string& bagName);
    void RemovePackFromAllBags(const std::string& code);
    void ClearBag(const std::string& bagName);  // Remove all packs from a bag
    bool IsPackInBag(const std::string& code, const std::string& bagName) const;

    // Pack reordering within a bag
    void SwapPacksInBag(const std::string& bagName, int idx1, int idx2);

    // Bag management
    void SetBagEnabled(const std::string& bagName, bool enabled);
    bool CreateCustomBag(const std::string& name, const std::string& icon, const float color[4]);
    bool DeleteCustomBag(const std::string& bagName);

    // Rotation
    TrainingEntry GetNextFromRotation();
    std::string GetNextBagInRotation() const;

    // Accessors
    const std::vector<TrainingEntry>& GetPacks() const { return RLTraining; }
    int GetPackCount() const { return packCount; }
    std::string GetLastUpdated() const { return lastUpdated; }
    bool IsScrapingInProgress() const { return scrapingInProgress; }
    TrainingEntry* GetPackByCode(const std::string& code);
    void TestHealerFetch(std::shared_ptr<GameWrapper> gw, std::string code);

private:
    void SavePacksToFile(const std::filesystem::path& filePath);
    void InitializeDefaultBags();
    void MigrateShuffleBagToBagCategories();  // Migration from old inShuffleBag to new system

    std::vector<TrainingEntry> RLTraining;
    std::vector<TrainingBag> availableBags;
    mutable std::mutex packMutex;  // Protects RLTraining from concurrent access
    int packCount = 0;
    std::string lastUpdated = "Never";
    bool scrapingInProgress = false;
    std::filesystem::path currentFilePath;
    int currentRotationIndex = 0;  // Tracks which bag is next in rotation
};



