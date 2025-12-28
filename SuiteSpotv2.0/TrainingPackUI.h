#pragma once
#include "IMGUI/imgui.h"
#include "MapList.h"
#include <string>
#include <vector>

class SuiteSpot;

class TrainingPackUI {
public:
    explicit TrainingPackUI(SuiteSpot* plugin);
    void RenderTrainingPackTab();

private:
    SuiteSpot* plugin_;

    char packSearchText[256] = {0};
    std::string packDifficultyFilter = "All";
    std::string packTagFilter = "";
    int packMinShots = 0;
    int packMaxShots = 100;
    int packSortColumn = 0;
    bool packSortAscending = true;

    char lastSearchText[256] = {0};
    std::string lastDifficultyFilter = "All";
    std::string lastTagFilter = "";
    int lastMinShots = 0;
    int lastSortColumn = 0;
    bool lastSortAscending = true;

    std::vector<std::string> availableTags;
    bool tagsInitialized = false;
    int lastPackCount = 0;
    std::vector<TrainingEntry> filteredPacks;

    // Custom pack form state
    char customPackCode[20] = {0};       // XXXX-XXXX-XXXX-XXXX format
    char customPackName[128] = {0};
    char customPackCreator[64] = {0};
    int customPackDifficulty = 0;        // Index into difficulties array
    int customPackShotCount = 10;
    char customPackTags[256] = {0};      // Comma-separated
    char customPackNotes[512] = {0};
    char customPackVideoUrl[256] = {0};
    std::string customPackError;
    bool customPackSuccess = false;

    // Helper methods
    void RenderCustomPackForm();
    bool ValidatePackCode(const char* code) const;
    void ClearCustomPackForm();
};