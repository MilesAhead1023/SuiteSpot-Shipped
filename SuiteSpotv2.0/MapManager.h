#pragma once
#include "MapList.h"
#include <filesystem>
#include <set>
#include <vector>

// MapManager: Owns map persistence, shuffle bag, and workshop discovery.
// UI modules call through SuiteSpot; no ImGui or wrapper storage.
class MapManager {
public:
    std::filesystem::path GetDataRoot() const;
    std::filesystem::path GetSuiteTrainingDir() const;
    std::filesystem::path GetTrainingFilePath() const;
    std::filesystem::path GetShuffleBagPath() const;
    std::filesystem::path GetWorkshopLoaderConfigPath() const;
    std::filesystem::path ResolveConfiguredWorkshopRoot() const;

    void EnsureDataDirectories() const;
    void EnsureReadmeFiles() const;

    void LoadTrainingMaps(std::vector<TrainingEntry>& training, int& currentTrainingIndex);
    void SaveTrainingMaps(const std::vector<TrainingEntry>& training) const;
    void LoadWorkshopMaps(std::vector<WorkshopEntry>& workshop, int& currentWorkshopIndex);
    void DiscoverWorkshopInDir(const std::filesystem::path& dir, std::vector<WorkshopEntry>& workshop) const;
    void LoadShuffleBag(std::vector<TrainingEntry>& shuffleBag,
                        std::set<int>& selectedTrainingIndices,
                        const std::vector<TrainingEntry>& training) const;
    void SaveShuffleBag(const std::vector<TrainingEntry>& shuffleBag) const;

    int GetRandomTrainingMap(const std::vector<TrainingEntry>& training,
                             const std::vector<TrainingEntry>& shuffleBag) const;

private:
    void MirrorDirectory(const std::filesystem::path& src, const std::filesystem::path& dst) const;
};
