#include "pch.h"
#include "PrejumpPackManager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

void PrejumpPackManager::LoadPacksFromFile(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath)) {
        LOG("SuiteSpot: Prejump packs file not found: " + filePath.string());
        packs.clear();
        packCount = 0;
        lastUpdated = "Never";
        return;
    }

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG("SuiteSpot: Failed to open Prejump packs file");
            return;
        }

        nlohmann::json jsonData;
        file >> jsonData;
        file.close();

        packs.clear();

        if (!jsonData.contains("packs") || !jsonData["packs"].is_array()) {
            LOG("SuiteSpot: Invalid Prejump packs file format - missing 'packs' array");
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

            packs.push_back(entry);
        }

        packCount = static_cast<int>(packs.size());
        lastUpdated = GetLastUpdatedTime(filePath);
        LOG("SuiteSpot: Loaded " + std::to_string(packCount) + " prejump packs from file");

    } catch (const std::exception& e) {
        LOG("SuiteSpot: Error loading Prejump packs: " + std::string(e.what()));
        packs.clear();
        packCount = 0;
    }
}

bool PrejumpPackManager::IsCacheStale(const std::filesystem::path& filePath) const
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

std::string PrejumpPackManager::GetLastUpdatedTime(const std::filesystem::path& filePath) const
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

void PrejumpPackManager::ScrapeAndLoadPrejumpPacks(const std::filesystem::path& outputPath,
                                                   const std::shared_ptr<GameWrapper>& gameWrapper)
{
    if (scrapingInProgress) {
        LOG("SuiteSpot: Prejump scraping already in progress");
        return;
    }

    auto dataFolder = gameWrapper->GetDataFolder();
    auto scraperScript = dataFolder / "SuiteSpot" / "scrape_prejump.ps1";

    if (!std::filesystem::exists(scraperScript)) {
        // Fallback to searching in the data root if not in SuiteSpot subfolder
        scraperScript = dataFolder / "scrape_prejump.ps1";
    }

    if (!std::filesystem::exists(scraperScript)) {
        LOG("SuiteSpot: Prejump scraper script not found in data folder: " + scraperScript.string());
        return;
    }

    std::string cmd = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scraperScript.string()
                    + "\" -OutputPath \"" + outputPath.string() + "\" -QuietMode:$true";

    scrapingInProgress = true;
    LOG("SuiteSpot: Started Prejump scraper...");

    if (!gameWrapper) {
        scrapingInProgress = false;
        LOG("SuiteSpot: GameWrapper unavailable for Prejump scrape");
        return;
    }

    gameWrapper->SetTimeout([this, cmd, outputPath](GameWrapper* gw) {
        int result = system(cmd.c_str());

        if (result == 0) {
            LOG("SuiteSpot: Prejump scraper completed successfully");
            LoadPacksFromFile(outputPath);
            lastUpdated = GetLastUpdatedTime(outputPath);
        } else {
            LOG("SuiteSpot: Prejump scraper failed with exit code " + std::to_string(result));
        }

        scrapingInProgress = false;
    }, 0.1f);
}

void PrejumpPackManager::FilterAndSortPacks(const std::string& searchText,
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

    for (const auto& pack : packs) {
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

    std::sort(out.begin(), out.end(), [sortColumn, sortAscending](const TrainingEntry& a, const TrainingEntry& b) {
        int cmp = 0;
        switch (sortColumn) {
            case 0: // Name
                cmp = a.name.compare(b.name);
                break;
            case 1: // Creator
                cmp = a.creator.compare(b.creator);
                break;
            case 2: // Difficulty
                cmp = a.difficulty.compare(b.difficulty);
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

void PrejumpPackManager::BuildAvailableTags(std::vector<std::string>& out) const
{
    std::set<std::string> uniqueTags;
    for (const auto& pack : packs) {
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
