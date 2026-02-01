# SuiteSpot Security Code Review Checklist
## Security-First Development Guidelines

**Version:** 1.0
**Date:** January 31, 2026
**Purpose:** Prevent future vulnerabilities through systematic code review

---

## QUICK REFERENCE

### High-Risk Patterns (Auto-Reject)
- [ ] `system()` with user input
- [ ] String concatenation for commands
- [ ] `strcpy()`, `sprintf()` without bounds
- [ ] Unchecked array access
- [ ] `eval()` or dynamic code execution
- [ ] Unvalidated network input used in system calls
- [ ] File paths without canonical verification
- [ ] `malloc()` without size check

### Medium-Risk Patterns (Requires Review)
- [ ] Mutex without clear unlock path
- [ ] `shared_ptr` cycles (potential deadlocks)
- [ ] Global mutable state without locks
- [ ] Exception in destructor
- [ ] Raw pointers (prefer smart pointers)
- [ ] `TODO` or `FIXME` comments in security code

### Low-Risk Patterns (Good Practice)
- [ ] RAII for resource management
- [ ] Lock guards for mutex
- [ ] Result types for error handling
- [ ] Input validation at boundaries
- [ ] Comprehensive logging

---

## SECURITY CODE REVIEW TEMPLATE

### For Every Pull Request

#### 1. Input Validation

```
REVIEWER CHECKLIST:
[ ] All external input validated?
    [ ] Function parameters checked
    [ ] Network data validated
    [ ] File system input checked
    [ ] User CVars validated
    [ ] JSON fields type-checked

[ ] Validation is comprehensive?
    [ ] Length limits enforced
    [ ] Format validation (regex/pattern)
    [ ] Range checks for numbers
    [ ] No dangerous characters
    [ ] Control characters rejected

[ ] Validation happens at right place?
    [ ] At system boundary (adapter)
    [ ] Before domain logic
    [ ] Not scattered throughout code
```

**Example Patterns to Look For:**

BAD:
```cpp
std::string userPath = request.data;
fs::remove_all(userPath);  // Dangerous!
```

GOOD:
```cpp
Result<std::string> ValidateAndSanitize(const std::string& input) {
    if (!IsValidPath(input)) {
        return Result::Error("Invalid path");
    }
    return Result::Success(SanitizePath(input));
}

auto pathResult = ValidateAndSanitize(userPath);
if (!pathResult) return;
fs::remove_all(pathResult.Value());
```

---

#### 2. Command/Shell Execution

```
REVIEWER CHECKLIST:
[ ] No system() calls with unchecked input?
[ ] No shell metacharacters in command strings?
[ ] Using CreateProcessW instead of system()?
[ ] Command arguments properly quoted/escaped?
[ ] No variable expansion in commands?
[ ] No backtick/$()/`command` patterns?
```

**Example Patterns to Look For:**

BAD:
```cpp
std::string cmd = "powershell.exe " + userData;
system(cmd.c_str());  // VULNERABLE!
```

GOOD:
```cpp
// Option 1: Validate before use
if (!PathValidator::IsValid(userPath)) return false;
STARTUPINFO si = {};
PROCESS_INFORMATION pi = {};
CreateProcessW(L"powershell.exe", &cmdLine, ...);

// Option 2: Use native API
std::filesystem::copy(source, dest);  // No shell!
```

---

#### 3. Path Security

```
REVIEWER CHECKLIST:
[ ] All paths canonicalized?
    std::filesystem::canonical(path, ec);

[ ] Path boundary verified?
    VerifyPathInWorkshop(base, file);

[ ] No .. or ../ in paths?
    Check for path traversal patterns

[ ] Symlinks handled safely?
    [ ] O_NOFOLLOW on open
    [ ] is_symlink() check before dereference
    [ ] No TOCTOU race conditions

[ ] Directory traversal impossible?
    [ ] Relative paths not allowed
    [ ] Absolute paths limited to specific dirs
    [ ] No UNC paths (\\server\share)
```

**Example Pattern:**

BAD:
```cpp
std::string filePath = basePath + "/" + userInput;
std::ifstream file(filePath);  // Traversal possible!
```

GOOD:
```cpp
auto filePath = std::filesystem::path(basePath) / userInput;
std::error_code ec;
auto canonical = std::filesystem::canonical(filePath, ec);

if (ec || canonical.string().find(basePath) != 0) {
    return Result::Error("Invalid path");
}

std::ifstream file(canonical);  // Safe!
```

---

#### 4. Thread Safety

```
REVIEWER CHECKLIST:
[ ] Shared state protected?
    [ ] Mutexes cover all access
    [ ] Lock held for entire operation
    [ ] No deadlock potential (lock ordering)

[ ] Race conditions impossible?
    [ ] No TOCTOU patterns
    [ ] Check-then-use operations atomic
    [ ] Callback race windows closed

[ ] Thread cleanup proper?
    [ ] Threads joined before exit
    [ ] No thread pool explosion
    [ ] Detached threads understood

[ ] Atomic operations correct?
    [ ] Memory ordering appropriate
    [ ] No ABA problems
    [ ] Generation counters work

[ ] No deadlock risks?
    [ ] Lock ordering consistent
    [ ] No nested locks
    [ ] Condition variables used correctly
```

**Example Pattern:**

BAD:
```cpp
if (file.exists(path)) {  // No lock!
    // Another thread could delete file here
    file.copy(path, dest);  // Crash!
}
```

GOOD:
```cpp
{
    std::lock_guard<std::mutex> lock(fileMutex);
    if (file.exists(path)) {  // Lock held
        file.copy(path, dest);
    }
}  // Lock released
```

---

#### 5. Error Handling

```
REVIEWER CHECKLIST:
[ ] All errors checked?
    [ ] system() return codes
    [ ] fopen() null checks
    [ ] CreateProcess() success verified
    [ ] Network errors handled

[ ] Exceptions caught properly?
    [ ] Try-catch for JSON parsing
    [ ] Try-catch for file operations
    [ ] No bare catch() statements

[ ] Error messages safe?
    [ ] No sensitive data in error logs
    [ ] Paths redacted
    [ ] API keys not logged
    [ ] User inputs sanitized in logs

[ ] Resource cleanup on error?
    [ ] File handles closed
    [ ] Memory freed (or RAII used)
    [ ] Locks released
```

---

#### 6. Dependency Security

```
REVIEWER CHECKLIST:
[ ] External libraries validated?
    [ ] No unsigned/untrusted sources
    [ ] SHA256 checksum verified
    [ ] Version pinned and tracked

[ ] Library usage secure?
    [ ] Input size limits enforced
    [ ] Resource limits set
    [ ] Deprecated functions not used
    [ ] Known CVEs not present

[ ] No supply chain risks?
    [ ] vcpkg packages verified
    [ ] No suspicious dependencies
    [ ] License compliance checked
```

---

#### 7. Sensitive Data Handling

```
REVIEWER CHECKLIST:
[ ] No hardcoded secrets?
    [ ] No API keys in code
    [ ] No credentials in tests
    [ ] No tokens in comments

[ ] Sensitive data protected?
    [ ] User paths encrypted in storage
    [ ] Settings file permissions correct
    [ ] Logs don't expose paths
    [ ] Error messages sanitized

[ ] Data destruction?
    [ ] Temporary data cleaned up
    [ ] Sensitive memory overwritten
    [ ] Temp files deleted
```

---

#### 8. JSON/Network Parsing

```
REVIEWER CHECKLIST:
[ ] JSON parsing protected?
    [ ] Size limit enforced (1MB)
    [ ] Depth limit enforced (10 levels)
    [ ] Type checking before access
    [ ] Try-catch around parse()

[ ] Network input validated?
    [ ] HTTPS enforced
    [ ] Certificate verified
    [ ] Content-type validated
    [ ] Timeout enforced

[ ] DoS attacks prevented?
    [ ] Large payloads rejected
    [ ] Deeply nested objects rejected
    [ ] Repeated requests rate-limited
    [ ] Memory allocation bounded
```

**Example Pattern:**

BAD:
```cpp
auto json = nlohmann::json::parse(apiResponse);
auto value = json["field"].get<std::string>();
```

GOOD:
```cpp
// Size check
if (apiResponse.size() > MAX_JSON_SIZE) {
    return Result::Error("JSON too large");
}

// Parse with depth protection
nlohmann::json::parser_callback_t callback = [](int depth, auto, auto&) {
    if (depth > MAX_DEPTH) {
        throw std::runtime_error("JSON too deep");
    }
    return true;
};

auto json = nlohmann::json::parse(apiResponse, callback);

// Safe field access
if (!json.contains("field") || !json["field"].is_string()) {
    return Result::Error("Invalid field");
}
auto value = json["field"].get<std::string>();
```

---

#### 9. Memory Safety

```
REVIEWER CHECKLIST:
[ ] No buffer overflows?
    [ ] std::string used (not char[])
    [ ] Bounds checking on arrays
    [ ] Vector bounds checked
    [ ] No strcat/strcpy family functions

[ ] No use-after-free?
    [ ] Smart pointers (unique_ptr/shared_ptr)
    [ ] Clear ownership semantics
    [ ] Callbacks invalidated on cleanup

[ ] No memory leaks?
    [ ] All allocations have delete
    [ ] RAII for resources
    [ ] Exception-safe code paths
    [ ] Valgrind/AddressSanitizer pass
```

---

#### 10. Logging & Monitoring

```
REVIEWER CHECKLIST:
[ ] Logs don't expose secrets?
    [ ] Paths redacted
    [ ] API keys not logged
    [ ] User data not logged
    [ ] Credentials never logged

[ ] Security events logged?
    [ ] Invalid input attempts
    [ ] Path traversal attempts
    [ ] Authentication failures
    [ ] Configuration changes

[ ] Logs are durable?
    [ ] File handle not lost
    [ ] Rotation configured
    [ ] Permissions correct (user-only read)
    [ ] No log injection
```

---

## SECURITY PROPERTIES TO VERIFY

### For Critical Functions

#### Function: `SafeZipExtractor::ExtractZip()`

```
PROPERTIES:
✓ Input validation
  - zipPath: checked with PathValidator
  - destPath: checked with PathValidator
  - Length limits enforced
  - No dangerous characters

✓ No command injection
  - Uses CreateProcessW, not system()
  - Arguments not concatenated
  - No variable expansion
  - Quotes handled properly

✓ Path traversal prevention
  - Both paths canonicalized
  - Verified within workshop dir
  - Symlinks rejected
  - UNC paths rejected

✓ Error handling
  - CreateProcess failure checked
  - Exit code verified
  - Resource cleanup guaranteed
  - Exceptions caught

✓ Thread safety
  - No mutable global state
  - Mutex protects file operations
  - No TOCTOU windows
```

**Verification Test:**
```cpp
TEST(ZipExtractor, RejectCommandInjection) {
    SafeZipExtractor extractor;
    EXPECT_FALSE(extractor.ValidatePath(
        "file.zip' | cmd /c whoami"));
}

TEST(ZipExtractor, RejectPathTraversal) {
    EXPECT_FALSE(extractor.ValidatePath(
        "../../windows/system32"));
}

TEST(ZipExtractor, AllowValidPaths) {
    EXPECT_TRUE(extractor.ValidatePath(
        "C:\\workshop\\map.zip"));
}
```

---

#### Function: `TrainingPackValidator::ValidatePackCode()`

```
PROPERTIES:
✓ Format validation
  - Regex pattern enforced
  - XXXX-XXXX-XXXX-XXXX format
  - Hexadecimal characters only
  - Exact length (19 chars)

✓ No injection vectors
  - No special characters allowed
  - Control characters rejected
  - Command separators blocked
  - Quotes blocked

✓ Performance
  - O(1) regex evaluation
  - No backtracking risk
  - Constant memory

✓ Test coverage
  - Valid codes pass
  - Invalid formats fail
  - Edge cases handled
  - 30+ test cases
```

**Verification Test:**
```cpp
TEST(PackValidator, ValidCode) {
    EXPECT_TRUE(Validator::ValidatePackCode(
        "ABCD-1234-EF56-7890"));
}

TEST(PackValidator, RejectInvalidFormat) {
    EXPECT_FALSE(Validator::ValidatePackCode("invalid"));
    EXPECT_FALSE(Validator::ValidatePackCode("XXXX-XXXX"));
    EXPECT_FALSE(Validator::ValidatePackCode(
        "ZZZZ-ZZZZ-ZZZZ-ZZZZ"));  // Not hex
}

TEST(PackValidator, RejectInjection) {
    EXPECT_FALSE(Validator::ValidatePackCode(
        "XXXX; DELETE *"));
}
```

---

## REVIEW PROCESS

### Step 1: Automated Checks (PR Submission)
- [ ] Clang-Format passes
- [ ] Static analysis (clang-tidy) passes
- [ ] No compiler warnings
- [ ] All tests passing

### Step 2: Security Review (Pull Request)
- [ ] Assigned to security-trained reviewer
- [ ] High-risk patterns checked
- [ ] Input validation verified
- [ ] Command/shell execution reviewed
- [ ] Path security verified
- [ ] Thread safety analyzed

### Step 3: Code Review (Peer Review)
- [ ] Functional correctness checked
- [ ] Performance implications reviewed
- [ ] Test coverage adequate
- [ ] Documentation complete
- [ ] Comments clear

### Step 4: Final Approval
- [ ] Security reviewer approves
- [ ] Code reviewer approves
- [ ] Product manager approves (if feature change)
- [ ] Merge to develop branch

---

## SECURITY TESTING REQUIREMENTS

### Unit Test Coverage

**For Security Code:**
- [ ] Minimum 80% code coverage
- [ ] 100% of decision branches
- [ ] All error paths exercised
- [ ] Edge cases covered

**Test Categories:**
- [ ] Valid input tests
- [ ] Invalid input tests
- [ ] Boundary tests
- [ ] Injection attack tests
- [ ] Error condition tests
- [ ] Concurrency tests (if applicable)

### Example Test Suite

```cpp
TEST_SUITE(SecurityValidation) {
    // Valid inputs
    TEST(ValidPackCode) { ... }
    TEST(ValidFilePath) { ... }
    TEST(ValidJsonInput) { ... }

    // Invalid inputs
    TEST(RejectEmptyCode) { ... }
    TEST(RejectWrongFormat) { ... }
    TEST(RejectTooLongName) { ... }

    // Injection attacks
    TEST(RejectCommandInjection) { ... }
    TEST(RejectPathTraversal) { ... }
    TEST(RejectJsonExplosion) { ... }

    // Boundary conditions
    TEST(MaxLengthAccepted) { ... }
    TEST(ExceedsMaxLength) { ... }
    TEST(EmptyStringHandled) { ... }
    TEST(NullByteHandled) { ... }

    // Concurrency
    TEST(ThreadSafeValidation) { ... }
    TEST(NoRaceConditions) { ... }
}
```

---

## COMMON VULNERABILITIES TO WATCH FOR

### CWE-78: OS Command Injection
**Pattern:** `system(command_with_user_input)`
**Fix:** Use CreateProcessW or Windows API
**Test:** Try semicolons, pipes, variables

### CWE-22: Path Traversal
**Pattern:** `path = base + user_input`
**Fix:** Use canonical paths + boundary check
**Test:** Try ../, ..\\, absolute paths

### CWE-20: Improper Input Validation
**Pattern:** No validation before use
**Fix:** Validate at boundaries with regex
**Test:** 100+ test cases of invalid input

### CWE-434: Unrestricted Upload
**Pattern:** Accept file without type check
**Fix:** Whitelist allowed file types
**Test:** Try executable uploads

### CWE-327: Use of Broken Cryptography
**Pattern:** SSL verification disabled
**Fix:** Always verify certificates
**Test:** MITM attack scenarios

### CWE-89: SQL Injection
**Status:** NOT APPLICABLE (no SQL in SuiteSpot)

---

## APPROVAL AUTHORITY

### Review Authority

**For Core Domain Code:**
- Primary: Security Architect
- Secondary: Senior Developer
- Tertiary: CTO

**For Infrastructure Code:**
- Primary: Infrastructure Lead
- Secondary: Security Architect
- Tertiary: CTO

**For UI Code:**
- Primary: Lead Developer
- Secondary: QA Engineer
- Tertiary: Product Manager

### Sign-Off Requirements

| Code Type | Security Review | Functional Review | Approval |
|-----------|-----------------|------------------|----------|
| Core Domain | **REQUIRED** | Required | 2 signatures |
| Network/API | **REQUIRED** | Required | Security + Dev |
| File I/O | **REQUIRED** | Required | Security + Dev |
| Thread Code | **REQUIRED** | Required | Security + Dev |
| UI Code | Recommended | Required | Dev + QA |
| Tests | Recommended | Required | QA |

---

## CONTINUOUS IMPROVEMENT

### Metrics to Track

```
Monthly Security Metrics:
- Vulnerabilities found in PRs: [Target: 0]
- High-risk patterns caught: [Target: < 3]
- Security test coverage: [Target: > 80%]
- Code review turn-around: [Target: < 24h]
```

### Regular Reviews

- **Weekly:** Security incident review
- **Monthly:** Code review metrics analysis
- **Quarterly:** Security training update
- **Annually:** Full threat model review

---

## DEVELOPER TRAINING

### Required Reading

1. OWASP Top 10 (15 min)
2. Secure Coding Guidelines (30 min)
3. This Checklist (20 min)
4. CWE-78, CWE-22, CWE-20 (15 min each)

### Hands-On Exercises

- [ ] Vulnerable code analysis (1 hour)
- [ ] Secure code examples (1 hour)
- [ ] Security code review (2 hours)
- [ ] Writing security tests (2 hours)

### Certification

- [ ] Complete training modules
- [ ] Pass security quiz (80%+)
- [ ] Review 5 pull requests with security focus
- [ ] Issue 1 security finding in PR

---

## QUESTIONS & ESCALATION

### If You're Unsure

1. **Ask in code review:** Flag as question for reviewer
2. **Ask security team:** security-questions@suitespot.local
3. **Check documentation:** See SECURITY_AUDIT_2026_UPDATE.md
4. **Escalate:** If blocking development, escalate to CTO

### Common Questions

**Q: Is it safe to use `std::string` for file paths?**
A: Yes, if you validate the path and check it's canonical.

**Q: Do we need to worry about symlinks?**
A: Yes, if the path is user-controlled or external.

**Q: Is JSON parsing inherently insecure?**
A: No, but it needs size/depth limits to prevent DoS.

**Q: How do we handle user input safely?**
A: Whitelist validation + early boundary checking.

---

## CONCLUSION

Security is **everyone's responsibility**. This checklist helps catch issues early, preventing incidents downstream.

**Remember:**
- Validate all input
- Assume the worst
- Test thoroughly
- Review carefully
- Ask when unsure

**Questions?** Contact Security Architect (V3)

---

**Last Updated:** January 31, 2026
**Next Review:** April 30, 2026
**Owner:** Security Architect
