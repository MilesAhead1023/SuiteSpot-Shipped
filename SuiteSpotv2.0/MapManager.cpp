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

    bool StartsWithCaseInsensitive(const std::string& value, const std::string& prefix)
    {
        if (value.size() < prefix.size()) return false;
        for (size_t i = 0; i < prefix.size(); i++)
        {
            if (std::tolower(static_cast<unsigned char>(value[i])) !=
                std::tolower(static_cast<unsigned char>(prefix[i])))
            {
                return false;
            }
        }
        return true;
    }

    int ParseTrailingShots(const std::string& nameField)
    {
        const auto openPos = nameField.find_last_of('(');
        const auto closePos = nameField.find_last_of(')');
        if (openPos == std::string::npos || closePos == std::string::npos || closePos <= openPos)
        {
            return 0;
        }
        std::string inside = Trim(nameField.substr(openPos + 1, closePos - openPos - 1));
        if (StartsWithCaseInsensitive(inside, "shots"))
        {
            auto colonPos = inside.find(':');
            if (colonPos != std::string::npos)
            {
                inside = Trim(inside.substr(colonPos + 1));
            }
        }
        try
        {
            return std::max(0, std::stoi(inside));
        }
        catch (...)
        {
            return 0;
        }
    }
}

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

std::filesystem::path MapManager::GetTrainingFilePath() const
{
    return GetSuiteTrainingDir() / "SuiteSpotTrainingMaps.txt";
}

std::filesystem::path MapManager::GetShuffleBagPath() const
{
    return GetSuiteTrainingDir() / "SuiteShuffleBag.txt";
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

void MapManager::LoadTrainingMaps(std::vector<TrainingEntry>& training, int& currentTrainingIndex)
{
    EnsureDataDirectories();
    EnsureReadmeFiles();
    training.clear();
    auto f = GetTrainingFilePath();
    std::error_code ec;

    if (!std::filesystem::exists(f, ec)) return;

    std::ifstream in(f.string());
    if (!in.is_open())
    {
        LOG("SuiteSpot: Failed to open training maps file: {}", f.string());
        return;
    }

    std::string line;
    bool sawLegacyFormat = false;
    int lineNum = 0;

    while (std::getline(in, line))
    {
        lineNum++;
        if (line.empty()) continue;

        std::vector<std::string> parts;
        size_t start = 0;
        while (start <= line.size())
        {
            const auto commaPos = line.find(',', start);
            if (commaPos == std::string::npos)
            {
                parts.push_back(Trim(line.substr(start)));
                break;
            }
            parts.push_back(Trim(line.substr(start, commaPos - start)));
            start = commaPos + 1;
        }

        if (parts.size() < 2)
        {
            LOG("SuiteSpot: Malformed entry on line {}: '{}' - expected 'code,name'", lineNum, line);
            continue;
        }

        std::string code = parts[0];
        std::string name = parts[1];
        int shots = 0;

        if (parts.size() == 2)
        {
            shots = ParseTrailingShots(name);
            if (shots > 0)
            {
                sawLegacyFormat = true;
            }
            const auto openPos = name.find_last_of('(');
            const auto closePos = name.find_last_of(')');
            if (openPos != std::string::npos && closePos != std::string::npos && closePos > openPos)
            {
                name = Trim(name.substr(0, openPos));
            }
        }

        if (parts.size() >= 3)
        {
            std::string shotsPart = parts[2];
            if (StartsWithCaseInsensitive(shotsPart, "shots:"))
            {
                auto valStr = Trim(shotsPart.substr(6));
                try
                {
                    shots = std::stoi(valStr);
                }
                catch (const std::invalid_argument&)
                {
                    LOG("SuiteSpot: Invalid shot count on line {}: '{}'", lineNum, valStr);
                    shots = 0;
                }
                catch (const std::out_of_range&)
                {
                    LOG("SuiteSpot: Shot count out of range on line {}: '{}'", lineNum, valStr);
                    shots = 0;
                }
                catch (...)
                {
                    LOG("SuiteSpot: Unknown error parsing shots on line {}", lineNum);
                    shots = 0;
                }
            }
        }

        if (!code.empty() && !name.empty())
        {
            TrainingEntry entry;
            entry.code = code;
            entry.name = name;
            entry.shotCount = shots;
            training.push_back(entry);
        }
        else
        {
            LOG("SuiteSpot: Empty code or name on line {}", lineNum);
        }
    }

    std::sort(training.begin(), training.end(),
        [](const TrainingEntry& lhs, const TrainingEntry& rhs)
        {
            return CaseInsensitiveCompare(lhs.name, rhs.name) < 0;
        });

    if (training.empty())
    {
        currentTrainingIndex = 0;
    }
    else
    {
        currentTrainingIndex = std::clamp(currentTrainingIndex, 0, static_cast<int>(training.size() - 1));
    }

    if (sawLegacyFormat)
    {
        LOG("SuiteSpot: Upgrading legacy training file format...");
        SaveTrainingMaps(training);
    }
}

void MapManager::SaveTrainingMaps(const std::vector<TrainingEntry>& training) const
{
    auto f = GetTrainingFilePath();
    EnsureDataDirectories();
    EnsureReadmeFiles();
    std::ofstream out(f.string(), std::ios::trunc);
    if (!out.is_open()) {
        LOG("SuiteSpot: Failed to open training maps file for writing: {}", f.string());
        return;
    }
    auto sorted = training;
    std::sort(sorted.begin(), sorted.end(),
        [](const TrainingEntry& lhs, const TrainingEntry& rhs)
        {
            return CaseInsensitiveCompare(lhs.name, rhs.name) < 0;
        });
    for (const auto& e : sorted)
    {
        out << e.code << "," << e.name << ",Shots:" << e.shotCount << "\n";
    }
    if (out.fail()) {
        LOG("SuiteSpot: Error writing training maps file: {}", f.string());
    }
}

void MapManager::LoadShuffleBag(std::vector<TrainingEntry>& shuffleBag,
                                std::set<int>& selectedTrainingIndices,
                                const std::vector<TrainingEntry>& training) const
{
    shuffleBag.clear();
    selectedTrainingIndices.clear();
    auto f = GetShuffleBagPath();
    std::error_code ec;
    if (!std::filesystem::exists(f, ec)) return;
    std::ifstream in(f.string());
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        auto pos = line.find(',');
        if (pos == std::string::npos) continue;
        std::string code = Trim(line.substr(0, pos));
        std::string name = Trim(line.substr(pos + 1));
        if (!code.empty() && !name.empty())
        {
            shuffleBag.push_back({ code, name });
            auto it = std::find_if(training.begin(), training.end(),
                [&](const TrainingEntry& e) { return e.code == code; });
            if (it != training.end())
            {
                selectedTrainingIndices.insert(static_cast<int>(std::distance(training.begin(), it)));
            }
        }
    }
}

void MapManager::SaveShuffleBag(const std::vector<TrainingEntry>& shuffleBag) const
{
    auto f = GetShuffleBagPath();
    EnsureDataDirectories();
    std::ofstream out(f.string(), std::ios::trunc);
    if (!out.is_open()) {
        LOG("SuiteSpot: Failed to open shuffle bag file for writing: {}", f.string());
        return;
    }
    for (const auto& e : shuffleBag)
    {
        out << e.code << "," << e.name << "\n";
    }
    if (out.fail()) {
        LOG("SuiteSpot: Error writing shuffle bag file: {}", f.string());
    }
}

int MapManager::GetRandomTrainingMap(const std::vector<TrainingEntry>& training,
                                     const std::vector<TrainingEntry>& shuffleBag) const
{
    if (shuffleBag.empty())
    {
        return 0;
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(shuffleBag.size()) - 1);
    int bagIndex = dist(rng);

    auto it = std::find_if(training.begin(), training.end(),
        [&](const TrainingEntry& e) { return e.code == shuffleBag[bagIndex].code; });

    if (it != training.end())
    {
        return static_cast<int>(std::distance(training.begin(), it));
    }

    return -1;
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
    if (const auto configured = ResolveConfiguredWorkshopRoot(); !configured.empty())
    {
        roots.push_back(configured);
    }

    const char* progFiles = std::getenv("ProgramFiles");
    const char* progFilesX86 = std::getenv("ProgramFiles(x86)");

    if (progFiles) {
        roots.emplace_back(std::filesystem::path(progFiles) / "Epic Games" / "rocketleague" / "TAGame" / "CookedPCConsole" / "mods");
    }
    if (progFilesX86) {
        roots.emplace_back(std::filesystem::path(progFilesX86) / "Steam" / "steamapps" / "common" / "rocketleague" / "TAGame" / "CookedPCConsole" / "mods");
    }

    for (const auto& root : roots)
    {
        if (!root.empty())
        {
            DiscoverWorkshopInDir(root, workshop);
        }
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

void MapManager::MirrorDirectory(const std::filesystem::path& src,
                                 const std::filesystem::path& dst) const
{
    std::error_code ec;
    if (!std::filesystem::exists(src, ec) || !std::filesystem::is_directory(src, ec)) return;
    std::filesystem::create_directories(dst, ec);
    for (auto const& entry : std::filesystem::recursive_directory_iterator(src, ec))
    {
        if (ec) { ec.clear(); continue; }
        auto rel = std::filesystem::relative(entry.path(), src, ec);
        if (ec) { ec.clear(); continue; }
        auto target = dst / rel;
        if (entry.is_directory())
        {
            std::filesystem::create_directories(target, ec);
        }
        else if (entry.is_regular_file())
        {
            bool copyNeeded = true;
            if (std::filesystem::exists(target, ec))
            {
                auto src_time = std::filesystem::last_write_time(entry.path(), ec);
                auto dst_time = std::filesystem::last_write_time(target, ec);
                auto src_size = std::filesystem::file_size(entry.path(), ec);
                auto dst_size = std::filesystem::file_size(target, ec);
                copyNeeded = (src_time != dst_time) || (src_size != dst_size);
            }
            if (copyNeeded)
            {
                std::filesystem::create_directories(target.parent_path(), ec);
                std::filesystem::copy_file(entry.path(), target,
                                           std::filesystem::copy_options::overwrite_existing, ec);
            }
        }
    }
}

void MapManager::EnsureReadmeFiles() const
{
    auto tr = GetSuiteTrainingDir() / "README.txt";
    if (!std::filesystem::exists(tr))
    {
        std::ofstream o(tr.string(), std::ios::trunc);
        if (!o.is_open()) {
            LOG("SuiteSpot: Failed to create README file: {}", tr.string());
            return;
        }
        o << "SuiteTraining\\SuiteSpotTrainingMaps.txt\n"
             "CSV format:\n"
             "    <training_code>,<display_name>\n"
             "One entry per line. This file is read on game start and updated when you add a map in SuiteSpot.\n";
        if (o.fail()) {
            LOG("SuiteSpot: Error writing README file: {}", tr.string());
        }
    }
}
