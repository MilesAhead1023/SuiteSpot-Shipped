#include "pch.h"
#include "TrainingPackManager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

void TrainingPackManager::LoadPacksFromFile(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath)) {
        LOG("SuiteSpot: Pack cache file not found: {}", filePath.string());
        RLTraining.clear();
        packCount = 0;
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
            if (pack.contains("inShuffleBag") && pack["inShuffleBag"].is_boolean()) {
                entry.inShuffleBag = pack["inShuffleBag"].get<bool>();
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
        LOG("SuiteSpot: Loaded {} training packs from file", packCount);

    } catch (const std::exception& e) {
        LOG("SuiteSpot: Error loading training packs: {}", std::string(e.what()));
        RLTraining.clear();
        packCount = 0;
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

void TrainingPackManager::ScrapeAndLoadTrainingPacks(const std::filesystem::path& outputPath,
                                                   const std::shared_ptr<GameWrapper>& gameWrapper)
{
    if (scrapingInProgress) {
        LOG("SuiteSpot: Pack scraping already in progress");
        return;
    }

    auto dataFolder = gameWrapper->GetDataFolder();
    auto scraperScript = dataFolder / "SuiteSpot" / "scrape_prejump.ps1";

    if (!std::filesystem::exists(scraperScript)) {
        // Fallback to searching in the data root if not in SuiteSpot subfolder
        scraperScript = dataFolder / "scrape_prejump.ps1";
    }

    if (!std::filesystem::exists(scraperScript)) {
        LOG("SuiteSpot: Pack scraper script not found in data folder: {}", scraperScript.string());
        return;
    }

    std::string cmd = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scraperScript.string()
                    + "\" -OutputPath \"" + outputPath.string() + "\" -QuietMode:$true";

    scrapingInProgress = true;
    LOG("SuiteSpot: Started scraper...");

    if (!gameWrapper) {
        scrapingInProgress = false;
        LOG("SuiteSpot: GameWrapper unavailable for scrape");
        return;
    }

    gameWrapper->SetTimeout([this, cmd, outputPath](GameWrapper* gw) {
        int result = system(cmd.c_str());

        if (result == 0) {
            LOG("SuiteSpot: Scraper completed successfully");
            LoadPacksFromFile(outputPath);
            lastUpdated = GetLastUpdatedTime(outputPath);
        } else {
            LOG("SuiteSpot: Scraper failed with exit code {}", result);
        }

        scrapingInProgress = false;
    }, 0.1f);
}

void TrainingPackManager::FilterAndSortPacks(const std::string& searchText,
                                            const std::string& difficultyFilter,
                                            const std::string& tagFilter,
                                            int minShots,
                                            int sortColumn,
                                            bool sortAscending,
                                            std::vector<TrainingEntry>& out) const
{
    out.clear();

    std::string searchLower(searchText);
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (const auto& pack : RLTraining) {
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

        if (difficultyFilter != "All" && pack.difficulty != difficultyFilter) {
            continue;
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
            p["inShuffleBag"] = pack.inShuffleBag;
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

    // Auto-save if we have a file path
    if (!currentFilePath.empty()) {
        SavePacksToFile(currentFilePath);
    }

    LOG("SuiteSpot: Added custom pack: {}", pack.name);
    return true;
}

bool TrainingPackManager::UpdatePack(const std::string& code, const TrainingEntry& updatedPack)
{
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

            // Auto-save
            if (!currentFilePath.empty()) {
                SavePacksToFile(currentFilePath);
            }

            LOG("SuiteSpot: Updated pack: {}", pack.name);
            return true;
        }
    }
    return false;
}

bool TrainingPackManager::DeletePack(const std::string& code)
{
    auto it = std::find_if(RLTraining.begin(), RLTraining.end(),
        [&code](const TrainingEntry& p) { return p.code == code; });

    if (it != RLTraining.end()) {
        std::string name = it->name;
        RLTraining.erase(it);
        packCount = static_cast<int>(RLTraining.size());

        // Auto-save
        if (!currentFilePath.empty()) {
            SavePacksToFile(currentFilePath);
        }

        LOG("SuiteSpot: Deleted pack: {}", name);
        return true;
    }
    return false;
}

void TrainingPackManager::ToggleShuffleBag(const std::string& code)
{
    for (auto& pack : RLTraining) {
        if (pack.code == code) {
            pack.inShuffleBag = !pack.inShuffleBag;

            // Auto-save
            if (!currentFilePath.empty()) {
                SavePacksToFile(currentFilePath);
            }

            LOG("SuiteSpot: {} pack from shuffle bag: {}", std::string(pack.inShuffleBag ? "Added" : "Removed"), pack.name);
            return;
        }
    }
}

void TrainingPackManager::AddToShuffleBag(const std::string& code)
{
    for (auto& pack : RLTraining) {
        if (pack.code == code) {
            if (!pack.inShuffleBag) {
                pack.inShuffleBag = true;

                // Auto-save
                if (!currentFilePath.empty()) {
                    SavePacksToFile(currentFilePath);
                }

                LOG("SuiteSpot: Added pack to shuffle bag: {}", pack.name);
            }
            return;
        }
    }
}

std::vector<TrainingEntry> TrainingPackManager::GetShuffleBagPacks() const
{
    std::vector<TrainingEntry> shuffleBag;
    for (const auto& pack : RLTraining) {
        if (pack.inShuffleBag) {
            shuffleBag.push_back(pack);
        }
    }
    return shuffleBag;
}

TrainingEntry* TrainingPackManager::GetPackByCode(const std::string& code)
{
    for (auto& pack : RLTraining) {
        if (pack.code == code) {
            return &pack;
        }
    }
    return nullptr;
}

int TrainingPackManager::GetShuffleBagCount() const
{
    int count = 0;
    for (const auto& pack : RLTraining) {
        if (pack.inShuffleBag) {
            count++;
        }
    }
    return count;
}
