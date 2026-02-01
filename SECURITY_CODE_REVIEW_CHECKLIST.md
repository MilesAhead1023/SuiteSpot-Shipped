# SUITESPOT SECURITY CODE REVIEW CHECKLIST
## Comprehensive Security Review Guidelines for Pull Requests

**Purpose:** Standardized security review process for all SuiteSpot code changes

**Use This For:** All PRs touching security-critical code (file I/O, network, command execution, validation)

**Review Time:** 30-60 minutes per PR (use for high-risk changes)

---

## QUICK TRIAGE (2 minutes)

### Does this PR touch security-critical code?

- [ ] **File I/O** (reading/writing files, directories)
- [ ] **Network operations** (API calls, downloads, TLS/SSL)
- [ ] **Command execution** (game commands, system calls, PowerShell)
- [ ] **Input validation** (pack codes, file paths, JSON parsing)
- [ ] **Authentication/Authorization** (settings, permissions)
- [ ] **Error handling** (exposing sensitive data in errors)
- [ ] **Logging** (PII, secrets in logs)
- [ ] **Data persistence** (settings storage, cache)
- [ ] **Cryptography** (if any)
- [ ] **External dependencies** (new library, version updates)

**If ANY box is checked:** Use FULL checklist below
**If NO boxes checked:** Use QUICK checklist at end

---

## FULL SECURITY REVIEW CHECKLIST

### SECTION 1: INJECTION VULNERABILITIES (10 min)

#### 1.1 Command Injection
- [ ] No `system()`, `exec()`, or `shell_exec()` with user input
- [ ] If command building required:
  - [ ] All user input validated with whitelist
  - [ ] Special characters properly escaped
  - [ ] Use `CreateProcessW()` with array args (not shell string)
  - [ ] No variable concatenation in commands

**Code Pattern to Look For:**
```cpp
// BAD - do not approve
system("command " + userInput);
system(("powershell.exe " + filePath).c_str());

// GOOD - approve
std::vector<std::string> args = {"program", userInput};
// Use secure API with array args
```

#### 1.2 Path Traversal
- [ ] All file paths validated for traversal attempts
- [ ] Paths resolved to canonical form before use
- [ ] Canonical path verified to be within expected root
- [ ] No `..` or `../` sequences remaining
- [ ] Symlinks handled appropriately

**Code Pattern to Look For:**
```cpp
// BAD - no traversal check
fs::copy(userProvidedPath, destination);

// GOOD - traversal prevention
auto canonical = fs::canonical(userProvidedPath);
if (canonical.string().find(rootDir) != 0) reject();
```

#### 1.3 JSON DoS
- [ ] JSON size limited (max 1MB recommended)
- [ ] JSON parsing depth limited (max 10 levels)
- [ ] Parsing catches exceptions
- [ ] Large arrays handled (max 100-1000 elements)

**Code Pattern to Look For:**
```cpp
// BAD - no limits
auto json = nlohmann::json::parse(untrustedString);

// GOOD - with limits
if (input.size() > MAX_SIZE) throw error;
// Use parsing callback to limit depth
```

### SECTION 2: DATA VALIDATION (10 min)

#### 2.1 Input Validation
- [ ] All external input validated before use
- [ ] Validation done at boundary (as early as possible)
- [ ] Whitelist approach used (not blacklist)
- [ ] Format validation (regex or equivalent)
- [ ] Range validation (min/max values)
- [ ] Length validation (string limits)

**Pack Code Validation Example:**
```cpp
// MUST have validation like this
static const std::regex PACK_CODE_PATTERN(
    "^[0-9A-Fa-f]{4}(-[0-9A-Fa-f]{4}){3}$"
);
if (!std::regex_match(code, PACK_CODE_PATTERN)) {
    return error("Invalid pack code");
}
```

#### 2.2 Integer Safety
- [ ] No integer underflow/overflow
- [ ] Signed/unsigned comparisons checked
- [ ] Bounds checking before array access
- [ ] No off-by-one errors in loops

**Code Pattern to Look For:**
```cpp
// BAD - potential overflow
int size = json["array"].size();  // size_t implicitly cast to int
for (int i = 0; i < size; ++i) { ... }

// GOOD - type safe
size_t size = json["array"].size();
for (size_t i = 0; i < size; ++i) { ... }
```

#### 2.3 String Safety
- [ ] `std::string` used (not raw char buffers)
- [ ] No `strcpy()`, `sprintf()` family functions
- [ ] String bounds checked if manual manipulation
- [ ] Control characters stripped from user input

### SECTION 3: FILE OPERATIONS (8 min)

#### 3.1 File I/O Security
- [ ] File paths validated (traversal check)
- [ ] File permissions appropriate (not world-readable for sensitive data)
- [ ] Temporary files cleaned up
- [ ] File handles closed in all code paths
- [ ] Race conditions checked (TOCTOU)

**Code Pattern to Look For:**
```cpp
// BAD - TOCTOU race
if (fs::exists(path)) {
    // another thread could delete between check and use
    fs::copy(path, dest);
}

// GOOD - atomic operation
std::lock_guard lock(fileMutex);
if (fs::exists(path)) {
    fs::copy(path, dest);
}
```

#### 3.2 Directory Operations
- [ ] No symlink following without caution
- [ ] Directory traversal prevented
- [ ] Recursive operations limited
- [ ] Disk space quota enforced (if applicable)

#### 3.3 Temp File Handling
- [ ] Temp files use secure temp directory
- [ ] Temp files have random names
- [ ] Temp files deleted after use
- [ ] No secrets in temp file names/content

### SECTION 4: NETWORK SECURITY (8 min)

#### 4.1 HTTPS/TLS
- [ ] All external APIs use HTTPS (not HTTP)
- [ ] Certificate validation enabled
- [ ] No certificate verification disabled
- [ ] TLS 1.2+ enforced (not older versions)
- [ ] Hostname verification enabled

**Code Pattern to Look For:**
```cpp
// BAD - no validation
curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
// Missing: CURLOPT_SSL_VERIFYPEER, CURLOPT_SSL_VERIFYHOST

// GOOD - validated
curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L);
```

#### 4.2 Request Handling
- [ ] Input to external URLs sanitized
- [ ] URL parsing safe
- [ ] No credentials in URLs
- [ ] Timeout enforced on requests
- [ ] Redirect handling safe (no open redirect)

#### 4.3 Response Handling
- [ ] Response size limited
- [ ] Response headers validated
- [ ] Content-type checked (if applicable)
- [ ] Response encoding handled safely

### SECTION 5: CRYPTOGRAPHY (5 min)

#### 5.1 If Any Crypto Used
- [ ] Using established libraries (not home-grown)
- [ ] Appropriate algorithms (no MD5/SHA1 for security)
- [ ] Proper key management
- [ ] Random number generator seeded properly
- [ ] No plaintext secrets in code

### SECTION 6: ERROR HANDLING (5 min)

#### 6.1 Exception Safety
- [ ] Exceptions caught and handled
- [ ] Error messages don't expose secrets
- [ ] Stack traces don't expose sensitive paths
- [ ] Error recovery proper (resources cleaned up)

**Code Pattern to Look For:**
```cpp
// BAD - exposes path
catch (const std::exception& e) {
    LOG("Error in {}: {}", filePath.string(), e.what());  // Exposes path!
}

// GOOD - redacted
catch (const std::exception& e) {
    LOG("Error loading file: {}", e.what());
    // Or: LOG("Error in [REDACTED_PATH]: {}", e.what());
}
```

#### 6.2 Null/Invalid Pointers
- [ ] All pointers null-checked before use
- [ ] Returned pointers validated
- [ ] Deleted pointers not reused

### SECTION 7: THREAD SAFETY (10 min)

#### 7.1 Locking
- [ ] All shared state protected by mutex/lock
- [ ] Lock scope minimized
- [ ] Lock held minimum time necessary
- [ ] No nested locks (deadlock risk)

#### 7.2 Race Conditions
- [ ] Check-then-act patterns protected
- [ ] Callbacks don't hold locks during I/O
- [ ] Atomic operations used for simple counters
- [ ] Condition variables used appropriately

**Code Pattern to Look For:**
```cpp
// BAD - race condition window
if (!isSearching) {  // Check without lock
    std::lock_guard lock(mutex);
    isSearching = true;  // Could have been set by another thread
}

// GOOD - atomic operation
bool expected = false;
if (isSearching.compare_exchange_strong(expected, true)) {
    // Guaranteed atomic
}
```

#### 7.3 Memory Management
- [ ] No use-after-free
- [ ] No double-delete
- [ ] RAII (Resource Acquisition Is Initialization) used
- [ ] std::lock_guard/std::unique_lock used (not manual unlock)

### SECTION 8: EXTERNAL DEPENDENCIES (5 min)

#### 8.1 Third-Party Libraries
- [ ] Library versions checked for CVEs
- [ ] Known vulnerabilities documented
- [ ] Library properly configured
- [ ] No unnecessary permissions requested

**For nlohmann/json specifically:**
- [ ] Parsing has size/depth limits (new code requirement)
- [ ] Error handling for malformed JSON

**For curl specifically:**
- [ ] HTTPS enforced
- [ ] Certificate validation enabled
- [ ] Timeout set

#### 8.2 BakkesMod SDK
- [ ] No assumptions about SDK security
- [ ] All CVar values validated (SDK could be compromised)
- [ ] Command arguments sanitized
- [ ] Plugin runs with minimal privileges

### SECTION 9: LOGGING (5 min)

#### 9.1 PII Redaction
- [ ] File paths redacted or not logged
- [ ] User names not included
- [ ] API keys not logged
- [ ] Sensitive config not logged

#### 9.2 Log Security
- [ ] Log files stored securely
- [ ] Log files not world-readable
- [ ] Log rotation implemented
- [ ] Old logs deleted appropriately

**Code Pattern to Look For:**
```cpp
// BAD - logs secrets
LOG("Loaded from {}", filepath);  // BAD
LOG("API response: {}", jsonResponse.dump());  // BAD

// GOOD - redacted
LOG("File loaded successfully");
LOG("API request completed");
```

### SECTION 10: SECURE CODING PATTERNS (5 min)

#### 10.1 Value Objects (DDD)
- [ ] Security-critical values are immutable value objects
- [ ] Invariants enforced in constructor
- [ ] No invalid states possible

**Examples:**
- [ ] Pack codes validated in value object
- [ ] File paths canonicalized in value object
- [ ] Difficulty levels from enum (not string)

#### 10.2 Aggregates (DDD)
- [ ] Aggregate enforces consistency
- [ ] All mutations go through single method
- [ ] Boundaries are clear
- [ ] Invariants documented

#### 10.3 Input Validation (DDD)
- [ ] Validation at aggregate boundary
- [ ] Anti-corruption layer for external input
- [ ] Invalid input rejected early

---

## QUICK CHECKLIST (For Non-Security-Critical Changes)

**Use if NO boxes were checked in QUICK TRIAGE**

- [ ] Code compiles without warnings
- [ ] Basic null pointer checks present
- [ ] No obvious logic errors
- [ ] Follows coding style
- [ ] Comments adequate

---

## RED FLAGS (Automatic Rejection)

If you see ANY of these, reject the PR immediately:

- [ ] `system()` with unsanitized user input
- [ ] `strcpy()` or `sprintf()` family functions
- [ ] File paths without traversal validation
- [ ] JSON parsing without size limits
- [ ] Network requests with HTTP (not HTTPS)
- [ ] Secrets (API keys, passwords) in code
- [ ] Disabled security checks (`SSL_VERIFYPEER=0`)
- [ ] No error handling around file/network operations
- [ ] Shared state without locks
- [ ] Log statements exposing PII/paths

**NEVER APPROVE** code with these issues. Request fixes.

---

## APPROVAL CRITERIA

### For Security-Critical Changes
- [ ] All checklist items verified (green)
- [ ] No red flags present
- [ ] Test coverage >= 80% for security-critical code
- [ ] Manual security testing performed
- [ ] Architecture review passed

### For Minor Changes
- [ ] Checklist items applicable (depends on code)
- [ ] No red flags
- [ ] Reasonable test coverage

### Sign-Off
```
Approved by: [Reviewer Name]
Date: [Date]
Risk Level: [Green/Yellow/Red]
Comments: [Any concerns or notes]
```

---

## TESTING REQUIREMENTS BY RISK LEVEL

### Green (Low Risk)
- [ ] Existing unit tests still pass
- [ ] Code compiles without warnings
- [ ] Manual smoke test

### Yellow (Medium Risk)
- [ ] New unit tests for critical code paths
- [ ] Security-specific test cases
- [ ] Integration testing
- [ ] Manual testing with edge cases

### Red (High Risk)
- [ ] Comprehensive unit test coverage (80%+)
- [ ] Security fuzzing (for parsing/validation)
- [ ] Integration tests with full system
- [ ] Penetration testing simulation
- [ ] Code review by 2+ security reviewers

---

## COMMON VULNERABILITIES CHECKLIST

### For Each Vulnerability Type, Verify:

**Command Injection:**
- [ ] No shell commands with string concatenation
- [ ] All arguments validated/escaped
- [ ] Use process execution APIs with array args

**Path Traversal:**
- [ ] Paths canonicalized
- [ ] Canonical path verified to be within root
- [ ] No `..` sequences possible

**XXE (XML External Entity):**
- [ ] Not applicable (no XML parsing in SuiteSpot)

**CSRF (Cross-Site Request Forgery):**
- [ ] Not applicable (not web application)

**SQL Injection:**
- [ ] Not applicable (no SQL database)

**Buffer Overflow:**
- [ ] Using std::string (not char arrays)
- [ ] Array bounds checked

**Integer Overflow:**
- [ ] Signed/unsigned conversions safe
- [ ] Range validation before operations

**Use-After-Free:**
- [ ] Smart pointers (shared_ptr, unique_ptr) used
- [ ] No manual delete/new in hot paths

**Race Condition:**
- [ ] All shared state locked
- [ ] Lock scope minimized
- [ ] TOCTOU patterns checked

---

## REVIEW DECISION TREE

```
START: Review this PR
  |
  +-- Does it touch security code? ---No--> Use QUICK checklist
  |                                          Approve if PASS
  |
  +-- Yes -->  Run FULL checklist
               |
               +-- Any RED FLAGS? ---Yes--> REJECT
               |                            (request fixes)
               |
               +-- No -->  Score for risk level
                           |
                           +-- Green -->  Approve
                           |
                           +-- Yellow --> Request security test cases
                           |               Review design/logic
                           |               Approve if adequate
                           |
                           +-- Red -->   Request 2nd reviewer
                                         May require design review
                                         Request fuzzing/testing
                                         Approve only if excellent
```

---

## EXAMPLE REVIEWS

### Example 1: Simple Logging Change (APPROVE)

```cpp
// Change: Improve debug logging in training pack search
- LOG("Found {} maps", maps.size());
+ for (const auto& map : maps) {
+     LOG("  - {}: {} (creator: {})", map.code, map.name, map.creator);
+ }
```

**Decision:** QUICK CHECKLIST
- [ ] Not security-critical (logging only)
- [ ] No vulnerabilities
- Result: **APPROVE**

---

### Example 2: Path Traversal Risk (REQUEST CHANGES)

```cpp
// Change: Load map from user-provided path
+ auto mapPath = userInput;  // from UI
+ auto canonical = fs::canonical(mapPath);
+ fs::copy(canonical, destination);
```

**Decision:** FULL CHECKLIST
- [ ] File I/O involved - security-critical
- [ ] Path canonicalization present - GOOD
- [ ] But: No verification that canonical path is within root directory
- [ ] **RED FLAG:** Could still be traversal attack

**Action:** REQUEST CHANGES
- Add check: `if (canonical.string().find(workshopRoot) != 0) return error;`

---

### Example 3: Command Injection Risk (REJECT)

```cpp
// Change: Quick load pack via command line
+ std::string cmd = "load_training " + userPackCode;
+ system(cmd.c_str());
```

**Decision:** FULL CHECKLIST
- [ ] Command execution - CRITICAL
- [ ] User input in shell command - **RED FLAG**
- [ ] No validation of userPackCode
- [ ] Using system() - BAD (should be API)

**Action:** REJECT - Do not approve
- Use: `cvarManager->executeCommand("load_training " + validated_code)`
- Validate: `PackCode::Create(userPackCode)` first

---

## RESOURCES

### Documentation References
- **SECURITY_AUDIT_REPORT.md** - Full vulnerability analysis
- **SECURITY_DDD_IMPLEMENTATION.md** - Secure coding patterns
- **DDD_QUICK_REFERENCE.md** - Domain-driven design patterns

### Testing Tools
- Static analysis: Clang Static Analyzer, Cppcheck
- Fuzzing: libFuzzer, AFL
- HTTPS verification: openssl s_client
- Path validation: strace (to see actual paths)

### Learning Resources
- OWASP Top 10 (C++)
- CWE/SANS Top 25 Most Dangerous Software Weaknesses
- "Secure Coding in C and C++" (SEI CERT)

---

## FEEDBACK & IMPROVEMENTS

This checklist is living document. After each review:

1. [ ] Was any item unclear?
2. [ ] Were any important items missed?
3. [ ] Did review take too long?
4. [ ] Would visual examples help?

Submit feedback to: [Security Architect]

---

**Last Updated:** 2026-01-31
**Maintained By:** Security Architect
**Version:** 1.0
**Status:** ACTIVE

Use this checklist for all security-critical code reviews.
