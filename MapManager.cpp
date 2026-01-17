#include "pch.h"
#include "MapManager.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <unordered_set>

namespace
{
    std::string Trim(const std::string& value)
    {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return {};
        }
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::string StripQuotes(const std::string& value)
    {
        if (value.size() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\'')))
        {
            return value.substr(1, value.size() - 2);
        }
        return value;
    }

    std::string ExpandEnvAndHome(const std::string& input)
    {
        std::string expanded;
        expanded.reserve(input.size());

        for (size_t i = 0; i < input.size();)
        {
            if (input[i] == '%')
            {
                const auto end = input.find('%', i + 1);
                if (end != std::string::npos)
                {
                    const auto varName = input.substr(i + 1, end - i - 1);
                    if (!varName.empty())
                    {
                        if (const char* val = std::getenv(varName.c_str()))
                        {
                            expanded.append(val);
                        }
                    }
                    i = end + 1;
                    continue;
                }
            }
            expanded.push_back(input[i]);
            ++i;
        }

        if (!expanded.empty() && expanded[0] == '~')
        {
            if (const char* home = std::getenv("USERPROFILE"))
            {
                expanded.replace(0, 1, home);
            }
        }

        return expanded;
    }

    int CaseInsensitiveCompare(const std::string& a, const std::string& b)
    {
        const size_t len = std::min(a.size(), b.size());
        for (size_t i = 0; i < len; i++)
        {
            const char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(a[i])));
            const char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(b[i])));
            if (ca != cb) return (ca < cb) ? -1 : 1;
        }
        if (a.size() == b.size()) return 0;
        return (a.size() < b.size()) ? -1 : 1;
    }

}

MapManager::MapManager() {}

std::filesystem::path MapManager::GetDataRoot() const
{
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return std::filesystem::path();
    return std::filesystem::path(appdata) / "bakkesmod" / "bakkesmod" / "data";
}

std::filesystem::path MapManager::GetSuiteTrainingDir() const
{
    return GetDataRoot() / "SuiteSpot" / "SuiteTraining";
}

std::filesystem::path MapManager::GetWorkshopLoaderConfigPath() const
{
    return GetDataRoot() / "WorkshopMapLoader" / "workshopmaploader.cfg";
}

std::filesystem::path MapManager::ResolveConfiguredWorkshopRoot() const
{
    const auto cfg = GetWorkshopLoaderConfigPath();
    std::ifstream in(cfg);
    if (!in.is_open())
    {
        return {};
    }

    std::string line;
    while (std::getline(in, line))
    {
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
        {
            continue;
        }

        const auto keyPos = trimmed.find("MapsFolderPath");
        if (keyPos == std::string::npos)
        {
            continue;
        }

        const auto eqPos = trimmed.find('=', keyPos);
        if (eqPos == std::string::npos)
        {
            continue;
        }

        std::string value = trimmed.substr(eqPos + 1);
        value = StripQuotes(Trim(value));
        value = ExpandEnvAndHome(value);

        if (value.empty())
        {
            continue;
        }

        std::error_code ec;
        std::filesystem::path candidate(value);
        if (std::filesystem::exists(candidate, ec) && std::filesystem::is_directory(candidate, ec))
        {
            return candidate;
        }

        LOG("SuiteSpot: Configured workshop path not found: " + value);
    }

    return {};
}

void MapManager::EnsureDataDirectories() const
{
    std::error_code ec;
    auto root = GetDataRoot();
    if (!root.empty()) std::filesystem::create_directories(root, ec);
    ec.clear();
    std::filesystem::create_directories(GetSuiteTrainingDir(), ec);
}

void MapManager::DiscoverWorkshopInDir(const std::filesystem::path& dir,
                                       std::vector<WorkshopEntry>& workshop) const
{
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) return;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (ec) { ec.clear(); continue; }
        if (!entry.is_directory()) continue;

        std::string foundMapFile;
        for (const auto& file : std::filesystem::directory_iterator(entry.path(), ec))
        {
            if (ec) { ec.clear(); continue; }
            if (!file.is_regular_file()) continue;

            const auto& path = file.path();
            if (path.extension().string() == ".upk")
            {
                foundMapFile = path.string();
                break;
            }
        }

        if (!foundMapFile.empty())
        {
            workshop.push_back({ foundMapFile, entry.path().filename().string() });
        }
    }
}

void MapManager::LoadWorkshopMaps(std::vector<WorkshopEntry>& workshop, int& currentWorkshopIndex)
{
    workshop.clear();

    std::vector<std::filesystem::path> roots;
    std::unordered_set<std::string> seenRoots;

    // Helper to add root only if not already seen
    auto addRoot = [&](const std::filesystem::path& path) {
        if (path.empty()) return;
        std::error_code ec;
        std::string canonical;
        if (std::filesystem::exists(path, ec)) {
            canonical = std::filesystem::canonical(path, ec).string();
        }
        if (canonical.empty()) {
            canonical = path.string();
        }
        // Normalize to lowercase for comparison on Windows
        std::string lower = canonical;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
        if (seenRoots.insert(lower).second) {
            roots.push_back(path);
        }
    };

    if (const auto configured = ResolveConfiguredWorkshopRoot(); !configured.empty())
    {
        addRoot(configured);
    }

    const char* progFiles = std::getenv("ProgramFiles");
    const char* progFilesX86 = std::getenv("ProgramFiles(x86)");

    if (progFiles) {
        addRoot(std::filesystem::path(progFiles) / "Epic Games" / "rocketleague" / "TAGame" / "CookedPCConsole" / "mods");
    }
    if (progFilesX86) {
        addRoot(std::filesystem::path(progFilesX86) / "Steam" / "steamapps" / "common" / "rocketleague" / "TAGame" / "CookedPCConsole" / "mods");
    }

    for (const auto& root : roots)
    {
        DiscoverWorkshopInDir(root, workshop);
    }

    std::unordered_set<std::string> seen;
    std::vector<WorkshopEntry> unique;
    unique.reserve(workshop.size());
    for (const auto& entry : workshop)
    {
        if (seen.insert(entry.filePath).second)
        {
            unique.push_back(entry);
        }
    }
    workshop.swap(unique);

    std::sort(workshop.begin(), workshop.end(),
        [](const WorkshopEntry& lhs, const WorkshopEntry& rhs)
        {
            const int cmp = CaseInsensitiveCompare(lhs.name, rhs.name);
            if (cmp == 0)
            {
                return lhs.filePath < rhs.filePath;
            }
            return cmp < 0;
        });

    if (workshop.empty())
    {
        currentWorkshopIndex = 0;
    }
    else
    {
        currentWorkshopIndex = std::clamp(currentWorkshopIndex, 0, static_cast<int>(workshop.size() - 1));
    }
}