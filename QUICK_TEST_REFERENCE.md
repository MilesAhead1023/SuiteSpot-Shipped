# SuiteSpot Testing - Quick Reference Card

## Critical Test Cases (MUST PASS)

### WorkshopDownloader HTTP Fixes

```
TC-WD-001: HTTP Callback Completion Without Polling
├─ Verify: No busy-wait loops in GetResults callback
├─ Check: RLMAPS_Searching properly released
├─ Assert: completedRequests == 1
└─ Expected: <100ms callback completion

TC-WD-002: Concurrent Search Isolation via Generation
├─ Verify: searchGeneration increments on each search
├─ Check: Old callbacks ignored (line 46, 112, 148)
├─ Assert: Results contain only latest generation
└─ Expected: No interference between searches

TC-WD-003: Stale Callback Invalidation
├─ Verify: Generation mismatch triggers early return
├─ Check: notifyCompletion() called even when stale
├─ Assert: No deadlock in condition variable
└─ Expected: Graceful handling of rapid searches

TC-WD-004: Atomic Progress Updates
├─ Verify: std::atomic<int> used for all progress vars
├─ Check: Updates visible on reader thread within 50ms
├─ Assert: Monotonically increasing progress
└─ Expected: No torn reads or stale values

TC-WD-005: Download Flag Lifecycle
├─ Verify: RLMAPS_IsDownloadingWorkshop = false in:
│  ├─ Line 344: Success path
│  ├─ Line 347: File write failure
│  ├─ Line 335: Extraction timeout
│  └─ Line 351: HTTP failure
├─ Check: No exception path leaves flag true
├─ Assert: All code paths covered
└─ Expected: 100% coverage of exit paths
```

### Threading Safety (MUST PASS)

```
TC-THR-001: Concurrent Searches Don't Crash
├─ Run: 5 simultaneous searches
├─ Monitor: No segfaults or undefined behavior
├─ Assert: Final state valid and consistent
└─ Tool: Address Sanitizer (no crashes)

TC-THR-002: Atomic Visibility Across Threads
├─ Test: Writer thread modifies, reader observes
├─ Check: Progress values monotonic
├─ Assert: Visible within 2 polling intervals
└─ Expected: No visibility issues

TC-THR-003: Mutex Protects MapResultList
├─ Load: 4 concurrent threads (add/clear/read/update)
├─ Check: No data races detected
├─ Assert: List integrity maintained
└─ Tool: ThreadSanitizer (0 races)

TC-THR-004: Condition Variable Signaling
├─ Test: Wait completes when completedRequests >= expected
├─ Check: Also exits on generation change
├─ Assert: No spurious wakeups
└─ Expected: <500ms total wait time

TC-THR-005: Detached Thread Cleanup
├─ Count: Threads before and after search
├─ Check: Thread count returns to baseline
├─ Assert: No zombie threads
└─ Expected: Clean thread lifecycle
```

---

## Test Execution Commands

### Build Tests

```powershell
# CMake approach
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# Direct compilation
msbuild tests\SuiteSpot.Tests.vcxproj /p:Configuration=Release /p:Platform=x64
```

### Run Tests

```bash
# All tests
.\build\Release\SuiteSpotUnitTests

# Specific suite
.\build\Release\SuiteSpotUnitTests --gtest_filter=WorkshopDownloaderTest.*

# Single test
.\build\Release\SuiteSpotUnitTests --gtest_filter=WorkshopDownloaderTest.SearchCompletesWithoutPolling

# With ThreadSanitizer
clang++ -fsanitize=thread tests/threading/*.obj -o test_thread

# With AddressSanitizer
clang++ -fsanitize=address tests/unit/*.obj -o test_asan
```

### Visual Studio Test Explorer

```
Ctrl+E, T       # Open Test Explorer
Ctrl+R, A       # Run all tests
Ctrl+R, D       # Debug selected tests
Right-click → Run Selected Tests
```

---

## Critical Code Paths to Verify

### WorkshopDownloader.cpp

```cpp
// Line 14-99: GetResults() - Main search entry point
14:    GetResults(string keyword, int page)
18:    RLMAPS_Searching.compare_exchange_strong()  // Must use atomic CAS
24:    searchGeneration++                           // Increment for invalidation
44:    HttpWrapper::SendCurlRequest(lambda {        // HTTP callback
46:        if (searchGeneration != currentGeneration) return;  // Gen check!
52-57:  HTTP error handling (lines must set flag)
74-78:  Empty result handling
88-91:  Wait on condition variable (must exit on gen change)
93:     RLMAPS_Searching = false;                   // Release on all paths!

// Line 102-245: GetMapResult() - Map detail fetching
105:    notifyCompletion lambda                     // Must be called always
146:    release callback lambda
147:    if (searchGeneration != generation) return; // Gen check!
156-157: HTTP failure (must call notifyCompletion)
209-234: Add to result list (must call notifyCompletion on all paths)
241:     catch exception (must call notifyCompletion)

// Line 271-354: RLMAPS_DownloadWorkshop() - Download with progress
306-307: Initialize progress to 0
319:     Download callback
320:     if (code == 200) - Success path
335:     Extraction timeout (must set RLMAPS_IsDownloadingWorkshop = false)
344:     Success - set flag to false
346-348: File error - set flag to false
351:     HTTP error - set flag to false
```

### AutoLoadFeature.cpp

```cpp
// Line 34-43: safeExecute lambda
34:     Calculate actualDelay (must enforce 0.1s minimum)
39:     SetTimeout with correct delay
40:     executeCommand with command string

// Line 46-60: Freeplay mode
47:     Find map in list (verify exists)
54:     Execute: "load_freeplay " + code

// Line 61-121: Training mode
67:     Get QuickPicksSelectedCode
74:     Trust BakkesMod (FIX: no validation against cache)
110:    Usage tracker increment
116:    Execute: "load_training " + code

// Line 122-138: Workshop mode
129:    Find workshop map (verify exists)
131:    Execute: "load_workshop \"path\"" (quoted!)

// Line 140-143: Auto-queue
141:    Execute: "queue" command
```

---

## Performance Targets

| Metric | Target | Max | Passes If |
|--------|--------|-----|-----------|
| Search Time (20 maps) | <3s | <5s | ✓ if < 5s |
| Download Throughput | Unlimited | N/A | ✓ no UI block |
| Memory per Search | <5MB | <10MB | ✓ if < 10MB |
| Logging Overhead | <1% | <5% | ✓ if < 5% |
| Frame Rate Drop | 0ms | <2ms | ✓ if < 2ms |
| Progress Update Lag | <50ms | <100ms | ✓ if < 100ms |

---

## Quick Failure Diagnostics

### "Search hangs indefinitely"
```
Check:
1. resultsMutex locked forever?
2. completedRequests never incremented?
3. Condition variable notify_one() called?
4. Wait condition logic correct?

Fix: Add timeout to wait_for() and inspect logs
```

### "Data race detected by ThreadSanitizer"
```
Check:
1. Is RLMAPS_MapResultList access protected by mutex?
2. Are all atomic accesses using std::atomic<T>?
3. Is there a lock_guard in every access path?

Fix: Add missing lock_guard, verify sequential consistency
```

### "Memory leak in search"
```
Check:
1. Is RLMAPS_MapResultList cleared properly?
2. Do lambda captures retain references?
3. Are threads properly joined?

Fix: Use smart pointers, clear container explicitly
```

### "Progress not visible"
```
Check:
1. Is RLMAPS_Download_Progress using std::atomic?
2. Is reader using .load() to read atomic?
3. Is there a memory barrier?

Fix: Use std::atomic with default sequential consistency
```

### "Callback never called"
```
Check:
1. Is mock configured with EXPECT_CALL()?
2. Does callback signature match expected?
3. Is callback lambda still in scope?

Fix: Verify mock setup matches actual usage
```

---

## File Locations

```
C:\Users\bmile\Source\SuiteSpot\
├── TEST_PLAN.md                    [Detailed test specifications]
├── TEST_EXECUTION_GUIDE.md         [How to run tests]
├── TESTING_SUMMARY.md              [Executive summary]
├── QUICK_TEST_REFERENCE.md         [This file]
└── tests/
    ├── unit/
    │   └── WorkshopDownloaderTests.cpp
    ├── integration/
    │   └── AutoLoadFeatureTests.cpp
    ├── threading/
    │   └── ConcurrencyTests.cpp
    └── mocks/
        └── MockGameWrapper.h
```

---

## Test Execution Phases

```
Phase 1: Unit Tests (30 min)
  → TC-WD-001 through TC-WD-005 ✓
  → Edge case tests ✓
  PASS CRITERIA: All tests green

Phase 2: Threading Tests (1 hour)
  → TC-THR-001 through TC-THR-005 ✓
  → ThreadSanitizer: 0 races ✓
  PASS CRITERIA: 0 data races

Phase 3: Integration Tests (1 hour)
  → TC-WF-001 through TC-WF-006 ✓
  → Manual game testing ✓
  PASS CRITERIA: All workflows work

Phase 4: Edge Cases (45 min)
  → TC-EDGE-001 through TC-EDGE-006 ✓
  PASS CRITERIA: Graceful error handling

Phase 5: Performance (2 hours)
  → Baseline measurements ✓
  → Regression detection ✓
  PASS CRITERIA: Within 10% of baseline
```

**Total Time: 5.75 hours automated**

---

## Sign-Off Checklist

### Must Have (CRITICAL)

- [ ] TC-WD-001 passing (callback completion)
- [ ] TC-WD-002 passing (generation tracking)
- [ ] TC-WD-003 passing (stale callback handling)
- [ ] TC-WD-005 passing (flag lifecycle)
- [ ] TC-THR-002 passing (atomic visibility)
- [ ] TC-THR-003 passing (mutex protection)
- [ ] TC-THR-004 passing (condition variable)
- [ ] ThreadSanitizer: 0 data races
- [ ] Manual game testing: All workflows
- [ ] Coverage: >85% overall, 100% critical paths

### Should Have (HIGH)

- [ ] TC-WD-004 passing (progress tracking)
- [ ] TC-THR-001 passing (concurrent safety)
- [ ] TC-THR-005 passing (thread cleanup)
- [ ] TC-WF-001 passing (match-end automation)
- [ ] All edge case tests passing
- [ ] Performance within baseline ±10%

### Nice to Have (MEDIUM)

- [ ] Full integration tests
- [ ] Performance improvement analysis
- [ ] Stress testing (100+ maps)
- [ ] Documentation completed

---

## Emergency Contacts

**For stuck tests:**
1. Check TEST_EXECUTION_GUIDE.md debugging section
2. Review mocks configuration
3. Enable verbose logging
4. Use Visual Studio debugger (F5)

**For data races:**
1. Run ThreadSanitizer
2. Check mutex lock_guard placement
3. Verify atomic variable usage
4. Inspect wait condition logic

**For performance issues:**
1. Profile with Release build
2. Check for tight loops in callbacks
3. Verify logging isn't excessive
4. Measure against baseline

---

## Key Insights from Code Review

### Generation Tracking Innovation
- Simple but effective: `searchGeneration++` on each search
- Callbacks check: `if (gen != current) return;` early
- Prevents stale data from old searches

### Atomic Variable Strategy
- std::atomic provides sequential consistency by default
- No need for explicit locks for simple reads/writes
- Progress updates visible within <50ms

### Condition Variable Pattern
- Wait for: `completedRequests >= expectedRequests || generation changed`
- Notify: `completedRequests++` then `notify_one()`
- Clean early exit on new search

### Detached Thread Safety
- Detach after lambda capture is safe
- Lambda captures by value (no dangling refs)
- Main thread waits on condition variable

---

**Last Updated:** January 31, 2026
**Version:** 1.0
**Status:** Ready for Use
