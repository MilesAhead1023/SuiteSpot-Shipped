#pragma once
#include "MapList.h"
#include <filesystem>
#include <set>
#include <vector>

// MapManager: Owns workshop discovery and file paths.
// Training pack management delegated to TrainingPackManager (unified JSON system).
// UI modules call through SuiteSpot; no ImGui or wrapper storage.
class MapManager {
public:
    // Path helpers
    std::filesystem::path GetDataRoot() const;
    std::filesystem::path GetSuiteTrainingDir() const;
    std::filesystem::path GetTrainingPacksPath() const;
    std::filesystem::path GetWorkshopLoaderConfigPath() const;
    std::filesystem::path ResolveConfiguredWorkshopRoot() const;

    void EnsureDataDirectories() const;

    // Workshop management
    void LoadWorkshopMaps(std::vector<WorkshopEntry>& workshop, int& currentWorkshopIndex);
    void DiscoverWorkshopInDir(const std::filesystem::path& dir, std::vector<WorkshopEntry>& workshop) const;

    // Random selection helper (works with shuffle bag from TrainingPackManager)
    int GetRandomTrainingMapIndex(const std::vector<TrainingEntry>& shuffleBag) const;
};