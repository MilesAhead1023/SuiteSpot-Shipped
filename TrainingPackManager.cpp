#include "bakkesmod/wrappers/gfx/GfxDataTrainingWrapper.h"
#include "bakkesmod/wrappers/GameEvent/SaveData/TrainingEditorSaveDataWrapper.h"
#include "pch.h"
#include "TrainingPackManager.h"
#include "EmbeddedPackGrabber.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include <set>
#include <sstream>
#include <thread>

void TrainingPackManager::LoadPacksFromFile(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath)) {
        LOG("SuiteSpot: Pack cache file not found: {}", filePath.string());
        {
            std::lock_guard<std::mutex> lock(packMutex);
            RLTraining.clear();
            packCount = 0;
        }
        lastUpdated = "Never";
        return;
    }

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG("SuiteSpot: Failed to open Pack cache file");
            return;
        }

        nlohmann::json jsonData;
        file >> jsonData;
        file.close();

        std::lock_guard<std::mutex> lock(packMutex);
        RLTraining.clear();

        if (!jsonData.contains("packs") || !jsonData["packs"].is_array()) {
            LOG("SuiteSpot: Invalid Pack cache file format - missing 'packs' array");
            return;
        }

        for (const auto& pack : jsonData["packs"]) {
            TrainingEntry entry;

            if (pack.contains("code") && pack["code"].is_string()) {
                entry.code = pack["code"].get<std::string>();
            }
            if (pack.contains("name") && pack["name"].is_string()) {
                entry.name = pack["name"].get<std::string>();
            }

            if (entry.code.empty() || entry.name.empty()) {
                continue;
            }

            if (pack.contains("creator") && pack["creator"].is_string()) {
                entry.creator = pack["creator"].get<std::string>();
            }
            if (pack.contains("creatorSlug") && pack["creatorSlug"].is_string()) {
                entry.creatorSlug = pack["creatorSlug"].get<std::string>();
            }
            if (pack.contains("difficulty") && pack["difficulty"].is_string()) {
                entry.difficulty = pack["difficulty"].get<std::string>();
            }
            if (pack.contains("shotCount") && pack["shotCount"].is_number()) {
                entry.shotCount = pack["shotCount"].get<int>();
            }
            if (pack.contains("staffComments") && pack["staffComments"].is_string()) {
                entry.staffComments = pack["staffComments"].get<std::string>();
            }
            if (pack.contains("notes") && pack["notes"].is_string()) {
                entry.notes = pack["notes"].get<std::string>();
            }
            if (pack.contains("videoUrl") && pack["videoUrl"].is_string()) {
                entry.videoUrl = pack["videoUrl"].get<std::string>();
            }
            if (pack.contains("likes") && pack["likes"].is_number()) {
                entry.likes = pack["likes"].get<int>();
            }
            if (pack.contains("plays") && pack["plays"].is_number()) {
                entry.plays = pack["plays"].get<int>();
            }
            if (pack.contains("status") && pack["status"].is_number()) {
                entry.status = pack["status"].get<int>();
            }

            if (pack.contains("tags") && pack["tags"].is_array()) {
                for (const auto& tag : pack["tags"]) {
                    if (tag.is_string()) {
                        entry.tags.push_back(tag.get<std::string>());
                    }
                }
            }

            // Unified system fields
            if (pack.contains("source") && pack["source"].is_string()) {
                entry.source = pack["source"].get<std::string>();
            } else {
                entry.source = "prejump"; // Default for backward compatibility
            }
            // Handle bagCategories (new system) or migrate from inShuffleBag (legacy)
            if (pack.contains("bagCategories") && pack["bagCategories"].is_array()) {
                for (const auto& cat : pack["bagCategories"]) {
                    if (cat.is_string()) {
                        entry.bagCategories.insert(cat.get<std::string>());
                    }
                }
            } else if (pack.contains("inShuffleBag") && pack["inShuffleBag"].is_boolean() && pack["inShuffleBag"].get<bool>()) {
                // Legacy migration: packs marked for shuffle go to "Warmup" by default
                entry.bagCategories.insert("Warmup");
            }
            // Load orderInBag (position within each bag)
            if (pack.contains("orderInBag") && pack["orderInBag"].is_object()) {
                for (auto& [bagName, order] : pack["orderInBag"].items()) {
                    if (order.is_number_integer()) {
                        entry.orderInBag[bagName] = order.get<int>();
                    }
                }
            } else {
                // Migration: If orderInBag missing, auto-assign based on load order
                int orderCounter = 0;
                for (const auto& bagName : entry.bagCategories) {
                    entry.orderInBag[bagName] = orderCounter++;
                }
            }
            if (pack.contains("isModified") && pack["isModified"].is_boolean()) {
                entry.isModified = pack["isModified"].get<bool>();
            }

            RLTraining.push_back(entry);
        }

        // Sort RLTraining alphabetically by name
        std::sort(RLTraining.begin(), RLTraining.end(), [](const TrainingEntry& a, const TrainingEntry& b) {
            // Case-insensitive comparison for names
            std::string nameA = a.name;
            std::string nameB = b.name;
            std::transform(nameA.begin(), nameA.end(), nameA.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(nameB.begin(), nameB.end(), nameB.begin(), [](unsigned char c) { return std::tolower(c); });
            return nameA < nameB;
        });

        packCount = static_cast<int>(RLTraining.size());
        lastUpdated = GetLastUpdatedTime(filePath);
        currentFilePath = filePath;

        // Initialize default bags if not already done
        if (availableBags.empty()) {
            InitializeDefaultBags();
        }

        LOG("SuiteSpot: Loaded {} training packs from file", packCount);

    } catch (const std::exception& e) {
        LOG("SuiteSpot: Error loading training packs: {}", std::string(e.what()));
        {
            std::lock_guard<std::mutex> lock(packMutex);
            RLTraining.clear();
            packCount = 0;
        }
    }
}

bool TrainingPackManager::IsCacheStale(const std::filesystem::path& filePath) const
{
    if (!std::filesystem::exists(filePath)) {
        return true;
    }

    try {
        auto lastWriteTime = std::filesystem::last_write_time(filePath);
        auto now = std::filesystem::file_time_type::clock::now();
        auto age = std::chrono::duration_cast<std::chrono::hours>(now - lastWriteTime);

        return age.count() > 168;
    } catch (...) {
        return true;
    }
}

std::string TrainingPackManager::GetLastUpdatedTime(const std::filesystem::path& filePath) const
{
    if (!std::filesystem::exists(filePath)) {
        return "Never";
    }

    try {
        auto lastWriteTime = std::filesystem::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastWriteTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        auto tt = std::chrono::system_clock::to_time_t(sctp);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M UTC");
        return oss.str();
    } catch (...) {
        return "Unknown";
    }
}

void TrainingPackManager::UpdateTrainingPackList(const std::filesystem::path& outputPath,
                                                   const std::shared_ptr<GameWrapper>& gameWrapper)
{
    if (scrapingInProgress) {
        LOG("SuiteSpot: Training pack update already in progress");
        return;
    }

    if (!gameWrapper) {
        LOG("SuiteSpot: GameWrapper unavailable for training pack update");
        return;
    }

    scrapingInProgress = true;

    LOG("SuiteSpot: Training pack updater starting");
    LOG("SuiteSpot: Output path: {}", outputPath.string());

    // Launch update in background thread to avoid blocking game thread
    std::thread updateThread([this, outputPath]() {
        try {
            // Get temp directory for script extraction
            std::filesystem::path tempDir = std::filesystem::temp_directory_path();
            auto tempScript = tempDir / "SuitePackGrabber_temp.ps1";
            auto logFile = outputPath.parent_path() / "PackGrabber.log";

            // Write embedded script to temp file
            {
                std::ofstream tempFile(tempScript, std::ios::binary);
                if (!tempFile.is_open()) {
                    LOG("SuiteSpot: Failed to create temp script file: {}", tempScript.string());
                    scrapingInProgress = false;
                    return;
                }
                tempFile.write(EMBEDDED_PACK_GRABBER_SCRIPT,
                              std::string_view(EMBEDDED_PACK_GRABBER_SCRIPT).length());
                tempFile.close();
            }

            std::string outStr = outputPath.string();

            // Use cmd.exe to launch PowerShell and capture output to log file
            std::string cmd = "cmd.exe /c powershell.exe -NoProfile -ExecutionPolicy Bypass -File \""
                            + tempScript.string() + "\" -OutputPath \"" + outStr + "\" > \"" + logFile.string() + "\" 2>&1";

            LOG("SuiteSpot: Training pack updater started");

            int result = system(cmd.c_str());

            LOG("SuiteSpot: Training pack updater returned: {}", result);

            // Try to read error log if it exists
            if (std::filesystem::exists(logFile)) {
                std::ifstream log(logFile);
                std::string line;
                LOG("SuiteSpot: PackGrabber output:");
                while (std::getline(log, line)) {
                    LOG("  {}", line);
                }
            }

            if (result == 0) {
                LOG("SuiteSpot: Training pack update completed successfully");
                LoadPacksFromFile(outputPath);
                lastUpdated = GetLastUpdatedTime(outputPath);
            } else {
                LOG("SuiteSpot: Training pack updater returned non-zero exit code: {}", result);
            }

            // Clean up temp script
            try {
                if (std::filesystem::exists(tempScript)) {
                    std::filesystem::remove(tempScript);
                    LOG("SuiteSpot: Cleaned up temporary script file");
                }
            } catch (const std::exception& e) {
                LOG("SuiteSpot: Warning - failed to clean up temp script: {}", std::string(e.what()));
            }

        } catch (const std::exception& e) {
            LOG("SuiteSpot: Training pack updater error: {}", std::string(e.what()));
        }

        scrapingInProgress = false;
    });

    updateThread.detach();  // Let thread run independently
}

void TrainingPackManager::FilterAndSortPacks(const std::string& searchText,
                                            const std::string& difficultyFilter,
                                            const std::string& tagFilter,
                                            int minShots,
                                            bool videoFilter,
                                            int sortColumn,
                                            bool sortAscending,
                                            std::vector<TrainingEntry>& out) const
{
    std::lock_guard<std::mutex> lock(packMutex);
    out.clear();

    std::string searchLower(searchText);
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (const auto& pack : RLTraining) {
        // Video filter - skip packs without video if filter is enabled
        if (videoFilter && pack.videoUrl.empty()) {
            continue;
        }

        if (!searchLower.empty()) {
            std::string nameLower = pack.name;
            std::string creatorLower = pack.creator;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(creatorLower.begin(), creatorLower.end(), creatorLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            bool matchesSearch = false;
            if (nameLower.find(searchLower) != std::string::npos) matchesSearch = true;
            if (creatorLower.find(searchLower) != std::string::npos) matchesSearch = true;

            for (const auto& tag : pack.tags) {
                std::string tagLower = tag;
                std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (tagLower.find(searchLower) != std::string::npos) {
                    matchesSearch = true;
                    break;
                }
            }

            if (!matchesSearch) {
                continue;
            }
        }

        if (difficultyFilter != "All") {
            if (difficultyFilter == "Unranked") {
                // Match "Unranked" filter against all "no difficulty" values
                if (!pack.difficulty.empty() && pack.difficulty != "Unknown" && pack.difficulty != "All" && pack.difficulty != "Unranked") {
                    continue;
                }
            } else {
                // Standard strict match for ranked tiers
                if (pack.difficulty != difficultyFilter) {
                    continue;
                }
            }
        }

        if (!tagFilter.empty()) {
            bool hasTag = false;
            for (const auto& tag : pack.tags) {
                if (tag == tagFilter) {
                    hasTag = true;
                    break;
                }
            }
            if (!hasTag) continue;
        }

        if (pack.shotCount < minShots) {
            continue;
        }

        out.push_back(pack);
    }

    // Case-insensitive string comparison helper
    auto caseInsensitiveCompare = [](const std::string& a, const std::string& b) -> int {
        std::string lowerA = a;
        std::string lowerB = b;
        std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), [](unsigned char c) { return std::tolower(c); });
        return lowerA.compare(lowerB);
    };

    std::sort(out.begin(), out.end(), [sortColumn, sortAscending, &caseInsensitiveCompare](const TrainingEntry& a, const TrainingEntry& b) {
        int cmp = 0;
        switch (sortColumn) {
            case 0: // Name
                cmp = caseInsensitiveCompare(a.name, b.name);
                break;
            case 1: // Creator
                cmp = caseInsensitiveCompare(a.creator, b.creator);
                break;
            case 2: // Difficulty
                cmp = caseInsensitiveCompare(a.difficulty, b.difficulty);
                break;
            case 3: // Shots
                cmp = (a.shotCount < b.shotCount) ? -1 : (a.shotCount > b.shotCount) ? 1 : 0;
                break;
            case 4: // Likes
                cmp = (a.likes < b.likes) ? -1 : (a.likes > b.likes) ? 1 : 0;
                break;
            case 5: // Plays
                cmp = (a.plays < b.plays) ? -1 : (a.plays > b.plays) ? 1 : 0;
                break;
        }
        return sortAscending ? (cmp < 0) : (cmp > 0);
    });
}

void TrainingPackManager::BuildAvailableTags(std::vector<std::string>& out) const
{
    std::lock_guard<std::mutex> lock(packMutex);
    std::set<std::string> uniqueTags;
    for (const auto& pack : RLTraining) {
        for (const auto& tag : pack.tags) {
            uniqueTags.insert(tag);
        }
    }

    out.clear();
    out.push_back("All Tags");
    for (const auto& tag : uniqueTags) {
        out.push_back(tag);
    }
}

void TrainingPackManager::SavePacksToFile(const std::filesystem::path& filePath)
{
    try {
        std::lock_guard<std::mutex> lock(packMutex);
        nlohmann::json output;
        output["version"] = "1.0.0";

        // Get current time in ISO format
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&tt), "%Y-%m-%dT%H:%M:%SZ");
        output["lastUpdated"] = oss.str();

        output["source"] = "https://prejump.com/training-packs";
        output["totalPacks"] = RLTraining.size();

        nlohmann::json packsArray = nlohmann::json::array();
        for (const auto& pack : RLTraining) {
            nlohmann::json p;
            p["name"] = pack.name;
            p["code"] = pack.code;
            p["creator"] = pack.creator;
            p["creatorSlug"] = pack.creatorSlug;
            p["difficulty"] = pack.difficulty;
            p["shotCount"] = pack.shotCount;
            p["tags"] = pack.tags;
            p["videoUrl"] = pack.videoUrl;
            p["staffComments"] = pack.staffComments;
            p["notes"] = pack.notes;
            p["likes"] = pack.likes;
            p["plays"] = pack.plays;
            p["status"] = pack.status;

            // Unified system fields
            p["source"] = pack.source;
            // Save bag categories as array
            nlohmann::json bagCatsArray = nlohmann::json::array();
            for (const auto& cat : pack.bagCategories) {
                bagCatsArray.push_back(cat);
            }
            p["bagCategories"] = bagCatsArray;
            // Save orderInBag as object
            if (!pack.orderInBag.empty()) {
                nlohmann::json orderObj;
                for (const auto& [bagName, order] : pack.orderInBag) {
                    orderObj[bagName] = order;
                }
                p["orderInBag"] = orderObj;
            }
            p["isModified"] = pack.isModified;

            packsArray.push_back(p);
        }
        output["packs"] = packsArray;

        // Ensure directory exists
        auto parentDir = filePath.parent_path();
        if (!std::filesystem::exists(parentDir)) {
            std::filesystem::create_directories(parentDir);
        }

        std::ofstream file(filePath);
        if (!file.is_open()) {
            LOG("SuiteSpot: Failed to open file for writing: {}", filePath.string());
            return;
        }

        file << output.dump(2); // Pretty print with 2-space indent
        file.close();

        currentFilePath = filePath;
        lastUpdated = GetLastUpdatedTime(filePath);
        LOG("SuiteSpot: Saved {} packs to file", RLTraining.size());

    } catch (const std::exception& e) {
        LOG("SuiteSpot: Error saving packs: {}", std::string(e.what()));
    }
}

bool TrainingPackManager::AddCustomPack(const TrainingEntry& pack)
{
    {
        std::lock_guard<std::mutex> lock(packMutex);
        // Check for duplicate code
        for (const auto& existing : RLTraining) {
            if (existing.code == pack.code) {
                LOG("SuiteSpot: Pack with code {} already exists", pack.code);
                return false;
            }
        }

        TrainingEntry newPack = pack;
        newPack.source = "custom";
        RLTraining.push_back(newPack);

        // Sort RLTraining alphabetically by name
        std::sort(RLTraining.begin(), RLTraining.end(), [](const TrainingEntry& a, const TrainingEntry& b) {
            std::string nameA = a.name;
            std::string nameB = b.name;
            std::transform(nameA.begin(), nameA.end(), nameA.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(nameB.begin(), nameB.end(), nameB.begin(), [](unsigned char c) { return std::tolower(c); });
            return nameA < nameB;
        });

        packCount = static_cast<int>(RLTraining.size());
        LOG("SuiteSpot: Added custom pack: {}", pack.name);
        // Lock releases here before SavePacksToFile
    }

    // Auto-save if we have a file path (outside the lock)
    if (!currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }
    return true;
}

bool TrainingPackManager::UpdatePack(const std::string& code, const TrainingEntry& updatedPack)
{
    {
        std::lock_guard<std::mutex> lock(packMutex);
        for (auto& pack : RLTraining) {
            if (pack.code == code) {
                // Preserve source and update isModified
                std::string originalSource = pack.source;
                pack = updatedPack;
                pack.source = originalSource;

                // Mark as modified if it was a prejump pack
                if (originalSource == "prejump") {
                    pack.isModified = true;
                }

                // Sort RLTraining alphabetically by name
                std::sort(RLTraining.begin(), RLTraining.end(), [](const TrainingEntry& a, const TrainingEntry& b) {
                    std::string nameA = a.name;
                    std::string nameB = b.name;
                    std::transform(nameA.begin(), nameA.end(), nameA.begin(), [](unsigned char c) { return std::tolower(c); });
                    std::transform(nameB.begin(), nameB.end(), nameB.begin(), [](unsigned char c) { return std::tolower(c); });
                    return nameA < nameB;
                });

                LOG("SuiteSpot: Updated pack: {}", pack.name);
                // Lock releases here before SavePacksToFile
            }
        }
    }

    // Auto-save outside the lock
    if (!currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
        return true;
    }
    return false;
}

bool TrainingPackManager::DeletePack(const std::string& code)
{
    std::string name;
    {
        std::lock_guard<std::mutex> lock(packMutex);
        auto it = std::find_if(RLTraining.begin(), RLTraining.end(),
            [&code](const TrainingEntry& p) { return p.code == code; });

        if (it != RLTraining.end()) {
            name = it->name;
            RLTraining.erase(it);
            packCount = static_cast<int>(RLTraining.size());

            LOG("SuiteSpot: Deleted pack: {}", name);
        } else {
            return false;
        }
    }

    // Auto-save outside the lock
    if (!currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }
    return true;
}

// ============================================================================
// CATEGORIZED BAG SYSTEM
// ============================================================================

void TrainingPackManager::InitializeDefaultBags()
{
    availableBags.clear();

    // Defense bag - saves, clears, defensive plays
    availableBags.push_back({
        "Defense", "Defense", "D",
        {"Saves", "Defensive", "Clears", "Shadow"},
        true, 1, false, {0.3f, 0.6f, 0.9f, 1.0f}  // Blue
    });

    // Offense bag - shots, finishing
    availableBags.push_back({
        "Offense", "Offense", "O",
        {"Offensive", "Shots", "Finishing", "Power"},
        true, 2, false, {0.9f, 0.4f, 0.3f, 1.0f}  // Red
    });

    // Air Control bag - aerials, air dribbles
    availableBags.push_back({
        "Air", "Air Control", "A",
        {"Aerials", "Air rolls", "Air dribble"},
        true, 3, false, {0.5f, 0.8f, 0.9f, 1.0f}  // Cyan
    });

    // Dribble bag - ground dribbles, flicks
    availableBags.push_back({
        "Dribble", "Dribble", "Dr",
        {"Dribbling", "Ground", "Flicks", "Ball control"},
        true, 4, false, {0.9f, 0.7f, 0.2f, 1.0f}  // Orange
    });

    // Rebounds bag - backboard, redirects
    availableBags.push_back({
        "Rebounds", "Rebounds", "R",
        {"Rebounds", "Redirects", "Backboard", "Double"},
        true, 5, false, {0.7f, 0.4f, 0.9f, 1.0f}  // Purple
    });

    // Warmup bag - general, variety
    availableBags.push_back({
        "Warmup", "Warmup", "W",
        {"Good for beginners", "Variety", "Warmup"},
        true, 0, false, {0.4f, 0.9f, 0.4f, 1.0f}  // Green
    });

    LOG("SuiteSpot: Initialized {} default training bags", availableBags.size());
}

const TrainingBag* TrainingPackManager::GetBag(const std::string& bagName) const
{
    for (const auto& bag : availableBags) {
        if (bag.name == bagName) {
            return &bag;
        }
    }
    return nullptr;
}

std::vector<TrainingEntry> TrainingPackManager::GetPacksInBag(const std::string& bagName) const
{
    std::lock_guard<std::mutex> lock(packMutex);
    std::vector<TrainingEntry> result;
    for (const auto& pack : RLTraining) {
        if (pack.bagCategories.count(bagName) > 0) {
            result.push_back(pack);
        }
    }

    // Sort by orderInBag
    std::sort(result.begin(), result.end(), [&bagName](const TrainingEntry& a, const TrainingEntry& b) {
        auto it_a = a.orderInBag.find(bagName);
        auto it_b = b.orderInBag.find(bagName);

        int order_a = (it_a != a.orderInBag.end()) ? it_a->second : INT_MAX;
        int order_b = (it_b != b.orderInBag.end()) ? it_b->second : INT_MAX;

        return order_a < order_b;
    });

    return result;
}

int TrainingPackManager::GetBagPackCount(const std::string& bagName) const
{
    std::lock_guard<std::mutex> lock(packMutex);
    int count = 0;
    for (const auto& pack : RLTraining) {
        if (pack.bagCategories.count(bagName) > 0) {
            count++;
        }
    }
    return count;
}

void TrainingPackManager::AddPackToBag(const std::string& code, const std::string& bagName)
{
    bool shouldSave = false;
    {
        std::lock_guard<std::mutex> lock(packMutex);
        for (auto& pack : RLTraining) {
            if (pack.code == code) {
                if (pack.bagCategories.insert(bagName).second) {
                    // Assign order (append to end)
                    int maxOrder = -1;
                    for (const auto& p : RLTraining) {
                        if (p.bagCategories.count(bagName) > 0) {
                            auto orderIt = p.orderInBag.find(bagName);
                            if (orderIt != p.orderInBag.end()) {
                                maxOrder = std::max(maxOrder, orderIt->second);
                            }
                        }
                    }
                    pack.orderInBag[bagName] = maxOrder + 1;

                    shouldSave = true;
                    LOG("SuiteSpot: Added pack '{}' to bag '{}'", pack.name, bagName);
                }
                break;
            }
        }
    }
    if (shouldSave && !currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }
}

void TrainingPackManager::AddPacksToBag(const std::vector<std::string>& codes, const std::string& bagName)
{
    bool shouldSave = false;
    {
        std::lock_guard<std::mutex> lock(packMutex);
        for (auto& pack : RLTraining) {
            for (const auto& code : codes) {
                if (pack.code == code) {
                    if (pack.bagCategories.insert(bagName).second) {
                        shouldSave = true;
                    }
                    break;
                }
            }
        }
    }
    if (shouldSave && !currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
        LOG("SuiteSpot: Added {} packs to bag '{}'", codes.size(), bagName);
    }
}

void TrainingPackManager::RemovePackFromBag(const std::string& code, const std::string& bagName)
{
    bool shouldSave = false;
    {
        std::lock_guard<std::mutex> lock(packMutex);
        for (auto& pack : RLTraining) {
            if (pack.code == code) {
                if (pack.bagCategories.erase(bagName) > 0) {
                    shouldSave = true;
                    LOG("SuiteSpot: Removed pack '{}' from bag '{}'", pack.name, bagName);
                }
                break;
            }
        }
    }
    if (shouldSave && !currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }
}

void TrainingPackManager::RemovePackFromAllBags(const std::string& code)
{
    bool shouldSave = false;
    {
        std::lock_guard<std::mutex> lock(packMutex);
        for (auto& pack : RLTraining) {
            if (pack.code == code) {
                if (!pack.bagCategories.empty()) {
                    pack.bagCategories.clear();
                    shouldSave = true;
                    LOG("SuiteSpot: Removed pack '{}' from all bags", pack.name);
                }
                break;
            }
        }
    }
    if (shouldSave && !currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }
}

void TrainingPackManager::ClearBag(const std::string& bagName)
{
    bool shouldSave = false;
    int removedCount = 0;
    {
        std::lock_guard<std::mutex> lock(packMutex);
        for (auto& pack : RLTraining) {
            if (pack.bagCategories.erase(bagName) > 0) {
                // Also remove orderInBag entry
                pack.orderInBag.erase(bagName);
                shouldSave = true;
                removedCount++;
            }
        }
    }
    if (shouldSave && !currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
        LOG("SuiteSpot: Cleared {} packs from bag '{}'", removedCount, bagName);
    }
}

bool TrainingPackManager::IsPackInBag(const std::string& code, const std::string& bagName) const
{
    std::lock_guard<std::mutex> lock(packMutex);
    for (const auto& pack : RLTraining) {
        if (pack.code == code) {
            return pack.bagCategories.count(bagName) > 0;
        }
    }
    return false;
}

void TrainingPackManager::SwapPacksInBag(const std::string& bagName, int idx1, int idx2)
{
    std::lock_guard<std::mutex> lock(packMutex);

    // Get all packs in bag (in order they appear after sorting)
    std::vector<TrainingEntry*> packsInBag;
    for (auto& pack : RLTraining) {
        if (pack.bagCategories.count(bagName) > 0) {
            packsInBag.push_back(&pack);
        }
    }

    // Sort to match display order
    std::sort(packsInBag.begin(), packsInBag.end(), [&bagName](const TrainingEntry* a, const TrainingEntry* b) {
        auto it_a = a->orderInBag.find(bagName);
        auto it_b = b->orderInBag.find(bagName);

        int order_a = (it_a != a->orderInBag.end()) ? it_a->second : INT_MAX;
        int order_b = (it_b != b->orderInBag.end()) ? it_b->second : INT_MAX;

        return order_a < order_b;
    });

    // Validate indices
    if (idx1 < 0 || idx1 >= (int)packsInBag.size() ||
        idx2 < 0 || idx2 >= (int)packsInBag.size()) {
        return;
    }

    TrainingEntry* pack1 = packsInBag[idx1];
    TrainingEntry* pack2 = packsInBag[idx2];

    // Swap order values
    int order1 = pack1->orderInBag[bagName];
    int order2 = pack2->orderInBag[bagName];
    pack1->orderInBag[bagName] = order2;
    pack2->orderInBag[bagName] = order1;

    // Mark as modified
    pack1->isModified = true;
    pack2->isModified = true;

    // Save to disk
    if (!currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }
}

void TrainingPackManager::SetBagEnabled(const std::string& bagName, bool enabled)
{
    for (auto& bag : availableBags) {
        if (bag.name == bagName) {
            bag.enabled = enabled;
            LOG("SuiteSpot: Bag '{}' {}", bagName, enabled ? "enabled" : "disabled");
            break;
        }
    }
}

bool TrainingPackManager::CreateCustomBag(const std::string& name, const std::string& icon, const float color[4])
{
    // Check for max bags (6 predefined + 6 custom = 12)
    if (availableBags.size() >= 12) {
        LOG("SuiteSpot: Cannot create bag '{}' - maximum 12 bags reached", name);
        return false;
    }

    // Check for duplicate name
    for (const auto& bag : availableBags) {
        if (bag.name == name) {
            LOG("SuiteSpot: Cannot create bag '{}' - name already exists", name);
            return false;
        }
    }

    TrainingBag newBag;
    newBag.name = name;
    newBag.displayName = name;
    newBag.icon = icon;
    newBag.enabled = true;
    newBag.priority = static_cast<int>(availableBags.size());
    newBag.isUserCreated = true;
    newBag.color[0] = color[0];
    newBag.color[1] = color[1];
    newBag.color[2] = color[2];
    newBag.color[3] = color[3];

    availableBags.push_back(newBag);
    LOG("SuiteSpot: Created custom bag '{}'", name);
    return true;
}

bool TrainingPackManager::DeleteCustomBag(const std::string& bagName)
{
    for (auto it = availableBags.begin(); it != availableBags.end(); ++it) {
        if (it->name == bagName && it->isUserCreated) {
            availableBags.erase(it);
            // Also remove this bag from all packs
            {
                std::lock_guard<std::mutex> lock(packMutex);
                for (auto& pack : RLTraining) {
                    pack.bagCategories.erase(bagName);
                }
            }
            if (!currentFilePath.empty()) {
                SavePacksToFile(currentFilePath);
            }
            LOG("SuiteSpot: Deleted custom bag '{}'", bagName);
            return true;
        }
    }
    return false;
}

TrainingEntry TrainingPackManager::GetNextFromRotation()
{
    std::lock_guard<std::mutex> lock(packMutex);

    // Get enabled bags sorted by priority
    std::vector<const TrainingBag*> enabledBags;
    for (const auto& bag : availableBags) {
        if (bag.enabled && GetBagPackCount(bag.name) > 0) {
            enabledBags.push_back(&bag);
        }
    }

    if (enabledBags.empty()) {
        // Fallback: random from all packs
        if (!RLTraining.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, static_cast<int>(RLTraining.size()) - 1);
            return RLTraining[dist(gen)];
        }
        return TrainingEntry{};  // Empty
    }

    // Sort by priority
    std::sort(enabledBags.begin(), enabledBags.end(), [](const TrainingBag* a, const TrainingBag* b) {
        return a->priority < b->priority;
    });

    // Round-robin through bags
    currentRotationIndex = currentRotationIndex % enabledBags.size();
    const TrainingBag* currentBag = enabledBags[currentRotationIndex];
    currentRotationIndex++;

    // Get random pack from this bag
    std::vector<TrainingEntry> bagsPacksLocal;
    for (const auto& pack : RLTraining) {
        if (pack.bagCategories.count(currentBag->name) > 0) {
            bagsPacksLocal.push_back(pack);
        }
    }

    if (!bagsPacksLocal.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, static_cast<int>(bagsPacksLocal.size()) - 1);
        TrainingEntry selected = bagsPacksLocal[dist(gen)];
        LOG("SuiteSpot: Selected pack '{}' from bag '{}'", selected.name, currentBag->name);
        return selected;
    }

    return TrainingEntry{};
}

std::string TrainingPackManager::GetNextBagInRotation() const
{
    std::vector<const TrainingBag*> enabledBags;
    for (const auto& bag : availableBags) {
        if (bag.enabled && GetBagPackCount(bag.name) > 0) {
            enabledBags.push_back(&bag);
        }
    }

    if (enabledBags.empty()) {
        return "None";
    }

    std::sort(enabledBags.begin(), enabledBags.end(), [](const TrainingBag* a, const TrainingBag* b) {
        return a->priority < b->priority;
    });

    int nextIndex = currentRotationIndex % enabledBags.size();
    return enabledBags[nextIndex]->displayName;
}

TrainingEntry* TrainingPackManager::GetPackByCode(const std::string& code)
{
    std::lock_guard<std::mutex> lock(packMutex);
    for (auto& pack : RLTraining) {
        if (pack.code == code) {
            return &pack;
        }
    }
    return nullptr;
}

void TrainingPackManager::TestHealerFetch(std::shared_ptr<GameWrapper> gw, std::string code) {
    if (!gw) return;

    auto gfxTraining = gw->GetGfxTrainingData();
    if (gfxTraining.IsNull()) {
        LOG("Healer: GfxTrainingData is null");
        return;
    }

    LOG("Healer: Attempting to set playlist to code: {0}", code);
    
    // Attempt to set the playlist context to the code.
    // Hypothesis: This triggers the game to fetch metadata to populate the 'Browse' UI state.
    gfxTraining.SetCurrentPlaylist(code);

    // We delay the read slightly to allow for any async fetch
    gw->SetTimeout([gw, code](GameWrapper* gw_ptr) {
        auto gfx = gw_ptr->GetGfxTrainingData();
        if (gfx.IsNull()) return;

        int rounds = gfx.GetTotalRounds();
        std::string diff = gfx.GetDifficulty().ToString();
        std::string type = gfx.GetTrainingType();

        LOG("Healer: Result for code {0}: Rounds={1}, Difficulty={2}, Type={3}", code, rounds, diff, type);
    }, 1.5f);
}
#include "pch.h"
#include "TrainingPackManager.h"
#include "bakkesmod/wrappers/GameWrapper.h"
#include "logging.h"





void TrainingPackManager::HealPack(const std::string& code, int shots)
{
    bool needsSave = false;
    {
        std::lock_guard<std::mutex> lock(packMutex);
        for (auto& pack : RLTraining) {
            if (pack.code == code) {
                if (pack.shotCount <= 0) {
                    pack.shotCount = shots;
                    needsSave = true;
                    LOG("SuiteSpot: Healed metadata for pack: {}", code);
                }
                break;
            }
        }
    }

    if (needsSave && !currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }
}
