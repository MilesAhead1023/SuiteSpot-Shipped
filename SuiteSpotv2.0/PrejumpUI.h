#pragma once
#include "IMGUI/imgui.h"
#include "MapList.h"
#include <string>
#include <vector>

class SuiteSpot;

class PrejumpUI {
public:
    explicit PrejumpUI(SuiteSpot* plugin);
    void RenderPrejumpTab();

private:
    SuiteSpot* plugin_;

    char prejumpSearchText[256] = {0};
    std::string prejumpDifficultyFilter = "All";
    std::string prejumpTagFilter = "";
    int prejumpMinShots = 0;
    int prejumpMaxShots = 100;
    int prejumpSortColumn = 0;
    bool prejumpSortAscending = true;

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
};
