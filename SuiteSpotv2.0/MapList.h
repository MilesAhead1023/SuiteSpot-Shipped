#pragma once
#include <string>
#include <vector>

// Freeplay maps
struct MapEntry {
    std::string code;
    std::string name;
};
extern std::vector<MapEntry> RLMaps;

// Training packs
struct TrainingEntry {
    std::string code;
    std::string name;

    // Pack metadata
    std::string creator;            // Creator's display name
    std::string creatorSlug;        // Creator's username (for linking)
    std::string difficulty;         // Bronze, Gold, Platinum, Diamond, Champion, Supersonic Legend
    std::vector<std::string> tags;  // Array of tags
    int shotCount = 0;              // Number of shots
    std::string staffComments;      // Staff description
    std::string notes;              // Creator's notes
    std::string videoUrl;           // Optional YouTube link
    int likes = 0;                  // Engagement metric
    int plays = 0;                  // Engagement metric
    int status = 1;                 // Pack status (1 = active)

    // Unified system fields
    std::string source = "prejump"; // "prejump" or "custom"
    bool inShuffleBag = false;      // Shuffle bag membership
    bool isModified = false;        // Track if user edited a scraped pack
};
extern std::vector<TrainingEntry> RLTraining;

// Workshop maps
struct WorkshopEntry {
    std::string filePath;
    std::string name;      
};
extern std::vector<WorkshopEntry> RLWorkshop;