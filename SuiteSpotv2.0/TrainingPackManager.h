#pragma once
#include "MapList.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class GameWrapper;

// TrainingPackManager: Loads/scrapes pack data and provides filter/sort helpers.
// Unified system: handles both scraped (prejump) and custom packs in a single JSON file.
// Consumed by TrainingPackUI; no ImGui or wrapper storage.
class TrainingPackManager {
public:
    // File I/O
    void LoadPacksFromFile(const std::filesystem::path& filePath);
    void SavePacksToFile(const std::filesystem::path& filePath);
    bool IsCacheStale(const std::filesystem::path& filePath) const;
    std::string GetLastUpdatedTime(const std::filesystem::path& filePath) const;
    void ScrapeAndLoadTrainingPacks(const std::filesystem::path& outputPath,
                                   const std::shared_ptr<GameWrapper>& gameWrapper);

    // CRUD operations
    bool AddCustomPack(const TrainingEntry& pack);
    bool UpdatePack(const std::string& code, const TrainingEntry& updatedPack);
    bool DeletePack(const std::string& code);
    void ToggleShuffleBag(const std::string& code);

    // Filtering and sorting
    void FilterAndSortPacks(const std::string& searchText,
                            const std::string& difficultyFilter,
                            const std::string& tagFilter,
                            int minShots,
                            int sortColumn,
                            bool sortAscending,
                            std::vector<TrainingEntry>& out) const;
    void BuildAvailableTags(std::vector<std::string>& out) const;

    // Accessors
    const std::vector<TrainingEntry>& GetPacks() const { return RLTraining; }
    std::vector<TrainingEntry> GetShuffleBagPacks() const;
    TrainingEntry* GetPackByCode(const std::string& code);
    int GetPackCount() const { return packCount; }
    int GetShuffleBagCount() const;
    const std::string& GetLastUpdated() const { return lastUpdated; }
    bool IsScrapingInProgress() const { return scrapingInProgress; }

    // Store the file path for auto-save
    void SetFilePath(const std::filesystem::path& path) { currentFilePath = path; }
    const std::filesystem::path& GetFilePath() const { return currentFilePath; }

private:
    std::filesystem::path currentFilePath;
    int packCount = 0;
    std::string lastUpdated = "";
    bool scrapingInProgress = false;
};