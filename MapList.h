#pragma once
#include <string>
#include <vector>
#include <set>

// Freeplay maps
struct MapEntry {
    std::string code;
    std::string name;
};
extern std::vector<MapEntry> RLMaps;

// Training bag categories for organized pack rotation
struct TrainingBag {
    std::string name;                       // Internal name: "Defense", "Offense", etc.
    std::string displayName;                // Display name with icon: "🛡️ Defense"
    std::string icon;                       // Icon only: "🛡️"
    std::vector<std::string> autoTags;      // Tags that auto-add packs to this bag
    bool enabled = true;                    // Include in rotation
    int priority = 0;                       // Rotation order (lower = first)
    bool isUserCreated = false;             // true for custom bags
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};  // Badge color (RGBA)
};

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
    std::set<std::string> bagCategories;  // Categorized bag membership (e.g., "Defense", "Offense")
    bool isModified = false;        // Track if user edited a scraped pack
};
extern std::vector<TrainingEntry> RLTraining;

// Workshop maps
struct WorkshopEntry {
    std::string filePath;
    std::string name;      
};
extern std::vector<WorkshopEntry> RLWorkshop;