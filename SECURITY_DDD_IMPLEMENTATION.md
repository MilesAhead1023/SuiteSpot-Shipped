# Security-First DDD Implementation Guide
## SuiteSpot - Secure Architecture Through Domain-Driven Design

**Purpose:** Detailed implementation roadmap for integrating security hardening with DDD refactoring

**Target Audience:** Development team, architects, security reviewers

**Timeline:** 3 weeks parallel with critical security fixes (Week 0-3)

---

## PHASE 1: AGGREGATES WITH INPUT VALIDATION (Days 1-2)

### Objective
Establish aggregate roots that enforce invariants and validate all inputs at boundaries.

### TrainingPackCatalog Aggregate

```cpp
// training_pack_catalog.h
#pragma once

#include <vector>
#include <mutex>
#include <string>
#include <memory>
#include "training_entry.h"

namespace domain::training {

// Value Object for pack code
class PackCode {
private:
    std::string value;

    explicit PackCode(const std::string& code) : value(code) {}

public:
    static Result<PackCode> Create(const std::string& code);

    const std::string& ToString() const { return value; }

    bool operator==(const PackCode& other) const {
        return value == other.value;
    }
};

// Value Object for pack name
class PackName {
private:
    std::string value;

    explicit PackName(const std::string& name) : value(name) {}

public:
    static Result<PackName> Create(const std::string& name);

    const std::string& ToString() const { return value; }
};

// Aggregate Root: TrainingPackCatalog
class TrainingPackCatalog {
private:
    std::vector<TrainingEntry> packs;
    mutable std::mutex mutex;

    // Private constructor - use factory methods
    explicit TrainingPackCatalog() = default;

    // Invariant validation methods
    bool ValidatePackBeforeAdding(const TrainingEntry& pack) const;
    bool ValidatePackCode(const std::string& code) const;
    bool ValidatePackMetadata(const TrainingEntry& pack) const;

public:
    // Factory method
    static Result<std::shared_ptr<TrainingPackCatalog>> Create() {
        return Result::Success(
            std::make_shared<TrainingPackCatalog>()
        );
    }

    // Main operations - all validate invariants
    Result<void> AddPack(const TrainingEntry& pack);
    Result<void> UpdatePack(const std::string& code, const TrainingEntry& updated);
    Result<void> RemovePack(const std::string& code);

    // Query operations
    Result<std::vector<TrainingEntry>> GetAllPacks() const;
    Result<TrainingEntry> GetPackByCode(const std::string& code) const;
    Result<std::vector<TrainingEntry>> SearchPacks(const std::string& query) const;

    // Batch operations (for migration)
    Result<void> ReplaceAllPacks(const std::vector<TrainingEntry>& newPacks);

    // Event publishing
    void PublishPackAddedEvent(const TrainingEntry& pack);
    void PublishPackUpdatedEvent(const std::string& code, const TrainingEntry& pack);
    void PublishPackDeletedEvent(const std::string& code);

    // Size/count for monitoring
    size_t GetPackCount() const;
};

} // namespace domain::training
```

#### Implementation Details

```cpp
// training_pack_catalog.cpp

namespace domain::training {

Result<PackCode> PackCode::Create(const std::string& code) {
    // Validate format: XXXX-XXXX-XXXX-XXXX where X is hex digit
    static const std::regex PACK_CODE_PATTERN(
        "^[0-9A-Fa-f]{4}(-[0-9A-Fa-f]{4}){3}$"
    );

    if (!std::regex_match(code, PACK_CODE_PATTERN)) {
        return Result::Error(
            "Invalid pack code format. Expected XXXX-XXXX-XXXX-XXXX"
        );
    }

    return Result::Success(PackCode(code));
}

Result<PackName> PackName::Create(const std::string& name) {
    // Validate name constraints
    if (name.empty()) {
        return Result::Error("Pack name cannot be empty");
    }

    if (name.length() > 512) {
        return Result::Error("Pack name too long (max 512 chars)");
    }

    // Remove control characters
    std::string clean = name;
    clean.erase(
        std::remove_if(clean.begin(), clean.end(),
                      [](unsigned char c) { return std::iscntrl(c); }),
        clean.end()
    );

    if (clean.empty()) {
        return Result::Error("Pack name contains only control characters");
    }

    return Result::Success(PackName(clean));
}

bool TrainingPackCatalog::ValidatePackBeforeAdding(const TrainingEntry& pack) const {
    // 1. Validate code
    if (auto result = PackCode::Create(pack.code); !result.IsSuccess()) {
        LOG("Invalid pack code: {}", pack.code);
        return false;
    }

    // 2. Validate name
    if (auto result = PackName::Create(pack.name); !result.IsSuccess()) {
        LOG("Invalid pack name: {}", pack.name);
        return false;
    }

    // 3. Check for duplicates
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& existing : packs) {
            if (existing.code == pack.code) {
                LOG("Pack with code {} already exists", pack.code);
                return false;
            }
        }
    }

    // 4. Validate metadata
    return ValidatePackMetadata(pack);
}

bool TrainingPackCatalog::ValidatePackMetadata(const TrainingEntry& pack) const {
    // Shot count must be positive
    if (pack.shotCount <= 0) {
        LOG("Invalid shot count: {}", pack.shotCount);
        return false;
    }

    // Creator name should be reasonable
    if (pack.creator.length() > 256) {
        LOG("Creator name too long");
        return false;
    }

    // Difficulty should be from known set
    static const std::vector<std::string> VALID_DIFFICULTIES = {
        "Unranked", "Bronze", "Silver", "Gold", "Platinum",
        "Diamond", "Champion", "Grand Champion", "Supersonic Legend"
    };

    if (!pack.difficulty.empty()) {
        bool validDifficulty = false;
        for (const auto& valid : VALID_DIFFICULTIES) {
            if (pack.difficulty == valid) {
                validDifficulty = true;
                break;
            }
        }
        if (!validDifficulty) {
            LOG("Unknown difficulty: {}", pack.difficulty);
            return false;
        }
    }

    return true;
}

Result<void> TrainingPackCatalog::AddPack(const TrainingEntry& pack) {
    // Validate at boundary before modification
    if (!ValidatePackBeforeAdding(pack)) {
        return Result::Error("Pack validation failed");
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        packs.push_back(pack);
        // Maintain sorted order
        std::sort(packs.begin(), packs.end(),
                 [](const TrainingEntry& a, const TrainingEntry& b) {
                     return a.name < b.name;
                 });
    }

    // Publish domain event (outside lock)
    PublishPackAddedEvent(pack);

    return Result::Success();
}

Result<void> TrainingPackCatalog::UpdatePack(
    const std::string& code,
    const TrainingEntry& updated) {

    // Validate new pack data
    if (!ValidatePackBeforeAdding(updated)) {
        return Result::Error("Updated pack validation failed");
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = std::find_if(packs.begin(), packs.end(),
            [&code](const TrainingEntry& p) { return p.code == code; });

        if (it == packs.end()) {
            return Result::Error("Pack not found");
        }

        // Update in place
        *it = updated;
    }

    PublishPackUpdatedEvent(code, updated);
    return Result::Success();
}

Result<std::vector<TrainingEntry>> TrainingPackCatalog::GetAllPacks() const {
    std::lock_guard<std::mutex> lock(mutex);
    return Result::Success(packs);
}

} // namespace domain::training
```

### MapRepository Aggregate

```cpp
// map_repository.h
#pragma once

#include <vector>
#include <filesystem>
#include <optional>
#include "map_entry.h"

namespace domain::maps {

// Value Object for map code
class MapCode {
private:
    std::string value;
    explicit MapCode(const std::string& code) : value(code) {}

public:
    static Result<MapCode> Create(const std::string& code);
    const std::string& ToString() const { return value; }
};

// Value Object for map file path
class MapFilePath {
private:
    std::filesystem::path value;
    explicit MapFilePath(const std::filesystem::path& path) : value(path) {}

public:
    // Validates path is within allowed directory
    static Result<MapFilePath> Create(const std::filesystem::path& path,
                                      const std::filesystem::path& allowedRoot);

    const std::filesystem::path& ToPath() const { return value; }
};

// Aggregate Root: MapRepository
class MapRepository {
private:
    std::vector<MapEntry> freeplayMaps;
    std::vector<WorkshopEntry> workshopMaps;
    mutable std::mutex mapsMutex;

    std::filesystem::path workshopRoot;

    explicit MapRepository(const std::filesystem::path& root)
        : workshopRoot(root) {}

    bool ValidateMapBeforeAdding(const MapEntry& map) const;
    bool ValidateWorkshopMapBeforeAdding(const WorkshopEntry& map) const;

public:
    static Result<std::shared_ptr<MapRepository>> Create(
        const std::filesystem::path& workshopRoot);

    // Freeplay map operations
    Result<void> AddFreeplayMap(const MapEntry& map);
    Result<std::vector<MapEntry>> GetFreeplayMaps() const;

    // Workshop map operations
    Result<void> AddWorkshopMap(const WorkshopEntry& map);
    Result<std::vector<WorkshopEntry>> GetWorkshopMaps() const;

    // Safety: Validates path before returning
    Result<WorkshopEntry> GetWorkshopMapByPath(const std::string& path) const;
};

} // namespace domain::maps
```

#### Secure Path Validation

```cpp
Result<MapFilePath> MapFilePath::Create(
    const std::filesystem::path& path,
    const std::filesystem::path& allowedRoot) {

    std::error_code ec;

    // 1. Verify path exists
    if (!std::filesystem::exists(path, ec)) {
        return Result::Error("Map file does not exist");
    }

    // 2. Verify it's a regular file
    if (!std::filesystem::is_regular_file(path, ec)) {
        return Result::Error("Path is not a regular file");
    }

    // 3. Get canonical (resolved) path
    auto canonical = std::filesystem::canonical(path, ec);
    if (ec) {
        return Result::Error("Failed to resolve canonical path");
    }

    // 4. Get canonical root
    auto canonical_root = std::filesystem::canonical(allowedRoot, ec);
    if (ec) {
        return Result::Error("Failed to resolve root path");
    }

    // 5. Verify path is within root directory
    std::string canonical_str = canonical.string();
    std::string root_str = canonical_root.string();

    // Ensure it starts with root AND has proper separator
    if (canonical_str.find(root_str) != 0) {
        return Result::Error("Map file is outside allowed directory");
    }

    // If root is "C:\\workshop" and canonical is "C:\\workshop\\map.upk",
    // this is OK. But if canonical is "C:\\workshopmods\\map.upk", it should fail.
    if (canonical_str.length() > root_str.length()) {
        char next_char = canonical_str[root_str.length()];
        if (next_char != std::filesystem::path::preferred_separator) {
            return Result::Error("Path traversal attempt detected");
        }
    } else if (canonical_str.length() < root_str.length()) {
        return Result::Error("Map file path is shorter than root");
    }

    return Result::Success(MapFilePath(canonical));
}
```

---

## PHASE 5: VALUE OBJECTS (Days 2-3)

### Objective
Create immutable value objects that eliminate invalid states at the type system level.

### Core Value Objects

```cpp
// value_objects.h
#pragma once

#include <string>
#include <chrono>

namespace domain::values {

// Difficulty level value object
class Difficulty {
private:
    std::string value;
    explicit Difficulty(const std::string& diff) : value(diff) {}

public:
    enum class Level {
        Unranked, Bronze, Silver, Gold, Platinum,
        Diamond, Champion, GrandChampion, SupersonicLegend
    };

    static Result<Difficulty> Create(const std::string& difficulty);

    const std::string& ToString() const { return value; }
    Level GetLevel() const;

    bool operator==(const Difficulty& other) const {
        return value == other.value;
    }

    bool operator<(const Difficulty& other) const {
        return GetLevel() < other.GetLevel();
    }
};

// Configuration delay (in seconds)
class DelayConfiguration {
private:
    int seconds;
    explicit DelayConfiguration(int secs) : seconds(secs) {}

public:
    static Result<DelayConfiguration> Create(int seconds) {
        if (seconds < 0 || seconds > 300) {  // 0-5 minutes
            return Result::Error("Delay must be between 0-300 seconds");
        }
        return Result::Success(DelayConfiguration(seconds));
    }

    int GetSeconds() const { return seconds; }

    // For use in game commands
    float GetAsFloat() const {
        return (seconds <= 0) ? 0.1f : static_cast<float>(seconds);
    }
};

// Maximum concurrent downloads
class ConcurrencyLimit {
private:
    size_t limit;
    explicit ConcurrencyLimit(size_t l) : limit(l) {}

public:
    static Result<ConcurrencyLimit> Create(size_t limit) {
        if (limit == 0 || limit > 10) {
            return Result::Error("Concurrency limit must be 1-10");
        }
        return Result::Success(ConcurrencyLimit(limit));
    }

    size_t GetLimit() const { return limit; }
};

// File size limit (in bytes)
class FileSizeLimit {
private:
    size_t bytes;
    explicit FileSizeLimit(size_t b) : bytes(b) {}

public:
    static Result<FileSizeLimit> Create(size_t sizeInBytes) {
        constexpr size_t MAX_WORKSHOP_SIZE = 500 * 1024 * 1024;  // 500MB
        if (sizeInBytes == 0 || sizeInBytes > MAX_WORKSHOP_SIZE) {
            return Result::Error("File size out of range");
        }
        return Result::Success(FileSizeLimit(sizeInBytes));
    }

    size_t GetBytes() const { return bytes; }
    bool Exceeds(size_t actualSize) const { return actualSize > bytes; }
};

} // namespace domain::values
```

### Using Value Objects in Aggregates

```cpp
// Updated TrainingPackCatalog using value objects
class TrainingPackCatalog {
private:
    // ... previous members ...

    // Enhanced with value objects
    Result<void> UpdateSettings(
        const domain::values::DelayConfiguration& delay,
        const domain::values::ConcurrencyLimit& limit) {

        std::lock_guard<std::mutex> lock(mutex);

        // These are now type-safe - no way to create invalid values
        settings.delaySeconds = delay.GetSeconds();
        settings.maxConcurrentOps = limit.GetLimit();

        return Result::Success();
    }
};
```

---

## PHASE 2: ANTI-CORRUPTION LAYERS (Days 4-5)

### Objective
Isolate domain logic from external dependencies (BakkesMod SDK, untrusted APIs).

### BakkesMod Adapter

```cpp
// bakkesmod_adapter.h
#pragma once

#include "domain/training_pack_catalog.h"
#include "bakkesmod/wrappers/GameWrapper.h"

namespace infrastructure::bakkesmod {

// Anti-corruption layer: Adapts BakkesMod types to domain types
class GameCommandAdapter {
private:
    std::shared_ptr<CVarManagerWrapper> cvarManager;

    // Whitelist of allowed commands - prevents injection
    static constexpr std::array<std::string_view, 4> ALLOWED_COMMANDS = {
        "load_training",
        "load_freeplay",
        "load_workshop",
        "queue"
    };

    bool IsCommandAllowed(const std::string& command) const;
    Result<std::string> ValidateGameCommand(const std::string& cmd);

public:
    explicit GameCommandAdapter(
        std::shared_ptr<CVarManagerWrapper> mgr) : cvarManager(mgr) {}

    // Domain operations translated to game commands
    Result<void> ExecuteAutoLoad(
        const domain::training::TrainingPackCatalog& catalog,
        const std::string& packCode) {

        // 1. Validate pack code format
        auto codeResult = domain::training::PackCode::Create(packCode);
        if (!codeResult.IsSuccess()) {
            return Result::Error("Invalid pack code");
        }

        // 2. Build sanitized command
        std::string command = "load_training " + packCode;

        // 3. Final validation
        auto validation = ValidateGameCommand(command);
        if (!validation.IsSuccess()) {
            return validation;
        }

        // 4. Execute through CVar manager
        try {
            cvarManager->executeCommand(validation.GetValue());
            return Result::Success();
        } catch (const std::exception& e) {
            return Result::Error(std::string("Game command failed: ") + e.what());
        }
    }

    Result<void> ExecuteWorkshopLoad(
        const std::string& filePath) {

        // Validate path using domain value object
        auto pathResult = domain::maps::MapFilePath::Create(
            filePath,
            GetWorkshopRoot()
        );
        if (!pathResult.IsSuccess()) {
            return Result::Error("Invalid workshop path");
        }

        // Build command with quoted path
        std::string command = std::string("load_workshop \"") +
                            pathResult.GetValue().ToPath().string() + "\"";

        auto validation = ValidateGameCommand(command);
        if (!validation.IsSuccess()) return validation;

        try {
            cvarManager->executeCommand(validation.GetValue());
            return Result::Success();
        } catch (const std::exception& e) {
            return Result::Error(std::string("Workshop load failed: ") + e.what());
        }
    }

    Result<void> QueueMatch() {
        auto validation = ValidateGameCommand("queue");
        if (!validation.IsSuccess()) return validation;

        try {
            cvarManager->executeCommand("queue");
            return Result::Success();
        } catch (const std::exception& e) {
            return Result::Error(std::string("Queue failed: ") + e.what());
        }
    }
};

} // namespace infrastructure::bakkesmod
```

### External API Adapter

```cpp
// external_api_adapter.h
#pragma once

#include "domain/training_pack_catalog.h"
#include "nlohmann/json.hpp"

namespace infrastructure::external_api {

// Anti-corruption: Validates and transforms untrusted API responses
class RLMapsApiAdapter {
private:
    // Resource limits for untrusted input
    static constexpr size_t MAX_JSON_SIZE = 1048576;      // 1MB
    static constexpr int MAX_JSON_DEPTH = 10;
    static constexpr int MAX_MAPS_PER_RESPONSE = 100;

    // Transformers: API JSON → Domain objects
    Result<domain::training::TrainingEntry> MapApiResponseToDomain(
        const nlohmann::json& apiResponse);

    bool ValidateJsonSize(const std::string& json);
    Result<nlohmann::json> ParseJsonWithLimits(const std::string& json);

public:
    // Main entry point for API responses
    Result<std::vector<domain::training::TrainingEntry>>
    TransformSearchResults(const std::string& apiResponse);
};

} // namespace infrastructure::external_api
```

#### Detailed Implementation

```cpp
// external_api_adapter.cpp

namespace infrastructure::external_api {

bool RLMapsApiAdapter::ValidateJsonSize(const std::string& json) {
    if (json.size() > MAX_JSON_SIZE) {
        LOG("JSON response too large: {} > {}", json.size(), MAX_JSON_SIZE);
        return false;
    }
    return true;
}

Result<nlohmann::json> RLMapsApiAdapter::ParseJsonWithLimits(
    const std::string& json) {

    if (!ValidateJsonSize(json)) {
        return Result::Error("JSON size exceeds limit");
    }

    try {
        // Parse with depth tracking
        int depth = 0;
        auto callback = [&depth](int current_depth, nlohmann::json::parse_event_t,
                                 nlohmann::json&) -> bool {
            depth = std::max(depth, current_depth);
            if (depth > MAX_JSON_DEPTH) {
                throw std::runtime_error("JSON depth exceeds limit");
            }
            return true;
        };

        auto parsed = nlohmann::json::parse(json, callback);
        return Result::Success(parsed);

    } catch (const std::exception& e) {
        return Result::Error(std::string("JSON parse error: ") + e.what());
    }
}

Result<domain::training::TrainingEntry>
RLMapsApiAdapter::MapApiResponseToDomain(const nlohmann::json& raw) {
    domain::training::TrainingEntry entry;

    try {
        // 1. Extract and validate code
        if (!raw.contains("code") || !raw["code"].is_string()) {
            return Result::Error("Missing or invalid 'code' field");
        }
        entry.code = raw["code"].get<std::string>();

        auto codeValidation = domain::training::PackCode::Create(entry.code);
        if (!codeValidation.IsSuccess()) {
            return Result::Error("Invalid pack code from API: " + entry.code);
        }

        // 2. Extract and validate name
        if (!raw.contains("name") || !raw["name"].is_string()) {
            return Result::Error("Missing or invalid 'name' field");
        }
        entry.name = raw["name"].get<std::string>();

        auto nameValidation = domain::training::PackName::Create(entry.name);
        if (!nameValidation.IsSuccess()) {
            return Result::Error("Invalid pack name from API");
        }

        // 3. Optional fields with defaults
        if (raw.contains("creator") && raw["creator"].is_string()) {
            entry.creator = raw["creator"].get<std::string>();
            // Truncate if too long
            if (entry.creator.length() > 256) {
                entry.creator = entry.creator.substr(0, 256);
            }
        }

        if (raw.contains("difficulty") && raw["difficulty"].is_string()) {
            auto diff = domain::values::Difficulty::Create(
                raw["difficulty"].get<std::string>()
            );
            if (diff.IsSuccess()) {
                entry.difficulty = diff.GetValue().ToString();
            }
        }

        if (raw.contains("shotCount") && raw["shotCount"].is_number()) {
            entry.shotCount = raw["shotCount"].get<int>();
            if (entry.shotCount < 0) entry.shotCount = 0;
        }

        return Result::Success(entry);

    } catch (const std::exception& e) {
        return Result::Error(std::string("Failed to map API response: ") + e.what());
    }
}

Result<std::vector<domain::training::TrainingEntry>>
RLMapsApiAdapter::TransformSearchResults(const std::string& apiResponse) {
    // 1. Validate and parse JSON
    auto parseResult = ParseJsonWithLimits(apiResponse);
    if (!parseResult.IsSuccess()) {
        return Result::Error(parseResult.GetError());
    }

    auto json = parseResult.GetValue();

    // 2. Ensure it's an array
    if (!json.is_array()) {
        return Result::Error("API response is not an array");
    }

    if (json.size() > MAX_MAPS_PER_RESPONSE) {
        LOG("API returned too many maps: {} > {}",
            json.size(), MAX_MAPS_PER_RESPONSE);
        return Result::Error("API response too large");
    }

    // 3. Transform each element
    std::vector<domain::training::TrainingEntry> results;
    for (size_t i = 0; i < json.size(); ++i) {
        auto elementResult = MapApiResponseToDomain(json[i]);
        if (elementResult.IsSuccess()) {
            results.push_back(elementResult.GetValue());
        } else {
            LOG("Skipping invalid map in position {}: {}", i, elementResult.GetError());
            // Skip invalid entries rather than failing entire response
        }
    }

    return Result::Success(results);
}

} // namespace infrastructure::external_api
```

---

## PHASE 4: DOMAIN EVENTS (Days 5-6)

### Objective
Publish events for all domain changes to enable audit logging and monitoring.

### Event Definitions

```cpp
// domain_events.h
#pragma once

#include <string>
#include <chrono>
#include <memory>

namespace domain::events {

// Base event
class DomainEvent {
public:
    virtual ~DomainEvent() = default;

    const std::string& GetEventType() const { return eventType; }
    std::chrono::system_clock::time_point GetTimestamp() const { return timestamp; }

protected:
    std::string eventType;
    std::chrono::system_clock::time_point timestamp =
        std::chrono::system_clock::now();
};

// Specific security-relevant events
class PackAddedEvent : public DomainEvent {
public:
    std::string packCode;
    std::string packName;
    std::string source;  // "api" or "custom"

    PackAddedEvent(const std::string& code, const std::string& name,
                   const std::string& src)
        : packCode(code), packName(name), source(src) {
        eventType = "PackAdded";
    }
};

class PackLoadedEvent : public DomainEvent {
public:
    std::string packCode;
    std::string packName;
    std::string triggeredBy;  // "auto_load" or "manual_load"

    PackLoadedEvent(const std::string& code, const std::string& name,
                    const std::string& trigger)
        : packCode(code), packName(name), triggeredBy(trigger) {
        eventType = "PackLoaded";
    }
};

class InvalidPackAccessAttempted : public DomainEvent {
public:
    std::string attemptedCode;
    std::string reason;

    InvalidPackAccessAttempted(const std::string& code, const std::string& r)
        : attemptedCode(code), reason(r) {
        eventType = "InvalidPackAccessAttempted";
    }
};

class ApiResponseValidationFailed : public DomainEvent {
public:
    std::string apiEndpoint;
    std::string errorDescription;
    int httpStatusCode;

    ApiResponseValidationFailed(const std::string& endpoint,
                               const std::string& error,
                               int code)
        : apiEndpoint(endpoint), errorDescription(error), httpStatusCode(code) {
        eventType = "ApiResponseValidationFailed";
    }
};

class FileAccessAttemptedOutsideRoot : public DomainEvent {
public:
    std::string attemptedPath;
    std::string allowedRoot;

    FileAccessAttemptedOutsideRoot(const std::string& path,
                                   const std::string& root)
        : attemptedPath(path), allowedRoot(root) {
        eventType = "FileAccessAttemptedOutsideRoot";
    }
};

} // namespace domain::events
```

### Event Publisher and Handler

```cpp
// event_publisher.h
#pragma once

#include "domain_events.h"
#include <vector>
#include <functional>
#include <memory>

namespace domain::events {

// Event handler interface
class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void Handle(const std::shared_ptr<DomainEvent>& evt) = 0;
};

// Event publisher (global service)
class EventPublisher {
private:
    std::vector<std::shared_ptr<IEventHandler>> handlers;
    static EventPublisher* instance;

public:
    static EventPublisher& GetInstance();

    void Subscribe(std::shared_ptr<IEventHandler> handler) {
        handlers.push_back(handler);
    }

    void Publish(std::shared_ptr<DomainEvent> evt) {
        for (auto& handler : handlers) {
            try {
                handler->Handle(evt);
            } catch (const std::exception& e) {
                LOG("Event handler error: {}", e.what());
                // Don't let handler exceptions kill publishing
            }
        }
    }

    // Template helper for common use case
    template<typename EventType>
    void PublishEvent(std::shared_ptr<EventType> evt) {
        Publish(std::static_pointer_cast<DomainEvent>(evt));
    }
};

} // namespace domain::events
```

### Security Event Handler

```cpp
// security_event_handler.h
#pragma once

#include "event_publisher.h"
#include "audit_log.h"

namespace infrastructure::monitoring {

class SecurityEventHandler : public domain::events::IEventHandler {
private:
    std::shared_ptr<AuditLog> auditLog;

public:
    explicit SecurityEventHandler(std::shared_ptr<AuditLog> log)
        : auditLog(log) {}

    void Handle(const std::shared_ptr<domain::events::DomainEvent>& evt) override;

private:
    void OnPackAdded(const domain::events::PackAddedEvent& evt);
    void OnPackLoaded(const domain::events::PackLoadedEvent& evt);
    void OnInvalidPackAccess(
        const domain::events::InvalidPackAccessAttempted& evt);
    void OnApiValidationFailed(
        const domain::events::ApiResponseValidationFailed& evt);
    void OnFileAccessOutsideRoot(
        const domain::events::FileAccessAttemptedOutsideRoot& evt);

    void DetectAndLogAnomalies(const domain::events::DomainEvent& evt);
};

} // namespace infrastructure::monitoring
```

#### Implementation

```cpp
// security_event_handler.cpp

namespace infrastructure::monitoring {

void SecurityEventHandler::Handle(
    const std::shared_ptr<domain::events::DomainEvent>& evt) {

    if (auto packAdded = std::dynamic_pointer_cast<
            domain::events::PackAddedEvent>(evt)) {
        OnPackAdded(*packAdded);
    } else if (auto packLoaded = std::dynamic_pointer_cast<
            domain::events::PackLoadedEvent>(evt)) {
        OnPackLoaded(*packLoaded);
    } else if (auto invalidAccess = std::dynamic_pointer_cast<
            domain::events::InvalidPackAccessAttempted>(evt)) {
        OnInvalidPackAccess(*invalidAccess);
    } else if (auto apiFailure = std::dynamic_pointer_cast<
            domain::events::ApiResponseValidationFailed>(evt)) {
        OnApiValidationFailed(*apiFailure);
    } else if (auto fileAccess = std::dynamic_pointer_cast<
            domain::events::FileAccessAttemptedOutsideRoot>(evt)) {
        OnFileAccessOutsideRoot(*fileAccess);
    }

    DetectAndLogAnomalies(*evt);
}

void SecurityEventHandler::OnInvalidPackAccess(
    const domain::events::InvalidPackAccessAttempted& evt) {

    // Critical security event
    LOG(LogLevel::SECURITY, "Invalid pack access attempt: {} ({})",
        evt.attemptedCode, evt.reason);

    auditLog->LogSecurityEvent(
        "InvalidPackAccess",
        {
            {"code", evt.attemptedCode},
            {"reason", evt.reason},
            {"timestamp", evt.GetTimestamp()}
        }
    );

    // Check for suspicious patterns
    // If > 10 attempts in < 1 minute, might be attack
}

void SecurityEventHandler::OnFileAccessOutsideRoot(
    const domain::events::FileAccessAttemptedOutsideRoot& evt) {

    // CRITICAL: Path traversal attempt
    LOG(LogLevel::CRITICAL, "Path traversal attempt: {} outside {}",
        evt.attemptedPath, evt.allowedRoot);

    auditLog->LogSecurityEvent(
        "PathTraversalAttempt",
        {
            {"attemptedPath", evt.attemptedPath},
            {"allowedRoot", evt.allowedRoot},
            {"timestamp", evt.GetTimestamp()}
        }
    );

    // Alert: Potential security attack
}

} // namespace infrastructure::monitoring
```

---

## PHASE 3: REPOSITORIES (Days 7-8)

### Objective
Abstract data persistence behind interfaces to enable testability and flexibility.

### Repository Interface

```cpp
// training_pack_repository.h
#pragma once

#include "domain/training_pack_catalog.h"
#include <filesystem>
#include <memory>

namespace domain::repositories {

// Repository interface
class ITrainingPackRepository {
public:
    virtual ~ITrainingPackRepository() = default;

    virtual Result<void> Save(
        const domain::training::TrainingPackCatalog& catalog) = 0;

    virtual Result<std::shared_ptr<
        domain::training::TrainingPackCatalog>> Load() = 0;

    virtual Result<void> Clear() = 0;
};

} // namespace domain::repositories
```

### File-based Repository Implementation

```cpp
// training_pack_file_repository.cpp
#pragma once

#include "training_pack_repository.h"
#include "nlohmann/json.hpp"
#include <fstream>

namespace infrastructure::persistence {

class TrainingPackFileRepository
    : public domain::repositories::ITrainingPackRepository {

private:
    std::filesystem::path filePath;

    Result<nlohmann::json> LoadAndValidateJson();
    Result<domain::training::TrainingEntry> JsonToEntry(const nlohmann::json& j);
    nlohmann::json EntryToJson(const domain::training::TrainingEntry& entry);

public:
    explicit TrainingPackFileRepository(const std::filesystem::path& path)
        : filePath(path) {}

    Result<void> Save(
        const domain::training::TrainingPackCatalog& catalog) override;

    Result<std::shared_ptr<
        domain::training::TrainingPackCatalog>> Load() override;

    Result<void> Clear() override;
};

} // namespace infrastructure::persistence
```

---

## INTEGRATION CHECKLIST

### Week 1 Completion (Phase 1 & 5)

- [ ] TrainingPackCatalog aggregate compiles and passes unit tests
- [ ] MapRepository aggregate compiles and passes unit tests
- [ ] All value objects enforce invariants
- [ ] No invalid states possible in value object construction
- [ ] Validation happens at aggregate boundaries
- [ ] Code review: Security perspective

### Week 2 Completion (Phase 2 & 4)

- [ ] BakkesMod adapter isolates SDK types
- [ ] Game command whitelist validated
- [ ] External API adapter validates all untrusted input
- [ ] Domain events published for all changes
- [ ] Security event handler logging all relevant events
- [ ] JSON parsing protected against DoS
- [ ] Code review: Security and architecture

### Week 3 Completion (Phase 3)

- [ ] Repository interface defined
- [ ] File repository implements interface
- [ ] Migration from old code to new aggregates complete
- [ ] All existing functionality working with new architecture
- [ ] 80%+ unit test coverage achieved
- [ ] Security tests added (injection, path traversal, etc.)
- [ ] Final code review and sign-off

---

## SECURITY TEST CASES

### Test Suite 1: Input Validation

```cpp
TEST(SecurityTests, PackCodeValidation) {
    // Valid codes
    EXPECT_TRUE(PackCode::Create("1234-ABCD-EF00-9876").IsSuccess());
    EXPECT_TRUE(PackCode::Create("0000-0000-0000-0000").IsSuccess());

    // Invalid codes
    EXPECT_FALSE(PackCode::Create("1234-ABCD-EF00-GHIJ").IsSuccess());  // Non-hex
    EXPECT_FALSE(PackCode::Create("1234-ABCD-EF00").IsSuccess());        // Too short
    EXPECT_FALSE(PackCode::Create("'; DROP TABLE --").IsSuccess());      // SQL injection
}

TEST(SecurityTests, MapFilePathTraversal) {
    auto root = std::filesystem::temp_directory_path() / "workshop";
    std::filesystem::create_directories(root);

    // Valid path
    auto validFile = root / "map.upk";
    std::ofstream(validFile).close();
    EXPECT_TRUE(MapFilePath::Create(validFile, root).IsSuccess());

    // Path traversal attempt
    std::filesystem::path badPath = root / ".." / ".." / "windows" / "system32" / "evil.exe";
    EXPECT_FALSE(MapFilePath::Create(badPath, root).IsSuccess());
}

TEST(SecurityTests, JsonDosProtection) {
    // Deeply nested JSON
    std::string deepJson = R"({"a":{"b":{"c":{"d":{"e":{"f":" ... )";
    auto result = ParseJsonWithLimits(deepJson);
    EXPECT_FALSE(result.IsSuccess());

    // Huge JSON
    std::string hugeJson(2000000, 'x');
    result = ParseJsonWithLimits(hugeJson);
    EXPECT_FALSE(result.IsSuccess());
}
```

---

## SUCCESS CRITERIA

### Security Metrics

- [ ] CVSS score reduced from 8.4 → 2.5 or lower
- [ ] Zero path traversal vulnerabilities
- [ ] Zero command injection vulnerabilities
- [ ] 100% of external input validated
- [ ] All domain invariants enforced

### Code Quality Metrics

- [ ] 80%+ unit test coverage
- [ ] All critical paths tested with fuzzing
- [ ] Zero security warnings in static analysis
- [ ] 100% of pull requests reviewed by security

### Process Metrics

- [ ] Threat model created and reviewed
- [ ] Security checklist in definition of done
- [ ] All team members trained on DDD + security
- [ ] Monthly security reviews scheduled

---

## CONCLUSION

This security-first DDD implementation transforms SuiteSpot from a high-risk codebase to a defense-in-depth, resilient architecture. By combining domain-driven design with security best practices, we eliminate entire classes of vulnerabilities while improving code quality and maintainability.

**Timeline:** 3 weeks
**Team:** 2-3 developers
**Expected Outcome:** Production-ready secure plugin with 80%+ test coverage

