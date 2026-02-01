# SuiteSpot DDD: Quick Reference Guide

**One-page cheat sheet for understanding the domain architecture**

---

## Bounded Contexts at a Glance

```
┌────────────────────────────────────────────────────────────────┐
│                       SUITESPOT DOMAINS                        │
└────────────────────────────────────────────────────────────────┘

TRAINING PACK MGMT          |  MAP & VENUE DISCOVERY
- 2000+ packs library       |  - Freeplay maps
- Search & filter           |  - Workshop maps
- Custom packs              |  - Metadata loading
- Usage tracking            |

AUTOMATION & ORCH.          |  SETTINGS & CONFIG
- Match-end events          |  - CVar management
- Load scheduling           |  - User preferences
- Command execution         |  - Persistence

LOADOUT MANAGEMENT          |  USAGE ANALYTICS
- Car presets               |  - Load statistics
- Loadout switching         |  - Favorites ranking
- Thread-safe execution     |
```

---

## Aggregate Roots (Consistency Boundaries)

| Aggregate | Responsibility | Protected Data | Transaction Scope |
|-----------|-----------------|-----------------|------------------|
| **TrainingPackCatalog** | Pack library | `std::vector<TrainingEntry>` | One pack operation |
| **MapRepository** | Map discovery | Freeplay + Workshop maps | One map operation |
| **UserConfiguration** | Settings | All CVars | One setting change |
| **LoadoutRegistry** | Car presets | Loadout list | One loadout switch |
| **UsageStatistics** | Usage stats | Stats records | One increment |

---

## Value Objects (Type Safety)

```cpp
// Replace these primitives:
std::string code = "XXXX-XXXX-XXXX-XXXX";
std::string difficulty = "Diamond";
int delayQueueSec = 5;

// With these value objects:
PackCode code("XXXX-XXXX-XXXX-XXXX");          // Validated format
Difficulty difficulty("Diamond");                 // Enum-like, type-safe
DelayConfiguration delays(5, 2, 3, 2);           // Complete, immutable

// Benefits:
// ✓ Compile-time type checking
// ✓ Domain validation at creation
// ✓ Self-documenting code
// ✓ Impossible invalid states
```

---

## Repositories (Data Access)

```cpp
// Before (tightly coupled to persistence):
std::vector<TrainingEntry> RLTraining;  // Global, no abstraction
TrainingEntry* GetByCode(const std::string& code);  // Scattered logic

// After (clean abstraction):
class ITrainingPackRepository {
    virtual std::vector<TrainingEntry> GetAll() const = 0;
    virtual TrainingEntry* GetByCode(const std::string& code) = 0;
    virtual void Add(const TrainingEntry& pack) = 0;
    virtual void SaveToDisk(const std::filesystem::path& path) = 0;
};

// Usage in domain:
class TrainingPackCatalog {
    std::unique_ptr<ITrainingPackRepository> repository_;
    // Domain logic uses repository interface, not concrete impl
};

// Benefits:
// ✓ Swap implementations (file, DB, network)
// ✓ Easy to mock in tests
// ✓ Dependency injection friendly
```

---

## Anti-Corruption Layers (SDK Isolation)

```cpp
// Problem: Domain logic depends on BakkesMod SDK
class AutoLoadFeature {
    void OnMatchEnded(std::shared_ptr<GameWrapper> gw, ...) {
        gw->Execute([](GameWrapper* g) {
            g->ExecuteUnrealCommand("load_workshop map.upk");
        });
    }
};

// Solution: Adapter pattern
class BakkesModGameAdapter {  // Shields domain from SDK
    void ExecuteCommandWithDelay(const std::string& cmd, int delayMs) {
        gameWrapper_->SetTimeout([cmd](GameWrapper* gw) {
            gw->ExecuteUnrealCommand(cmd);
        }, delayMs / 1000.0f);
    }
};

// Usage in domain:
class AutomationOrchestrator {
    std::unique_ptr<BakkesModGameAdapter> gameAdapter_;
    void OnMatchEnded(...) {
        gameAdapter_->ExecuteCommandWithDelay("load_workshop map.upk", 2000);
    }
};

// Benefits:
// ✓ Domain doesn't know about BakkesMod
// ✓ Easy to update SDK without domain changes
// ✓ Testable without game environment
```

---

## Domain Events (Loose Coupling)

```cpp
// Problem: Direct coupling between contexts
class TrainingPackCatalog {
    void AddCustomPack(const TrainingEntry& pack) {
        // Now need to update usage tracker here too?
        // And analytics? And UI?
        // Tight coupling!
    }
};

// Solution: Domain events
class TrainingPackCatalog {
    void AddCustomPack(const TrainingEntry& pack) {
        packs_.push_back(pack);
        PublishEvent(CustomPackAddedEvent(pack));
        // Other contexts listen and respond
    }
};

class CustomPackAddedEvent : public DomainEvent { ... };

// Subscribers subscribe independently:
usageTracker->OnPackAdded([](const auto& evt) {
    // Initialize usage for new pack
});
analytics->OnPackAdded([](const auto& evt) {
    // Log analytics event
});
ui->OnPackAdded([](const auto& evt) {
    // Refresh pack list in UI
});

// Benefits:
// ✓ Loose coupling
// ✓ Easy to add new features
// ✓ Clear audit trail
// ✓ Supports event sourcing
```

---

## Hub-and-Spoke Pattern

**SuiteSpot is the Hub:**

```
                    ┌─────────────────┐
                    │   SUITESPOT     │
                    │   (Hub)         │
                    └────────┬────────┘
         ┌──────────────────┼──────────────────┐
         │                  │                  │
         ▼                  ▼                  ▼
    ┌─────────┐         ┌─────────┐      ┌──────────┐
    │Settings │         │Training │      │Map       │
    │Sync     │         │Packs    │      │Manager   │
    │(Spoke)  │         │(Spoke)  │      │(Spoke)   │
    └─────────┘         └─────────┘      └──────────┘
         │                  │                  │
         └──────────────────┼──────────────────┘
                            ▼
                    ┌─────────────────┐
                    │AutoLoadFeature  │
                    │(Service)        │
                    └─────────────────┘
```

**Why this pattern?**
- BakkesMod requires single entry point (SuiteSpot)
- Natural for plugin architecture
- Hub coordinates lifecycle and events
- Each spoke focused on one domain

---

## Key Principles

### 1. Aggregates Enforce Invariants
```cpp
class TrainingPackCatalog {
    // Only way to modify packs
    bool AddCustomPack(const TrainingEntry& pack) {
        if (!ValidatePack(pack)) return false;
        if (PackCodeExists(pack.code)) return false;
        packs_.push_back(pack);
        SaveToDisk();  // Consistency
        return true;
    }
};
```

### 2. Value Objects are Immutable
```cpp
class PackCode {
    const std::string value_;  // Private, immutable
public:
    explicit PackCode(const std::string& code);  // Validation in ctor
    const std::string& ToString() const;  // Read-only access
    // No setters!
};
```

### 3. Repositories Hide Persistence
```cpp
class TrainingPackCatalog {
    std::unique_ptr<ITrainingPackRepository> repository_;
    // Doesn't care if persistence is file, DB, or network
    // Just calls repository interface
};
```

### 4. Services Coordinate Aggregates
```cpp
class AutomationOrchestrator {
    // Coordinates multiple aggregates
    void OnMatchEnded(...) {
        auto pack = packCatalog_->GetByCode(code);
        auto map = mapRepository_->GetByCode(mapCode);
        usageStats_->IncrementLoadCount(code);
        gameAdapter_->ExecuteCommand(loadCmd);
    }
};
```

### 5. Events Decouple Contexts
```cpp
// TrainingPackMgmt doesn't know about UI
packCatalog_->AddCustomPack(pack);  // Just publish event
eventPublisher_->Publish(CustomPackAddedEvent(pack));

// UI listens independently
ui_->OnCustomPackAdded([](const auto& evt) {
    RefreshPackList();
});
```

---

## Naming Conventions

| Component Type | Naming Pattern | Example |
|----------------|-----------------|---------|
| Aggregate Root | `[Domain][Entity]` | `TrainingPackCatalog`, `MapRepository` |
| Entity | `[Domain][Concept]` | `TrainingEntry`, `WorkshopEntry` |
| Value Object | `[Concept]` | `PackCode`, `Difficulty`, `Creator` |
| Service | `[Action][Domain]` | `AutomationOrchestrator` |
| Repository | `I[Domain]Repository` or `[Impl][Domain]Repository` | `ITrainingPackRepository`, `FileSystemTrainingPackRepository` |
| Adapter | `[External][Domain]Adapter` | `BakkesModGameAdapter`, `FileSystemAdapter` |
| Event | `[Action][Domain]Event` | `PackLoadedEvent`, `MatchEndedEvent` |

---

## Testing Strategy

### Unit Tests (Per Aggregate)
```cpp
TEST(TrainingPackCatalogTests, RejectsInvalidPackCode) {
    auto mockRepository = std::make_unique<MockPackRepository>();
    TrainingPackCatalog catalog(std::move(mockRepository));

    TrainingEntry invalidPack{.code = "INVALID"};
    ASSERT_FALSE(catalog.AddCustomPack(invalidPack));
}
```

### Integration Tests (Multiple Aggregates)
```cpp
TEST(AutomationOrchestratorTests, LoadsTrainingPackOnMatchEnd) {
    auto packRepo = std::make_unique<FileSystemTrainingPackRepository>(...);
    auto gameAdapter = std::make_unique<MockGameAdapter>();

    AutomationOrchestrator orchestrator(std::move(packRepo), std::move(gameAdapter));
    orchestrator.OnMatchEnded(...);

    ASSERT_TRUE(gameAdapter->CommandExecuted("load_workshop..."));
}
```

### Event Tests (Event Handling)
```cpp
TEST(EventPublisherTests, NotifiesSubscribersOfPackLoaded) {
    MockSubscriber subscriber;
    EventPublisher publisher;
    publisher.Subscribe<PackLoadedEvent>([&](const auto& evt) {
        subscriber.OnPackLoaded(evt.GetPackCode());
    });

    publisher.Publish(PackLoadedEvent("XXXX-XXXX-XXXX-XXXX"));
    ASSERT_TRUE(subscriber.WasNotified());
}
```

---

## Refactoring Roadmap

| Phase | Timeline | Risk | Key Deliverables |
|-------|----------|------|------------------|
| 1: Aggregates | 3-4 days | Low | Explicit aggregate roots |
| 5: Value Objects | 2-3 days | Low | Type-safe domain concepts |
| 4: Domain Events | 3-4 days | Low-Med | Event system, subscribers |
| 2: ACL | 4-5 days | Medium | SDK/FS adapters, isolation |
| 3: Repositories | 5-7 days | Medium | Repository interfaces, impls |
| **Total** | **~3 weeks** | **Medium** | **Full DDD architecture** |

---

## Anti-Patterns to Avoid

| ❌ Don't Do This | ✅ Do This Instead |
|-----------------|-------------------|
| Expose internals of aggregate | Keep aggregate root as sole entry point |
| Persist partial aggregates | Transaction = entire aggregate |
| Create anemic domain models | Rich domain objects with validation |
| Global state (RLTraining vector) | Injected repositories |
| Circular dependencies | Depend on abstractions, not implementations |
| Events with mutable data | Immutable domain events |
| SDK types in domain logic | Anti-corruption adapter layer |
| Primitives everywhere | Value objects for domain concepts |

---

## Ubiquitous Language Glossary

| Term | Meaning | Context |
|------|---------|---------|
| **Training Pack** | A set of practice shots | Training Pack Mgmt |
| **Pack Code** | Unique pack identifier (XXXX-XXXX-XXXX-XXXX) | Training Pack Mgmt |
| **Difficulty** | Skill level (Bronze-Legend) | Training Pack Mgmt |
| **Freeplay Map** | Standard RL arena | Map Discovery |
| **Workshop Map** | Custom community map | Map Discovery |
| **Auto-Load** | Automatic post-match transition | Automation |
| **Load Sequence** | Steps to execute after match | Automation |
| **Delay** | Wait time before action | Automation |
| **Loadout** | Car preset configuration | Loadout Mgmt |
| **Quick Picks** | Curated pack favorites | Settings |
| **Usage Stats** | Pack load history | Analytics |

---

## File Structure After Refactoring

```
SuiteSpot/
├── Domain/
│   ├── Aggregates/
│   │   ├── TrainingPackCatalog.h/cpp
│   │   ├── MapRepository.h/cpp
│   │   ├── UserConfiguration.h/cpp
│   │   └── LoadoutRegistry.h/cpp
│   ├── Entities/
│   │   └── MapList.h (refactored)
│   ├── ValueObjects/
│   │   ├── PackCode.h/cpp
│   │   ├── Difficulty.h/cpp
│   │   ├── Creator.h/cpp
│   │   └── DelayConfiguration.h/cpp
│   ├── Services/
│   │   └── AutomationOrchestrator.h/cpp
│   └── Events/
│       ├── DomainEvent.h
│       ├── EventPublisher.h/cpp
│       └── [EventName]Event.h
├── Application/
│   └── Services/
│       ├── PackManagementService.h/cpp
│       └── AutomationService.h/cpp
├── Infrastructure/
│   ├── Adapters/
│   │   ├── BakkesModGameAdapter.h/cpp
│   │   ├── BakkesModSettingsAdapter.h/cpp
│   │   └── FileSystemAdapter.h/cpp
│   ├── Repositories/
│   │   ├── ITrainingPackRepository.h
│   │   ├── FileSystemTrainingPackRepository.h/cpp
│   │   ├── IMapRepository.h
│   │   └── FileSystemMapRepository.h/cpp
│   └── Persistence/
│       └── JsonSerializer.h/cpp
├── UI/
│   ├── SettingsUI.h/cpp
│   ├── TrainingPackUI.h/cpp
│   ├── LoadoutUI.h/cpp
│   └── HelpersUI.h/cpp
├── Tests/
│   ├── Unit/
│   │   ├── TrainingPackCatalogTests.cpp
│   │   ├── ValueObjectTests.cpp
│   │   └── ...
│   └── Integration/
│       ├── AutomationOrchestratorTests.cpp
│       └── ...
└── SuiteSpot.h/cpp (simplified hub)
```

---

**For detailed implementation, see `DDD_IMPLEMENTATION_GUIDE.md`**

**For comprehensive analysis, see `DDD_ANALYSIS.md`**

