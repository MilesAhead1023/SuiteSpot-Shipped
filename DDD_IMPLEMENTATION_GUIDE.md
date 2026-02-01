# SuiteSpot DDD Implementation Guide

**Phase-by-Phase Refactoring with Code Examples**

---

## PHASE 1: Extract Explicit Aggregates

### Step 1.1: Create TrainingPackCatalog Aggregate Root

**File: `Domain/Aggregates/TrainingPackCatalog.h`**

```cpp
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "../MapList.h"

namespace Domain {

/**
 * TrainingPackCatalog Aggregate Root
 *
 * Represents the complete library of training packs, managing:
 * - Loading from persistent storage
 * - Synchronizing with web sources
 * - Searching and filtering
 * - Creating and modifying custom packs
 *
 * INVARIANTS:
 * - Pack codes must be unique within the catalog
 * - Source must be either "prejump" or "custom"
 * - Difficulty must be a valid value
 * - Shot count must be >= 0
 */
class TrainingPackCatalog {
public:
    TrainingPackCatalog();
    ~TrainingPackCatalog() = default;

    // Core CRUD operations
    TrainingEntry* GetByCode(const std::string& code);
    const TrainingEntry* GetByCode(const std::string& code) const;

    const std::vector<TrainingEntry>& GetAll() const;

    bool AddCustomPack(const TrainingEntry& pack);
    bool UpdatePack(const std::string& code, const TrainingEntry& updatedPack);
    bool DeletePack(const std::string& code);

    // Search and filtering
    void FilterAndSortPacks(
        const std::string& searchText,
        const std::string& difficultyFilter,
        const std::string& tagFilter,
        int minShots,
        bool videoFilter,
        int sortColumn,
        bool sortAscending,
        std::vector<TrainingEntry>& out) const;

    // Metadata operations
    void BuildAvailableTags(std::vector<std::string>& out) const;
    void HealPack(const std::string& code, int newShotCount);

    // Persistence
    void LoadFromDisk(const std::filesystem::path& filePath);
    void SaveToDisk(const std::filesystem::path& filePath);

    // Web synchronization
    void SyncWithWebSource(const std::shared_ptr<GameWrapper>& gameWrapper);
    bool IsSyncInProgress() const { return syncInProgress_; }

    // Accessors
    int GetPackCount() const { return packCount_; }
    std::string GetLastUpdated() const { return lastUpdated_; }

private:
    std::vector<TrainingEntry> packs_;
    mutable std::mutex packMutex_;
    int packCount_ = 0;
    std::string lastUpdated_ = "Never";
    bool syncInProgress_ = false;
    std::filesystem::path currentFilePath_;

    // Internal helpers
    void SavePacksToFile(const std::filesystem::path& filePath);
    bool ValidatePack(const TrainingEntry& pack) const;
    bool IsValidDifficulty(const std::string& difficulty) const;
    bool IsValidSource(const std::string& source) const;
};

} // namespace Domain
```

**File: `Domain/Aggregates/TrainingPackCatalog.cpp`**

```cpp
#include "TrainingPackCatalog.h"
#include "../../IMGUI/json.hpp"
#include <algorithm>

namespace Domain {

TrainingPackCatalog::TrainingPackCatalog()
    : packCount_(0), syncInProgress_(false) {}

TrainingEntry* TrainingPackCatalog::GetByCode(const std::string& code) {
    std::lock_guard<std::mutex> lock(packMutex_);
    auto it = std::find_if(packs_.begin(), packs_.end(),
        [&code](const TrainingEntry& pack) { return pack.code == code; });

    return (it != packs_.end()) ? &(*it) : nullptr;
}

const TrainingEntry* TrainingPackCatalog::GetByCode(const std::string& code) const {
    std::lock_guard<std::mutex> lock(packMutex_);
    auto it = std::find_if(packs_.begin(), packs_.end(),
        [&code](const TrainingEntry& pack) { return pack.code == code; });

    return (it != packs_.end()) ? &(*it) : nullptr;
}

const std::vector<TrainingEntry>& TrainingPackCatalog::GetAll() const {
    return packs_;  // Caller should hold lock for iteration
}

bool TrainingPackCatalog::AddCustomPack(const TrainingEntry& pack) {
    if (!ValidatePack(pack)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(packMutex_);

        // Check for duplicate code
        if (GetByCode(pack.code) != nullptr) {
            return false;
        }

        packs_.push_back(pack);
        packCount_++;
    }

    SavePacksToFile(currentFilePath_);
    return true;
}

bool TrainingPackCatalog::UpdatePack(const std::string& code, const TrainingEntry& updatedPack) {
    if (!ValidatePack(updatedPack)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(packMutex_);
        auto it = std::find_if(packs_.begin(), packs_.end(),
            [&code](const TrainingEntry& pack) { return pack.code == code; });

        if (it == packs_.end()) {
            return false;
        }

        *it = updatedPack;
    }

    SavePacksToFile(currentFilePath_);
    return true;
}

bool TrainingPackCatalog::DeletePack(const std::string& code) {
    {
        std::lock_guard<std::mutex> lock(packMutex_);
        auto it = std::find_if(packs_.begin(), packs_.end(),
            [&code](const TrainingEntry& pack) { return pack.code == code; });

        if (it == packs_.end()) {
            return false;
        }

        packs_.erase(it);
        packCount_--;
    }

    SavePacksToFile(currentFilePath_);
    return true;
}

void TrainingPackCatalog::HealPack(const std::string& code, int newShotCount) {
    if (newShotCount < 0) {
        return;  // Invariant violation
    }

    {
        std::lock_guard<std::mutex> lock(packMutex_);
        auto pack = GetByCode(code);
        if (pack) {
            pack->shotCount = newShotCount;
        }
    }

    SavePacksToFile(currentFilePath_);
}

void TrainingPackCatalog::LoadFromDisk(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return;
    }

    try {
        std::ifstream file(filePath);
        auto json = nlohmann::json::parse(file);

        std::vector<TrainingEntry> loaded;
        for (const auto& item : json) {
            TrainingEntry entry{
                .code = item["code"],
                .name = item["name"],
                .creator = item.value("creator", ""),
                .creatorSlug = item.value("creatorSlug", ""),
                .difficulty = item.value("difficulty", ""),
                .tags = item.value("tags", std::vector<std::string>()),
                .shotCount = item.value("shotCount", 0),
                .staffComments = item.value("staffComments", ""),
                .notes = item.value("notes", ""),
                .videoUrl = item.value("videoUrl", ""),
                .likes = item.value("likes", 0),
                .plays = item.value("plays", 0),
                .status = item.value("status", 1),
                .source = item.value("source", "prejump"),
                .isModified = item.value("isModified", false),
            };

            if (ValidatePack(entry)) {
                loaded.push_back(entry);
            }
        }

        {
            std::lock_guard<std::mutex> lock(packMutex_);
            packs_ = loaded;
            packCount_ = packs_.size();
            currentFilePath_ = filePath;
            lastUpdated_ = "Loaded";
        }
    }
    catch (const std::exception& e) {
        // Log error
    }
}

void TrainingPackCatalog::SavePacksToFile(const std::filesystem::path& filePath) {
    try {
        nlohmann::json json = nlohmann::json::array();

        {
            std::lock_guard<std::mutex> lock(packMutex_);
            for (const auto& pack : packs_) {
                json.push_back({
                    {"code", pack.code},
                    {"name", pack.name},
                    {"creator", pack.creator},
                    {"creatorSlug", pack.creatorSlug},
                    {"difficulty", pack.difficulty},
                    {"tags", pack.tags},
                    {"shotCount", pack.shotCount},
                    {"staffComments", pack.staffComments},
                    {"notes", pack.notes},
                    {"videoUrl", pack.videoUrl},
                    {"likes", pack.likes},
                    {"plays", pack.plays},
                    {"status", pack.status},
                    {"source", pack.source},
                    {"isModified", pack.isModified},
                });
            }
        }

        std::ofstream file(filePath);
        file << json.dump(2);
    }
    catch (const std::exception& e) {
        // Log error
    }
}

bool TrainingPackCatalog::ValidatePack(const TrainingEntry& pack) const {
    if (pack.code.empty()) return false;
    if (pack.name.empty()) return false;
    if (!IsValidDifficulty(pack.difficulty) && !pack.difficulty.empty()) return false;
    if (!IsValidSource(pack.source)) return false;
    if (pack.shotCount < 0) return false;
    return true;
}

bool TrainingPackCatalog::IsValidDifficulty(const std::string& difficulty) const {
    static const std::vector<std::string> validLevels = {
        "Bronze", "Gold", "Platinum", "Diamond", "Champion", "Supersonic Legend"
    };
    return std::find(validLevels.begin(), validLevels.end(), difficulty) != validLevels.end();
}

bool TrainingPackCatalog::IsValidSource(const std::string& source) const {
    return source == "prejump" || source == "custom";
}

} // namespace Domain
```

---

### Step 1.2: Create MapRepository Aggregate Root

**File: `Domain/Aggregates/MapRepository.h`**

```cpp
#pragma once
#include <filesystem>
#include <vector>
#include <memory>
#include <mutex>
#include "../MapList.h"

namespace Domain {

/**
 * MapRepository Aggregate Root
 *
 * Manages both freeplay maps and workshop maps.
 *
 * INVARIANTS:
 * - Freeplay map codes must be unique
 * - Workshop file paths must be valid and exist
 * - Metadata must be consistent with actual files
 */
class MapRepository {
public:
    MapRepository();
    ~MapRepository() = default;

    // Freeplay map operations
    const MapEntry* GetFreeplayByCode(const std::string& code) const;
    const std::vector<MapEntry>& GetAllFreeplay() const;

    // Workshop map operations
    const WorkshopEntry* GetWorkshopByPath(const std::filesystem::path& path) const;
    const std::vector<WorkshopEntry>& GetAllWorkshop() const;

    // Discovery
    void DiscoverWorkshopMaps(const std::filesystem::path& rootDir);
    void LoadWorkshopMetadata(WorkshopEntry& entry);
    void ClearWorkshopMaps();

    // Metadata
    int GetWorkshopMapCount() const;
    int GetFreeplayMapCount() const;

private:
    std::vector<MapEntry> freeplayMaps_;
    std::vector<WorkshopEntry> workshopMaps_;
    mutable std::mutex workshopMutex_;

    bool LoadWorkshopMetadata(const std::filesystem::path& jsonPath,
                             std::string& outTitle,
                             std::string& outAuthor,
                             std::string& outDescription) const;

    std::filesystem::path FindPreviewImage(const std::filesystem::path& folder) const;
};

} // namespace Domain
```

---

### Step 1.3: Update SuiteSpot Hub to Use New Aggregates

**File: `SuiteSpot.h` (relevant portions)**

```cpp
class SuiteSpot final : public BakkesMod::Plugin::BakkesModPlugin,
                        public SettingsWindowBase,
                        public BakkesMod::Plugin::PluginWindow
{
    // ... existing code ...

private:
    // NEW: Use aggregates instead of raw managers
    std::unique_ptr<Domain::TrainingPackCatalog> trainingPackCatalog_;
    std::unique_ptr<Domain::MapRepository> mapRepository_;
    std::unique_ptr<Domain::UserConfiguration> userConfig_;
    std::unique_ptr<Domain::LoadoutRegistry> loadoutRegistry_;
    std::unique_ptr<Domain::UsageStatistics> usageStats_;

    // Keep these for now (transitioning)
    WorkshopDownloader* workshopDownloader = nullptr;
    AutoLoadFeature* autoLoadFeature = nullptr;
    SettingsUI* settingsUI = nullptr;
};
```

---

## PHASE 5: Introduce Value Objects

### Step 5.1: Create PackCode Value Object

**File: `Domain/ValueObjects/PackCode.h`**

```cpp
#pragma once
#include <string>
#include <functional>

namespace Domain {

/**
 * PackCode Value Object
 *
 * Represents a training pack identifier in the format: XXXX-XXXX-XXXX-XXXX
 * This is an immutable, validated value that encodes domain knowledge.
 */
class PackCode final {
public:
    static constexpr const char* PATTERN = "^[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}$";

    /**
     * Creates a PackCode from a string.
     * @throws InvalidPackCodeError if format is invalid
     */
    explicit PackCode(const std::string& code);

    // Copy operations (value objects are immutable)
    PackCode(const PackCode&) = default;
    PackCode& operator=(const PackCode&) = default;

    // Move operations
    PackCode(PackCode&&) noexcept = default;
    PackCode& operator=(PackCode&&) noexcept = default;

    // Value object equality
    bool operator==(const PackCode& other) const;
    bool operator!=(const PackCode& other) const;

    // Access the underlying value
    const std::string& ToString() const { return value_; }

    // Hashing support (for use in maps/sets)
    size_t GetHash() const;

private:
    std::string value_;

    static bool IsValidFormat(const std::string& code);
};

} // namespace Domain

// Hash support for PackCode in std::unordered_map
namespace std {
template<>
struct hash<Domain::PackCode> {
    size_t operator()(const Domain::PackCode& code) const {
        return code.GetHash();
    }
};
}
```

**File: `Domain/ValueObjects/PackCode.cpp`**

```cpp
#include "PackCode.h"
#include <regex>
#include <stdexcept>

namespace Domain {

class InvalidPackCodeError : public std::runtime_error {
public:
    InvalidPackCodeError(const std::string& code)
        : std::runtime_error("Invalid pack code format: " + code) {}
};

PackCode::PackCode(const std::string& code) : value_(code) {
    if (!IsValidFormat(code)) {
        throw InvalidPackCodeError(code);
    }
}

bool PackCode::IsValidFormat(const std::string& code) {
    static const std::regex pattern(PATTERN);
    return std::regex_match(code, pattern);
}

bool PackCode::operator==(const PackCode& other) const {
    return value_ == other.value_;
}

bool PackCode::operator!=(const PackCode& other) const {
    return !(*this == other);
}

size_t PackCode::GetHash() const {
    return std::hash<std::string>()(value_);
}

} // namespace Domain
```

---

### Step 5.2: Create Difficulty Value Object

**File: `Domain/ValueObjects/Difficulty.h`**

```cpp
#pragma once
#include <string>

namespace Domain {

/**
 * Difficulty Value Object
 *
 * Represents skill level required for a training pack.
 * Ensures only valid difficulty levels are used throughout the system.
 */
class Difficulty final {
public:
    enum Level {
        Bronze,
        Gold,
        Platinum,
        Diamond,
        Champion,
        SupersonicLegend
    };

    /**
     * Creates a Difficulty from a string.
     * @throws std::invalid_argument if level is not recognized
     */
    explicit Difficulty(const std::string& levelStr);
    explicit Difficulty(Level level);

    // Value object semantics
    Difficulty(const Difficulty&) = default;
    Difficulty& operator=(const Difficulty&) = default;

    // Comparison
    bool operator==(const Difficulty& other) const;
    bool operator!=(const Difficulty& other) const;
    bool operator<(const Difficulty& other) const;

    // Accessors
    Level GetLevel() const { return level_; }
    std::string ToString() const;
    int GetRankingOrder() const;  // 0 = Bronze, 5 = Legend

private:
    Level level_;

    static Level FromString(const std::string& levelStr);
};

} // namespace Domain
```

**File: `Domain/ValueObjects/Difficulty.cpp`**

```cpp
#include "Difficulty.h"
#include <stdexcept>
#include <algorithm>

namespace Domain {

Difficulty::Difficulty(const std::string& levelStr)
    : level_(FromString(levelStr)) {}

Difficulty::Difficulty(Level level)
    : level_(level) {}

Difficulty::Level Difficulty::FromString(const std::string& levelStr) {
    static const std::vector<std::pair<std::string, Level>> mapping = {
        {"Bronze", Level::Bronze},
        {"Gold", Level::Gold},
        {"Platinum", Level::Platinum},
        {"Diamond", Level::Diamond},
        {"Champion", Level::Champion},
        {"Supersonic Legend", Level::SupersonicLegend},
    };

    auto it = std::find_if(mapping.begin(), mapping.end(),
        [&levelStr](const auto& pair) { return pair.first == levelStr; });

    if (it == mapping.end()) {
        throw std::invalid_argument("Unknown difficulty level: " + levelStr);
    }

    return it->second;
}

std::string Difficulty::ToString() const {
    switch (level_) {
        case Level::Bronze: return "Bronze";
        case Level::Gold: return "Gold";
        case Level::Platinum: return "Platinum";
        case Level::Diamond: return "Diamond";
        case Level::Champion: return "Champion";
        case Level::SupersonicLegend: return "Supersonic Legend";
    }
    return "Unknown";
}

int Difficulty::GetRankingOrder() const {
    return static_cast<int>(level_);
}

bool Difficulty::operator==(const Difficulty& other) const {
    return level_ == other.level_;
}

bool Difficulty::operator!=(const Difficulty& other) const {
    return level_ != other.level_;
}

bool Difficulty::operator<(const Difficulty& other) const {
    return level_ < other.level_;
}

} // namespace Domain
```

---

### Step 5.3: Create DelayConfiguration Value Object

**File: `Domain/ValueObjects/DelayConfiguration.h`**

```cpp
#pragma once

namespace Domain {

/**
 * DelayConfiguration Value Object
 *
 * Represents all delay times (in seconds) for different load scenarios.
 * Immutable value object ensuring consistency across the system.
 */
class DelayConfiguration final {
public:
    // Constructor requires all values (forces completeness)
    DelayConfiguration(int queueDelay, int freeplayDelay,
                      int trainingDelay, int workshopDelay);

    // Validation
    static bool IsValidDelay(int delaySec);

    // Accessors
    int GetQueueDelay() const { return queueDelay_; }
    int GetFreeplayDelay() const { return freeplayDelay_; }
    int GetTrainingDelay() const { return trainingDelay_; }
    int GetWorkshopDelay() const { return workshopDelay_; }

    // Conversion to milliseconds
    int GetQueueDelayMs() const { return queueDelay_ * 1000; }
    int GetFreeplayDelayMs() const { return freeplayDelay_ * 1000; }
    int GetTrainingDelayMs() const { return trainingDelay_ * 1000; }
    int GetWorkshopDelayMs() const { return workshopDelay_ * 1000; }

    // Value object equality
    bool operator==(const DelayConfiguration& other) const;
    bool operator!=(const DelayConfiguration& other) const;

private:
    int queueDelay_;       // Delay before queuing (seconds)
    int freeplayDelay_;    // Delay before loading freeplay (seconds)
    int trainingDelay_;    // Delay before loading training (seconds)
    int workshopDelay_;    // Delay before loading workshop (seconds)
};

} // namespace Domain
```

**File: `Domain/ValueObjects/DelayConfiguration.cpp`**

```cpp
#include "DelayConfiguration.h"
#include <stdexcept>

namespace Domain {

DelayConfiguration::DelayConfiguration(int queueDelay, int freeplayDelay,
                                     int trainingDelay, int workshopDelay)
    : queueDelay_(queueDelay),
      freeplayDelay_(freeplayDelay),
      trainingDelay_(trainingDelay),
      workshopDelay_(workshopDelay) {

    if (!IsValidDelay(queueDelay) || !IsValidDelay(freeplayDelay) ||
        !IsValidDelay(trainingDelay) || !IsValidDelay(workshopDelay)) {
        throw std::invalid_argument("Delay must be between 0 and 60 seconds");
    }
}

bool DelayConfiguration::IsValidDelay(int delaySec) {
    return delaySec >= 0 && delaySec <= 60;
}

bool DelayConfiguration::operator==(const DelayConfiguration& other) const {
    return queueDelay_ == other.queueDelay_ &&
           freeplayDelay_ == other.freeplayDelay_ &&
           trainingDelay_ == other.trainingDelay_ &&
           workshopDelay_ == other.workshopDelay_;
}

bool DelayConfiguration::operator!=(const DelayConfiguration& other) const {
    return !(*this == other);
}

} // namespace Domain
```

---

## PHASE 2: Create Anti-Corruption Layers

### Step 2.1: Create BakkesMod Game Adapter

**File: `Infrastructure/Adapters/BakkesModGameAdapter.h`**

```cpp
#pragma once
#include <string>
#include <memory>
#include <functional>
#include "bakkesmod/plugin/bakkesmodplugin.h"

namespace Infrastructure {

/**
 * Anti-Corruption Layer for BakkesMod Game Integration
 *
 * This adapter shields the domain layer from BakkesMod SDK details.
 * The domain layer sees a clean interface, while this class handles
 * all BakkesMod-specific complexity.
 */
class BakkesModGameAdapter {
public:
    explicit BakkesModGameAdapter(std::shared_ptr<GameWrapper> gameWrapper);
    ~BakkesModGameAdapter() = default;

    /**
     * Execute a BakkesMod command after a delay.
     * @param command The console command to execute (e.g., "load_workshop map.upk")
     * @param delayMs Time to wait before executing (milliseconds)
     */
    void ExecuteCommandWithDelay(const std::string& command, int delayMs);

    /**
     * Execute a BakkesMod command immediately (on game thread).
     * @param command The console command to execute
     */
    void ExecuteCommandImmediate(const std::string& command);

    /**
     * Apply a car loadout by name.
     * @param loadoutName Name of the loadout to switch to
     * @param onComplete Callback when operation finishes
     */
    void ApplyCameraSettings(const std::string& loadoutName,
                           std::function<void(bool)> onComplete = nullptr);

    /**
     * Register a console variable.
     * @param name CVar name (e.g., "suitespot_enabled")
     * @param defaultValue Default value
     * @param description CVar description
     */
    void RegisterCVar(const std::string& name,
                     const std::string& defaultValue,
                     const std::string& description);

    /**
     * Get current CVar value.
     * @param name CVar name
     * @return Current value, empty if not found
     */
    std::string GetCVarValue(const std::string& name) const;

    /**
     * Set CVar value.
     * @param name CVar name
     * @param value New value
     */
    void SetCVarValue(const std::string& name, const std::string& value);

    /**
     * Check if game is running/playable.
     * @return true if game is in a playable state
     */
    bool IsGameReady() const;

private:
    std::shared_ptr<GameWrapper> gameWrapper_;
};

} // namespace Infrastructure
```

**File: `Infrastructure/Adapters/BakkesModGameAdapter.cpp`**

```cpp
#include "BakkesModGameAdapter.h"

namespace Infrastructure {

BakkesModGameAdapter::BakkesModGameAdapter(std::shared_ptr<GameWrapper> gameWrapper)
    : gameWrapper_(gameWrapper) {}

void BakkesModGameAdapter::ExecuteCommandWithDelay(const std::string& command, int delayMs) {
    if (!gameWrapper_) return;

    gameWrapper_->SetTimeout([this, command](GameWrapper* gw) {
        ExecuteCommandImmediate(command);
    }, delayMs / 1000.0f);  // Convert ms to seconds for BakkesMod API
}

void BakkesModGameAdapter::ExecuteCommandImmediate(const std::string& command) {
    if (!gameWrapper_) return;

    gameWrapper_->Execute([this, command](GameWrapper* gw) {
        gw->ExecuteUnrealCommand(command);
    });
}

void BakkesModGameAdapter::ApplyCameraSettings(const std::string& loadoutName,
                                              std::function<void(bool)> onComplete) {
    if (!gameWrapper_) {
        if (onComplete) onComplete(false);
        return;
    }

    // This would call the actual loadout switching logic
    // Implementation depends on BakkesMod's loadout API
}

void BakkesModGameAdapter::RegisterCVar(const std::string& name,
                                       const std::string& defaultValue,
                                       const std::string& description) {
    if (!gameWrapper_) return;

    auto cvarManager = gameWrapper_->GetCVarManager();
    if (!cvarManager) return;

    cvarManager->registerCvar(name, defaultValue, description, true, true,
                             std::numeric_limits<float>::lowest(),
                             std::numeric_limits<float>::max());
}

std::string BakkesModGameAdapter::GetCVarValue(const std::string& name) const {
    if (!gameWrapper_) return "";

    auto cvarManager = gameWrapper_->GetCVarManager();
    if (!cvarManager) return "";

    auto cvar = cvarManager->getCvar(name);
    if (!cvar) return "";

    return cvar.getStringValue();
}

void BakkesModGameAdapter::SetCVarValue(const std::string& name, const std::string& value) {
    if (!gameWrapper_) return;

    auto cvarManager = gameWrapper_->GetCVarManager();
    if (!cvarManager) return;

    auto cvar = cvarManager->getCvar(name);
    if (cvar) {
        cvar.setValue(value);
    }
}

bool BakkesModGameAdapter::IsGameReady() const {
    if (!gameWrapper_) return false;
    // Add actual game state check
    return true;
}

} // namespace Infrastructure
```

---

### Step 2.2: Create File System Adapter

**File: `Infrastructure/Adapters/FileSystemAdapter.h`**

```cpp
#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "../IMGUI/json.hpp"

namespace Infrastructure {

/**
 * Anti-Corruption Layer for File System Operations
 *
 * Provides a clean interface for file system access, isolating
 * the domain layer from std::filesystem details.
 */
class FileSystemAdapter {
public:
    FileSystemAdapter();
    ~FileSystemAdapter() = default;

    // Path operations
    std::filesystem::path GetBakkesModDataPath() const;
    std::filesystem::path GetSuiteSpotDataPath() const;

    // Directory operations
    bool EnsureDirectoryExists(const std::filesystem::path& path) const;
    std::vector<std::filesystem::path> ListFilesInDirectory(
        const std::filesystem::path& dir,
        const std::string& extension = "") const;

    // JSON operations
    nlohmann::json LoadJsonFile(const std::filesystem::path& path) const;
    void SaveJsonFile(const std::filesystem::path& path,
                      const nlohmann::json& data) const;

    // File operations
    bool FileExists(const std::filesystem::path& path) const;
    bool DirectoryExists(const std::filesystem::path& path) const;
    bool DeleteFile(const std::filesystem::path& path) const;

    // Preview image operations
    std::filesystem::path FindPreviewImage(const std::filesystem::path& dir) const;

private:
    std::filesystem::path bakkesmodDataPath_;

    void InitializeBakkesModPath();
    bool IsValidImageExtension(const std::string& ext) const;
};

} // namespace Infrastructure
```

---

## PHASE 3: Implement Repository Pattern

### Step 3.1: Create Repository Interfaces

**File: `Infrastructure/Repositories/ITrainingPackRepository.h`**

```cpp
#pragma once
#include <vector>
#include <memory>
#include <filesystem>
#include "../../Domain/MapList.h"

namespace Infrastructure {

/**
 * Repository interface for Training Pack persistence.
 *
 * Abstracts the data access layer, allowing multiple implementations
 * (file system, database, network, etc.)
 */
class ITrainingPackRepository {
public:
    virtual ~ITrainingPackRepository() = default;

    virtual std::vector<TrainingEntry> GetAll() const = 0;
    virtual TrainingEntry* GetByCode(const std::string& code) = 0;
    virtual const TrainingEntry* GetByCode(const std::string& code) const = 0;

    virtual void Add(const TrainingEntry& pack) = 0;
    virtual void Update(const std::string& code, const TrainingEntry& pack) = 0;
    virtual void Delete(const std::string& code) = 0;

    virtual void LoadFromDisk(const std::filesystem::path& path) = 0;
    virtual void SaveToDisk(const std::filesystem::path& path) = 0;

    virtual int GetPackCount() const = 0;
};

} // namespace Infrastructure
```

**File: `Infrastructure/Repositories/FileSystemTrainingPackRepository.h`**

```cpp
#pragma once
#include "ITrainingPackRepository.h"
#include <mutex>

namespace Infrastructure {

/**
 * File System Implementation of Training Pack Repository
 *
 * Persists training packs to JSON files on disk.
 */
class FileSystemTrainingPackRepository : public ITrainingPackRepository {
public:
    explicit FileSystemTrainingPackRepository(const std::filesystem::path& filePath);
    ~FileSystemTrainingPackRepository() override = default;

    std::vector<TrainingEntry> GetAll() const override;
    TrainingEntry* GetByCode(const std::string& code) override;
    const TrainingEntry* GetByCode(const std::string& code) const override;

    void Add(const TrainingEntry& pack) override;
    void Update(const std::string& code, const TrainingEntry& pack) override;
    void Delete(const std::string& code) override;

    void LoadFromDisk(const std::filesystem::path& path) override;
    void SaveToDisk(const std::filesystem::path& path) override;

    int GetPackCount() const override;

private:
    std::filesystem::path filePath_;
    std::vector<TrainingEntry> packs_;
    mutable std::mutex mutex_;

    void SaveToFile() const;
};

} // namespace Infrastructure
```

---

## PHASE 4: Add Domain Events

### Step 4.1: Create Event System

**File: `Domain/Events/DomainEvent.h`**

```cpp
#pragma once
#include <string>
#include <ctime>

namespace Domain {

/**
 * Base class for all domain events.
 *
 * Domain events represent things that have happened in the domain.
 * They provide a way for aggregates to communicate without direct coupling.
 */
class DomainEvent {
public:
    virtual ~DomainEvent() = default;

    virtual std::string GetEventType() const = 0;
    std::time_t GetTimestamp() const { return timestamp_; }

protected:
    DomainEvent() : timestamp_(std::time(nullptr)) {}

private:
    std::time_t timestamp_;
};

} // namespace Domain
```

**File: `Domain/Events/PackLoadedEvent.h`**

```cpp
#pragma once
#include "DomainEvent.h"

namespace Domain {

/**
 * Event published when a training pack is loaded.
 *
 * Subscribers can use this to update usage statistics, analytics, etc.
 */
class PackLoadedEvent : public DomainEvent {
public:
    explicit PackLoadedEvent(const std::string& packCode)
        : packCode_(packCode) {}

    std::string GetEventType() const override {
        return "PackLoaded";
    }

    const std::string& GetPackCode() const { return packCode_; }

private:
    std::string packCode_;
};

} // namespace Domain
```

**File: `Domain/Events/EventPublisher.h`**

```cpp
#pragma once
#include <typeinfo>
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include "DomainEvent.h"

namespace Domain {

/**
 * Event Publisher for Domain Events.
 *
 * Allows aggregates and services to publish events that other
 * components can subscribe to without direct coupling.
 */
class EventPublisher {
public:
    using Handler = std::function<void(const DomainEvent&)>;

    EventPublisher() = default;
    ~EventPublisher() = default;

    /**
     * Subscribe to events of a specific type.
     * @tparam TEvent Event type to subscribe to
     * @param handler Function to call when event is published
     */
    template<typename TEvent>
    void Subscribe(Handler handler) {
        auto& typeKey = typeid(TEvent);
        handlers_[typeKey.name()].push_back(handler);
    }

    /**
     * Publish an event to all subscribers.
     * @param event Event to publish
     */
    void Publish(const DomainEvent& event) {
        auto& typeKey = typeid(event);
        auto it = handlers_.find(typeKey.name());

        if (it != handlers_.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }

    void Clear() {
        handlers_.clear();
    }

private:
    std::map<std::string, std::vector<Handler>> handlers_;
};

} // namespace Domain
```

---

## INTEGRATION CHECKLIST

### Phase 1: Aggregates
- [ ] Create TrainingPackCatalog aggregate root
- [ ] Create MapRepository aggregate root
- [ ] Create UserConfiguration aggregate root
- [ ] Create LoadoutRegistry aggregate root
- [ ] Update SuiteSpot hub to instantiate aggregates
- [ ] Update AutoLoadFeature to use aggregates
- [ ] Test basic aggregate operations

### Phase 5: Value Objects
- [ ] Create PackCode value object
- [ ] Create Difficulty value object
- [ ] Create DelayConfiguration value object
- [ ] Create Creator value object
- [ ] Update aggregates to use value objects
- [ ] Update persistence layer to serialize value objects

### Phase 2: Anti-Corruption Layers
- [ ] Create BakkesModGameAdapter
- [ ] Create FileSystemAdapter
- [ ] Create BakkesModSettingsAdapter
- [ ] Update SuiteSpot to inject adapters
- [ ] Remove direct BakkesMod SDK usage from domain

### Phase 3: Repositories
- [ ] Create ITrainingPackRepository interface
- [ ] Create FileSystemTrainingPackRepository
- [ ] Create IMapRepository interface
- [ ] Create FileSystemMapRepository
- [ ] Inject repositories into aggregates
- [ ] Update SuiteSpot to use repositories

### Phase 4: Domain Events
- [ ] Create DomainEvent base class
- [ ] Create PackLoadedEvent
- [ ] Create other domain events
- [ ] Create EventPublisher
- [ ] Wire event publishing into aggregates
- [ ] Subscribe to events in infrastructure layer

---

**End of Implementation Guide**

