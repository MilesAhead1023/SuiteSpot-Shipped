#pragma once
#include "MapList.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class GameWrapper;

// PrejumpPackManager: Loads/scrapes pack data and provides filter/sort helpers.
// Consumed by PrejumpUI; no ImGui or wrapper storage.
class PrejumpPackManager {
public:
    void LoadPacksFromFile(const std::filesystem::path& filePath);
    bool IsCacheStale(const std::filesystem::path& filePath) const;
    std::string GetLastUpdatedTime(const std::filesystem::path& filePath) const;
    void ScrapeAndLoadPrejumpPacks(const std::filesystem::path& outputPath,
                                   const std::shared_ptr<GameWrapper>& gameWrapper);

    void FilterAndSortPacks(const std::string& searchText,
                            const std::string& difficultyFilter,
                            const std::string& tagFilter,
                            int minShots,
                            int sortColumn,
                            bool sortAscending,
                            std::vector<TrainingEntry>& out) const;
    void BuildAvailableTags(std::vector<std::string>& out) const;

    const std::vector<TrainingEntry>& GetPacks() const { return packs; }
    int GetPackCount() const { return packCount; }
    const std::string& GetLastUpdated() const { return lastUpdated; }
    bool IsScrapingInProgress() const { return scrapingInProgress; }

private:
    std::vector<TrainingEntry> packs;
    int packCount = 0;
    std::string lastUpdated = "";
    bool scrapingInProgress = false;
};
