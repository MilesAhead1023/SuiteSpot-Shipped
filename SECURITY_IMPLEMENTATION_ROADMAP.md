# SuiteSpot Security Implementation Roadmap
## Phase-by-Phase Security Hardening with DDD Integration

**Status:** Ready for Development
**Last Updated:** January 31, 2026
**Owner:** Security Architect + Development Lead

---

## QUICK START: Critical Path (Next 7 Days)

### Day 1: CVE-001 Mitigation (PowerShell RCE)

**Task:** Replace unsafe PowerShell execution with safe path validation

**Files to Create:**
- `Domain/Security/PathValidator.h`
- `Domain/Security/PathValidator.cpp`

**Implementation Checklist:**
- [ ] Create `class PathValidator` with `ValidatePath(path)` method
- [ ] Implement dangerous character detection
- [ ] Implement canonical path resolution
- [ ] Update `WorkshopDownloader::ExtractZipPowerShell()` to use validator
- [ ] Replace `system()` call with `CreateProcessW()`
- [ ] Write unit tests for 10 injection payloads
- [ ] Run suite of tests - **MUST PASS**
- [ ] Code review with second engineer
- [ ] Build and smoke test

**Code Example:**
```cpp
// Domain/Security/PathValidator.h
class PathValidator {
public:
    static bool IsValidPath(const std::string& path);
    static std::string GetCanonicalPath(const std::string& path);
};

// Usage
if (!PathValidator::IsValidPath(zipPath)) {
    LOG("Invalid path rejected");
    return false;
}
// Safe to use zipPath
```

**Time Estimate:** 4 hours
**Owner:** [Assign senior developer]
**Done Criteria:**
- [ ] All injection payloads blocked
- [ ] Code review approved
- [ ] Tests passing
- [ ] No regressions in normal operation

---

### Day 2: CVE-002 Mitigation (Path Traversal)

**Task:** Add path boundary verification to all file operations

**Files to Update:**
- `WorkshopDownloader.cpp` - Update file path handling
- `MapManager.cpp` - Update map discovery
- New: `Domain/Security/FileSystemSecurity.h`

**Implementation Checklist:**
- [ ] Create `VerifyPathInWorkshop()` function
- [ ] Update all `std::filesystem::path` operations
- [ ] Remove old character-stripping approach
- [ ] Write unit tests for traversal attempts
- [ ] Test with real-world payloads (../, ..\\, etc.)
- [ ] Verify canonical path check works on all platforms
- [ ] Code review
- [ ] Build and test

**Critical Function:**
```cpp
bool VerifyPathInWorkshop(const std::filesystem::path& root,
                          const std::filesystem::path& file) {
    auto canonical_root = std::filesystem::canonical(root, ec);
    auto canonical_file = std::filesystem::canonical(file, ec);

    std::string root_str = canonical_root.string();
    std::string file_str = canonical_file.string();

    // Check prefix match with separator
    return file_str.find(root_str) == 0 &&
           file_str[root_str.length()] == std::filesystem::path::preferred_separator;
}
```

**Time Estimate:** 3 hours
**Owner:** [Assign file system expert]
**Done Criteria:**
- [ ] All traversal attempts blocked
- [ ] No valid workshop files rejected
- [ ] Tests comprehensive and passing
- [ ] No performance degradation

---

### Day 3: CVE-003 Mitigation (Input Validation)

**Task:** Add pack code validation everywhere

**Files to Update:**
- `TrainingPackManager.cpp` - Validate on load/save
- `AutoLoadFeature.cpp` - Validate before execution
- New: `Domain/Security/InputValidator.h`

**Implementation Checklist:**
- [ ] Create `class InputValidator` with validation methods
- [ ] Implement regex pattern matching for pack codes
- [ ] Add length checks for all strings
- [ ] Validate control character removal
- [ ] Add validation to `AddPack()`, `UpdatePack()`, `LoadPack()`
- [ ] Write regex-based fuzzing tests
- [ ] Test with 100+ invalid codes
- [ ] Code review
- [ ] Build and test

**Core Validator:**
```cpp
class InputValidator {
public:
    static bool ValidatePackCode(const std::string& code) {
        static const std::regex PACK_PATTERN(
            "^[0-9A-Fa-f]{4}(-[0-9A-Fa-f]{4}){3}$");
        return std::regex_match(code, PACK_PATTERN);
    }

    static bool ValidateName(const std::string& name) {
        if (name.empty() || name.length() > 512) return false;
        for (char c : name) {
            if (std::iscntrl(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }
};
```

**Time Estimate:** 2 hours
**Owner:** [Assign data validation expert]
**Done Criteria:**
- [ ] All invalid codes rejected
- [ ] All valid codes accepted
- [ ] No performance issues
- [ ] Tests comprehensive

---

### Day 4: Security Test Suite & Code Review

**Task:** Build comprehensive security test suite

**Files to Create:**
- `Tests/SecurityTests.cpp` - Main security test suite
- `Tests/InjectionTests.cpp` - Injection attack tests
- `Tests/PathTraversalTests.cpp` - Path traversal tests
- `Tests/InputValidationTests.cpp` - Input validation tests

**Test Checklist:**
- [ ] 20+ injection payload tests
- [ ] 15+ path traversal tests
- [ ] 30+ input validation tests
- [ ] Race condition tests
- [ ] JSON parsing DoS tests
- [ ] All tests passing
- [ ] Coverage reports generated
- [ ] Security review complete

**Sample Test:**
```cpp
TEST_SUITE(SecurityTests) {
    TEST(CommandInjection_PowerShellPipe) {
        ASSERT_FALSE(PathValidator::IsValidPath(
            "test.zip' | Remove-Item -Path 'C:\\*"));
    }

    TEST(PathTraversal_DotDotSlash) {
        ASSERT_FALSE(VerifyPathInWorkshop(
            "C:\\workshop",
            "C:\\workshop\\..\\..\\windows"));
    }

    TEST(InputValidation_InvalidCode) {
        ASSERT_FALSE(InputValidator::ValidatePackCode(
            "invalid-code-format"));
    }
}
```

**Time Estimate:** 6 hours
**Owner:** [Assign QA/test engineer]
**Done Criteria:**
- [ ] 65+ tests written
- [ ] All tests passing
- [ ] Coverage > 80% for security paths
- [ ] Security review approved

---

### Day 5: Final Build & Release Candidate

**Task:** Integration testing and release candidate build

**Verification Checklist:**
- [ ] All security fixes integrated
- [ ] All tests passing (65+ security tests)
- [ ] No regressions in normal features
- [ ] Performance unaffected
- [ ] Memory usage normal
- [ ] Log output clean (no security errors)
- [ ] Documentation updated
- [ ] Release notes prepared

**Release Notes Template:**
```markdown
## Security Update v1.1.0

**CRITICAL:** This release fixes three security vulnerabilities:

### CVE-SUITESPOT-001: Remote Code Execution
- **Severity:** CRITICAL (CVSS 9.1)
- **Impact:** Fixed unsafe PowerShell execution
- **Mitigation:** All paths now validated before system calls

### CVE-SUITESPOT-002: Path Traversal
- **Severity:** HIGH (CVSS 7.5)
- **Impact:** Unauthorized file write capability eliminated
- **Mitigation:** Canonical path verification on all file operations

### CVE-SUITESPOT-003: Input Validation
- **Severity:** MEDIUM (CVSS 6.8)
- **Impact:** Reduced attack surface for malicious pack codes
- **Mitigation:** Strict input validation on all training pack operations

**All Users Should Update Immediately**
```

**Time Estimate:** 4 hours
**Owner:** [Release manager]
**Done Criteria:**
- [ ] Build succeeds
- [ ] All tests pass
- [ ] Integration test successful
- [ ] Release candidate approved

---

## PHASE 1: DDD Foundation (Week 2)

### Goal
Create explicit aggregate roots that enforce security invariants at the domain level.

### Deliverables

#### 1.1 TrainingPackCatalog Aggregate Root
**File:** `Domain/Aggregates/TrainingPackCatalog.h`

```cpp
class TrainingPackCatalog {
private:
    std::vector<TrainingEntry> packs_;
    std::mutex packs_mutex_;

    bool ValidateBeforeAdding(const TrainingEntry& pack) const {
        // Centralized validation
        return InputValidator::ValidatePack(pack);
    }

public:
    Result<void> AddPack(const TrainingEntry& pack) {
        auto validation = ValidateBeforeAdding(pack);
        if (!validation) return Result::Error("Invalid pack");

        {
            std::lock_guard<std::mutex> lock(packs_mutex_);
            packs_.push_back(pack);
        }

        // Publish domain event for audit logging
        PublishEvent(PackAddedEvent{pack});
        return Result::Success();
    }

    // All mutations go through validation
    Result<void> UpdatePack(const TrainingEntry& pack);
    Result<void> RemovePack(const std::string& code);
};
```

**Security Benefits:**
- Single entry point for all pack modifications
- No invalid packs can exist in system
- Easier to audit and test
- Thread-safe with minimal lock scope

**Acceptance Criteria:**
- [ ] Class compiles and links
- [ ] All pack operations use validator
- [ ] Unit tests for valid/invalid cases
- [ ] No pack can bypass validation
- [ ] Performance >= 1000 packs/sec

**Time Estimate:** 6 hours
**Owner:** [Domain expert]

---

#### 1.2 MapRepository Aggregate Root
**File:** `Domain/Aggregates/MapRepository.h`

Similar structure to TrainingPackCatalog but for maps:
- ValidateMapEntry() before adding
- CanonicalPath verification for all file operations
- Consistent locking pattern

**Acceptance Criteria:**
- [ ] All maps validated on entry
- [ ] Path traversal impossible
- [ ] Performance >= 500 maps/sec
- [ ] Unit tests comprehensive

**Time Estimate:** 5 hours

---

#### 1.3 Security Validators as Shared Services
**File:** `Domain/Services/InputValidationService.h`

Centralize all validation logic:
```cpp
class InputValidationService {
public:
    static Result<std::string> ValidatePackCode(const std::string& code);
    static Result<std::string> ValidateName(const std::string& name);
    static Result<std::string> ValidatePath(const std::string& path);
};
```

**Acceptance Criteria:**
- [ ] 100+ test cases
- [ ] All patterns pass/fail correctly
- [ ] Performance < 1ms per validation
- [ ] No allocation in hot path

**Time Estimate:** 4 hours

---

**Week 2 Deliverables:**
- [ ] TrainingPackCatalog aggregate root
- [ ] MapRepository aggregate root
- [ ] Centralized validation services
- [ ] 50+ unit tests
- [ ] Full integration with existing code
- [ ] Build passing, no regressions

**Week 2 Effort:** 20 hours
**Timeline:** Full week (Monday-Friday)

---

## PHASE 2: Anti-Corruption Layers (Week 3)

### Goal
Isolate external untrusted input at system boundaries.

### Deliverables

#### 2.1 BakkesMod Adapter (Command Isolation)
**File:** `Application/Adapters/BakkesModAdapter.h`

```cpp
class BakkesModAdapter {
private:
    static const std::set<std::string> ALLOWED_COMMANDS;

    bool IsCommandAllowed(const std::string& cmd) const {
        return ALLOWED_COMMANDS.count(cmd) > 0;
    }

public:
    Result<void> ExecuteGameCommand(const GameCommand& cmd) {
        if (!IsCommandAllowed(cmd.name)) {
            return Result::Error("Command not whitelisted");
        }

        // Only adapter code interacts with GameWrapper
        cvarManager->executeCommand(cmd.name + " " + cmd.args);
        return Result::Success();
    }
};

const std::set<std::string> BakkesModAdapter::ALLOWED_COMMANDS = {
    "load_training",
    "load_freeplay",
    "load_workshop",
    "queue"
};
```

**Security Benefits:**
- Game commands limited to safe whitelist
- Injection impossible even if args invalid
- Easy to audit and review
- Reduces SDK dependency

**Acceptance Criteria:**
- [ ] All 4 commands work
- [ ] Injection attempts blocked
- [ ] No performance impact
- [ ] Unit tests for all commands

**Time Estimate:** 4 hours

---

#### 2.2 RLMAPS API Adapter (Response Validation)
**File:** `Application/Adapters/RlmapsApiAdapter.h`

```cpp
class RlmapsApiAdapter {
public:
    static Result<std::vector<TrainingEntry>> ParseTrainingPackResponse(
        const nlohmann::json& raw) {

        std::vector<TrainingEntry> packs;

        for (const auto& item : raw) {
            // Validate each field
            auto code = InputValidator::ValidatePackCode(
                item["code"].get<std::string>());
            if (!code) continue;  // Skip invalid entries

            auto name = InputValidator::ValidateName(
                item["name"].get<std::string>());
            if (!name) continue;

            TrainingEntry pack;
            pack.code = code.value();
            pack.name = name.value();
            // ... other fields

            packs.push_back(pack);
        }

        return Result::Success(packs);
    }
};
```

**Security Benefits:**
- API responses validated at boundary
- Invalid entries silently skipped
- No compromised data reaches domain
- Easy to enhance validation later

**Acceptance Criteria:**
- [ ] Valid responses parsed correctly
- [ ] Invalid entries rejected
- [ ] Malicious payloads don't crash
- [ ] Performance > 1000 packs/sec

**Time Estimate:** 4 hours

---

#### 2.3 File System Adapter (Path Safety)
**File:** `Application/Adapters/FileSystemAdapter.h`

```cpp
class FileSystemAdapter {
public:
    static Result<std::vector<MapEntry>> DiscoverMaps(
        const std::filesystem::path& workshopRoot) {

        std::vector<MapEntry> maps;

        for (const auto& entry : std::filesystem::directory_iterator(workshopRoot)) {
            // Verify each path is within workshop
            if (!FileSystemSecurity::VerifyPathInWorkshop(workshopRoot, entry)) {
                LOG("Suspicious path detected, skipping");
                continue;
            }

            MapEntry map;
            map.name = entry.filename().string();
            maps.push_back(map);
        }

        return Result::Success(maps);
    }
};
```

**Security Benefits:**
- All file operations verified before use
- Path traversal impossible
- Symlink attacks prevented
- Consistent across all file access

**Acceptance Criteria:**
- [ ] Real workshop discovery works
- [ ] Traversal attempts blocked
- [ ] Symlinks handled safely
- [ ] Performance acceptable

**Time Estimate:** 3 hours

---

**Week 3 Deliverables:**
- [ ] BakkesMod adapter with command whitelist
- [ ] RLMAPS API adapter with validation
- [ ] File system adapter with path checking
- [ ] 30+ adapter unit tests
- [ ] Integration tests with existing code
- [ ] Full documentation

**Week 3 Effort:** 15 hours
**Timeline:** Full week

---

## PHASE 3: Domain Events (Week 4)

### Goal
Enable security monitoring through comprehensive event logging.

### Deliverables

#### 3.1 Security Domain Events
**File:** `Domain/Events/SecurityEvents.h`

```cpp
// Domain events for all security-relevant operations
struct PackLoadedEvent {
    std::string packCode;
    std::string packName;
    std::chrono::system_clock::time_point timestamp;
    std::string triggeredBy;  // "user" or "auto"
};

struct PathAccessEvent {
    std::filesystem::path path;
    std::string operation;  // "read", "write", "delete"
    std::chrono::system_clock::time_point timestamp;
    bool allowed;
};

struct ValidationFailedEvent {
    std::string validationType;  // "PackCode", "Path", "Json"
    std::string invalidValue;
    std::string reason;
    std::chrono::system_clock::time_point timestamp;
};
```

**Security Benefits:**
- Complete audit trail of sensitive operations
- Anomaly detection possible
- Forensic analysis after incidents
- Regulatory compliance

**Acceptance Criteria:**
- [ ] All domain events defined
- [ ] Events published from aggregates
- [ ] Event schemas documented
- [ ] Serialization working

**Time Estimate:** 4 hours

---

#### 3.2 Event Publishing Infrastructure
**File:** `Domain/Events/DomainEventPublisher.h`

```cpp
class DomainEventPublisher {
private:
    static std::vector<std::shared_ptr<IDomainEventHandler>> handlers_;
    static std::mutex handlers_mutex_;

public:
    static void Subscribe(std::shared_ptr<IDomainEventHandler> handler) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.push_back(handler);
    }

    static void Publish(const DomainEvent& evt) {
        std::vector<std::shared_ptr<IDomainEventHandler>> snapshot;
        {
            std::lock_guard<std::mutex> lock(handlers_mutex_);
            snapshot = handlers_;
        }

        for (const auto& handler : snapshot) {
            handler->Handle(evt);
        }
    }
};
```

**Security Benefits:**
- Decoupled event handling
- Can add/remove monitors dynamically
- No impact on core domain logic
- Testable in isolation

**Acceptance Criteria:**
- [ ] Events publish correctly
- [ ] All handlers invoked
- [ ] No deadlocks
- [ ] Performance > 10K events/sec

**Time Estimate:** 3 hours

---

#### 3.3 Security Event Handler & Audit Logger
**File:** `Infrastructure/Monitoring/SecurityEventHandler.h`

```cpp
class SecurityEventHandler : public IDomainEventHandler {
public:
    void Handle(const DomainEvent& evt) override {
        if (auto pack_evt = dynamic_cast<const PackLoadedEvent*>(&evt)) {
            OnPackLoaded(*pack_evt);
        }
        else if (auto fail_evt = dynamic_cast<const ValidationFailedEvent*>(&evt)) {
            OnValidationFailed(*fail_evt);
        }
    }

private:
    void OnPackLoaded(const PackLoadedEvent& evt) {
        // Detect anomalies
        if (IsAnomalousPattern(evt)) {
            LOG_SECURITY_ALERT("Suspicious pack load: {}", evt.packCode);
        }

        // Write to audit log
        AuditLog::Record("PACK_LOADED", {
            {"code", evt.packCode},
            {"timestamp", evt.timestamp.time_since_epoch().count()}
        });
    }

    void OnValidationFailed(const ValidationFailedEvent& evt) {
        LOG_WARNING("Validation failed: {} - {}", evt.validationType, evt.reason);
        AuditLog::Record("VALIDATION_FAILED", {
            {"type", evt.validationType},
            {"reason", evt.reason}
        });
    }
};
```

**Security Benefits:**
- Real-time anomaly detection
- Permanent audit trail
- Can trigger alerts
- Forensic analysis possible

**Acceptance Criteria:**
- [ ] Handler logs all events
- [ ] Anomalies detected correctly
- [ ] Audit log persistent
- [ ] No performance impact

**Time Estimate:** 4 hours

---

**Week 4 Deliverables:**
- [ ] Domain event definitions
- [ ] Event publishing infrastructure
- [ ] Security event handler
- [ ] Audit logging system
- [ ] 20+ event handler tests
- [ ] Integration with aggregates

**Week 4 Effort:** 15 hours
**Timeline:** Full week

---

## PHASE 4: Repository Pattern (Week 5)

### Goal
Isolate persistence logic from domain.

### Deliverables

#### 4.1 TrainingPackRepository
**File:** `Infrastructure/Repositories/TrainingPackRepository.h`

```cpp
class TrainingPackRepository {
private:
    std::filesystem::path persistencePath_;
    TrainingPackCatalog& catalog_;

public:
    Result<void> LoadFromDisk(const std::filesystem::path& path) {
        // Load, parse, validate
        auto json = ReadJsonFile(path);
        auto packs = RlmapsApiAdapter::ParseTrainingPackResponse(json);

        if (!packs) {
            return Result::Error("Failed to parse packs");
        }

        // Add to catalog (which validates)
        for (const auto& pack : packs.value()) {
            catalog_.AddPack(pack);
        }

        return Result::Success();
    }

    Result<void> SaveToDisk(const std::filesystem::path& path) {
        auto json = SerializePacks(catalog_.GetAll());
        return WriteJsonFile(path, json);
    }
};
```

**Acceptance Criteria:**
- [ ] Load/save working
- [ ] Validation integrated
- [ ] Corruption detection
- [ ] Transactions atomic

**Time Estimate:** 4 hours

---

#### 4.2 MapRepository
**File:** `Infrastructure/Repositories/MapRepository.h`

Scan file system, validate paths, populate aggregate.

**Time Estimate:** 3 hours

---

**Week 5 Deliverables:**
- [ ] TrainingPackRepository
- [ ] MapRepository
- [ ] Integration tests
- [ ] Transaction semantics
- [ ] Comprehensive unit tests

**Week 5 Effort:** 10 hours

---

## PHASE 5: Defense-in-Depth Hardening (Weeks 6-8)

### Goal
Add additional security layers and monitoring.

### Week 6: Resource Limits & Rate Limiting

**Deliverables:**
- [ ] `ResourceLimiter` class - Enforce concurrent download limit (3)
- [ ] `DownloadQueue` - Queue-based request handling
- [ ] `MemoryLimiter` - Cache size enforcement (100MB)
- [ ] `RequestThrottler` - Rate limiting on API calls

**Implementation Example:**
```cpp
class ResourceLimiter {
private:
    std::atomic<int> activeDownloads = 0;
    static constexpr int MAX_CONCURRENT = 3;
    static constexpr size_t MAX_CACHE = 100 * 1024 * 1024;

public:
    Result<void> AcquireDownloadSlot() {
        if (activeDownloads.load() >= MAX_CONCURRENT) {
            return Result::Error("Download queue full");
        }
        activeDownloads++;
        return Result::Success();
    }
};
```

**Effort:** 8 hours

---

### Week 7: File Integrity & Logging Hardening

**Deliverables:**
- [ ] `FileIntegrityChecker` - SHA256 verification
- [ ] `SecureLogger` - Log redaction (path masking)
- [ ] `AuditLogRotation` - Secure log rotation
- [ ] `SensitiveDataRedaction` - Mask PII in logs

**Implementation Example:**
```cpp
class FileIntegrityChecker {
public:
    std::string ComputeSHA256(const std::filesystem::path& file) {
        // Return hex-encoded SHA256 hash
    }

    bool VerifyIntegrity(const std::filesystem::path& file,
                         const std::string& expectedHash) {
        return ComputeSHA256(file) == expectedHash;
    }
};
```

**Effort:** 8 hours

---

### Week 8: Anomaly Detection & Incident Response

**Deliverables:**
- [ ] `AnomalyDetectionEngine` - Pattern-based detection
- [ ] `IncidentResponseHandler` - Automated responses
- [ ] `ThreatDetectionRules` - Rule engine
- [ ] `SecurityDashboard` - Monitoring UI

**Effort:** 12 hours

---

## QUALITY GATES & ACCEPTANCE CRITERIA

### Critical Path (Week 1) Gate
All three CVE fixes must:
- [ ] Pass all security tests (65+ test cases)
- [ ] Code review approved by security expert
- [ ] No regressions in functional tests
- [ ] Performance acceptable (< 5% overhead)
- [ ] Zero known vulnerabilities remaining

**Gating Decision:** Can ship emergency patch?

---

### Phase 1 Gate (Week 2)
Aggregates must:
- [ ] Enforce all security invariants
- [ ] Pass comprehensive unit tests (50+ tests)
- [ ] Maintain backward compatibility
- [ ] Performance >= baseline

**Gating Decision:** Can proceed to Phase 2?

---

### Phase 2 Gate (Week 3)
Adapters must:
- [ ] Validate all external input
- [ ] Block all injection attempts
- [ ] Maintain full functionality
- [ ] Pass integration tests

**Gating Decision:** Can proceed to Phase 3?

---

### Phase 3-5 Gates
Each phase must have:
- [ ] 80%+ code coverage for security code
- [ ] Comprehensive security testing
- [ ] Performance benchmarks
- [ ] Security review approval

---

## RESOURCE ALLOCATION

### Team Requirements

**Week 1 (Emergency Fixes):**
- 1x Senior Backend Developer (full week)
- 1x QA/Security Tester (3 days)
- 1x Reviewer/Code Quality (2 days)

**Weeks 2-5 (DDD Refactoring):**
- 2x Domain-Driven Design Expert (full 4 weeks)
- 1x Security Architect (2 days/week review)
- 1x QA Engineer (full 4 weeks)

**Weeks 6-8 (Defense-in-Depth):**
- 1x Senior Backend Developer (full 3 weeks)
- 1x Security Engineer (2 days/week)
- 1x Monitoring/DevOps (1 day/week)

**Total Investment:** ~4 weeks senior engineer time
**Estimated Cost:** $25K-30K

---

## SUCCESS METRICS

### Security Metrics
- CVSS score: 8.4 → 3.2 (Phase 1) → 1.5 (Phase 5)
- Vulnerabilities: 3 → 0 (Week 1) → 0 with hardening (Week 8)
- Code coverage: 60% → 80% (Week 5)
- Security test cases: 65 → 200+ (Week 8)

### Operational Metrics
- Build time: No change
- Startup time: < 5% increase
- Memory usage: < 10% increase
- API latency: No regression
- Logging volume: Controlled via rotation

### Organizational Metrics
- Security incidents: 0 (maintained)
- Patch deployment time: < 1 day
- Code review cycle: < 24 hours
- Security debt: Eliminated

---

## TIMELINE SUMMARY

```
Week 1: EMERGENCY FIXES
├─ Mon: CVE-001 fix + tests
├─ Tue: CVE-002 fix + tests
├─ Wed: CVE-003 fix + tests
├─ Thu: Security test suite
└─ Fri: Release candidate

Week 2: PHASE 1 (DDD Aggregates)
├─ Mon-Tue: TrainingPackCatalog
├─ Wed: MapRepository
├─ Thu: Validators
└─ Fri: Integration testing

Week 3: PHASE 2 (Anti-Corruption)
├─ Mon-Tue: BakkesMod adapter
├─ Wed: RLMAPS API adapter
├─ Thu: FileSystem adapter
└─ Fri: Integration testing

Week 4: PHASE 3 (Domain Events)
├─ Mon: Event definitions
├─ Tue: Event publisher
├─ Wed-Thu: Event handlers
└─ Fri: Integration testing

Week 5: PHASE 4 (Repositories)
├─ Mon-Tue: TrainingPackRepository
├─ Wed: MapRepository
├─ Thu-Fri: Integration testing

Weeks 6-8: PHASE 5 (Defense-in-Depth)
├─ Week 6: Resource limits + rate limiting
├─ Week 7: File integrity + logging
└─ Week 8: Anomaly detection + response

TOTAL: 8 weeks to full security posture
```

---

## DEPLOYMENT CHECKLIST

Before shipping each phase:

**Pre-Deployment:**
- [ ] All automated tests passing
- [ ] Security review completed
- [ ] Code coverage >= 80%
- [ ] Performance benchmarks run
- [ ] Release notes prepared
- [ ] Stakeholder approval obtained

**Deployment:**
- [ ] Staged rollout (5% → 25% → 100%)
- [ ] Monitoring enabled
- [ ] Incident response team alerted
- [ ] Rollback plan ready

**Post-Deployment:**
- [ ] Monitor for issues (24 hours)
- [ ] Collect performance metrics
- [ ] Update documentation
- [ ] Conduct post-mortem if issues

---

## CONCLUSION

This roadmap provides a clear path from emergency fixes to production-ready security. By following this plan, SuiteSpot will achieve:

✓ Zero critical vulnerabilities
✓ Secure-by-design architecture
✓ Comprehensive audit logging
✓ Real-time anomaly detection
✓ Sustainable security posture

**Start Date:** [Today]
**Go-Live Date:** 8 weeks
**Owner:** [Development Lead]

**Questions? Contact:** Security Architect (V3)
