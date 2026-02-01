# SuiteSpot Plugin - Comprehensive Test Plan

**Version:** 1.0
**Date:** January 31, 2026
**Focus:** WorkshopDownloader fix, critical workflows, threading, edge cases, and performance regression testing

---

## Executive Summary

This test plan ensures the reliability and robustness of SuiteSpot's core systems following the WorkshopDownloader HTTP callback fix. The plan covers:

1. **WorkshopDownloader HTTP callback completion** without polling
2. **Concurrent search request isolation** via generation tracking
3. **Critical match-end automation workflows**
4. **Thread safety and race condition prevention**
5. **Edge cases and error scenarios**
6. **Performance regression testing**

---

## Part 1: WorkshopDownloader HTTP Callback Tests

### TC-WD-001: HTTP Callback Completion Without Polling

**Objective:** Verify that HTTP callbacks complete without blocking or polling mechanisms.

**Setup:**
- Create mock HTTP wrapper that simulates network latency (100-500ms)
- Initialize WorkshopDownloader with mocked GameWrapper
- Prepare test keyword: "test_map"

**Test Steps:**
1. Call `GetResults("test_map", 1)`
2. Verify `RLMAPS_Searching` transitions from false → true → false
3. Monitor HTTP callback invocation with timestamp tracking
4. Verify no polling loops in HTTP callback path
5. Check `completedRequests` atomic increments exactly once per HTTP response

**Acceptance Criteria:**
- [ ] Callback completes within 100ms of response receipt
- [ ] No busy-wait loops detected (grep for `while (!condition)` in callback path)
- [ ] `completedRequests` atomic matches `expectedRequests` count
- [ ] `RLMAPS_Searching` flag properly released on all code paths (success/failure/error)
- [ ] No memory leaks from callback lambda captures

**Code Inspection Points:**
- Line 44-99 (GetResults callback)
- Line 146-240 (GetMapResult release callback)
- Verify `notifyCompletion()` lambda is always called
- Check no exception propagation from callback

---

### TC-WD-002: Concurrent Search Request Isolation

**Objective:** Verify concurrent search requests don't interfere with each other via generation tracking.

**Setup:**
- Initialize WorkshopDownloader with mocked HTTP
- Prepare two sequential search keywords: "keyword1", "keyword2"
- Mock HTTP responses with variable delays (100ms, 200ms, 500ms)

**Test Steps:**
1. Start search for "keyword1"
   - Capture `searchGeneration` value (gen1)
   - Trigger 5 HTTP requests that will complete out of order
2. Before gen1 completes, start search for "keyword2"
   - Increment generation to gen2
   - Trigger different HTTP requests
3. Verify callbacks:
   - Callbacks with generation < gen2 are ignored
   - Callbacks with generation == gen2 are processed
   - Result list contains only gen2 data

**Acceptance Criteria:**
- [ ] `searchGeneration` atomic increments on each `GetResults()` call
- [ ] Old callbacks check generation before processing (line 46, 112, 148)
- [ ] Stale callbacks still call `notifyCompletion()` for proper wait release
- [ ] `RLMAPS_MapResultList` contains only the latest generation results
- [ ] No race condition in generation check (atomic load + early return)

**Expected Behavior:**
```
Timeline:
  T=0ms: Search1 started (gen=1)
  T=50ms: Search1 HTTP1 response arrives (gen check: 1==1, process)
  T=100ms: Search2 started (gen=2), Search1 results cleared
  T=150ms: Search1 HTTP2 response arrives (gen check: 1!=2, ignore)
  T=200ms: Search2 HTTP1 response arrives (gen check: 2==2, process)
Result: Only Search2 data in final list
```

---

### TC-WD-003: Search Generation Invalidation

**Objective:** Verify generation invalidation properly prevents stale callback processing.

**Setup:**
- Prepare HTTP mock that responds very slowly (5000ms)
- Initialize WorkshopDownloader

**Test Steps:**
1. Trigger `GetResults()` capturing generation=1
2. Wait 100ms, trigger new `GetResults()` capturing generation=2
3. Monitor first search's callbacks:
   - When initial search HTTP response arrives after 5 seconds
   - Verify generation mismatch check (line 46 in GetResults callback)
4. Verify `RLMAPS_Searching` flag released properly for both searches
5. Check no deadlock in condition variable wait

**Acceptance Criteria:**
- [ ] Stale callbacks exit early without processing
- [ ] `RLMAPS_Searching` flag released for each search attempt
- [ ] Condition variable not stuck waiting on stale generation
- [ ] Memory for stale lambda captures properly released
- [ ] No orphaned HTTP requests in memory

**Edge Case:** Rapid generation changes (3+ consecutive searches in 50ms)

---

### TC-WD-004: Progress Updates Track Correctly Via Atomics

**Objective:** Verify progress atomics update correctly without data races.

**Setup:**
- Mock HTTP callback with progress function simulation
- Enable progress tracking for map result downloads
- Use tools to detect atomic visibility (std::atomic sequential consistency)

**Test Steps:**
1. Start map download via `RLMAPS_DownloadWorkshop()`
2. Simulate progress function callbacks:
   ```cpp
   progress_function(1000000, 250000);  // 25% complete
   progress_function(1000000, 500000);  // 50% complete
   progress_function(1000000, 1000000); // 100% complete
   ```
3. On main thread, poll atomics:
   - `RLMAPS_Download_Progress`
   - `RLMAPS_WorkshopDownload_FileSize`
4. Verify values are monotonically increasing
5. Verify final values match input parameters

**Acceptance Criteria:**
- [ ] All atomics use `std::atomic<int>` with sequential consistency (default)
- [ ] Progress updates visible immediately on other threads
- [ ] No stale reads or visibility issues
- [ ] No overflow on large file sizes (>2GB requires 64-bit atomic)
- [ ] Progress function captures don't leak memory

**Code Review:**
- Line 314-317: Progress function in `RLMAPS_DownloadWorkshop`
- Verify proper casting (double → int)
- Check for potential precision loss

---

### TC-WD-005: Download Completion Flag Set in All Code Paths

**Objective:** Verify `RLMAPS_IsDownloadingWorkshop` is set to false in all success/failure/error code paths.

**Setup:**
- Prepare test cases for various HTTP completion scenarios
- Initialize WorkshopDownloader

**Test Cases:**

**TC-WD-005a: Success Path**
```
Trigger: RLMAPS_DownloadWorkshop()
HTTP Response: 200 OK with valid data
Expected: RLMAPS_IsDownloadingWorkshop → false (line 344)
```

**TC-WD-005b: HTTP Failure**
```
Trigger: RLMAPS_DownloadWorkshop()
HTTP Response: 404 Not Found
Expected: RLMAPS_IsDownloadingWorkshop → false (line 351)
```

**TC-WD-005c: File Write Failure**
```
Trigger: RLMAPS_DownloadWorkshop()
HTTP Response: 200 OK
File I/O: std::ofstream fails (permission denied)
Expected: RLMAPS_IsDownloadingWorkshop → false (line 347)
```

**TC-WD-005d: Extraction Timeout**
```
Trigger: RLMAPS_DownloadWorkshop()
HTTP Response: 200 OK
Extraction: Timeout after 10 retries
Expected: RLMAPS_IsDownloadingWorkshop → false (line 335)
```

**Acceptance Criteria:**
- [ ] All code paths (lines 320-352) have `RLMAPS_IsDownloadingWorkshop = false`
- [ ] Exception handlers also set flag to false
- [ ] No dangling true state that could block subsequent downloads
- [ ] Flag setter is not in finally-like construct (ensure all paths hit)

**Verification Method:**
- Grep: `RLMAPS_IsDownloadingWorkshop = false` count should match all exit paths
- Code review: Trace each execution path in callback lambda (line 319-353)

---

## Part 2: Critical Workflow Tests

### TC-WF-001: Match-End Automation Complete Flow

**Objective:** Verify AutoLoadFeature properly handles match end with all settings combinations.

**Setup:**
- Mock GameWrapper with timeout tracking
- Mock CVarManagerWrapper to capture executed commands
- Initialize SettingsSync with known values
- Prepare training, freeplay, and workshop pack lists

**Test Steps:**

**Scenario A: Freeplay Mode**
1. Set map type to 0 (Freeplay)
2. Set current freeplay code to "DFHStadium"
3. Set delay to 2 seconds
4. Call `OnMatchEnded(...)`
5. Verify:
   - Timeout scheduled for ~2s
   - Command executed: `load_freeplay DFHStadium`
   - Log entry created

**Scenario B: Training Mode with Valid Pack**
1. Set map type to 1 (Training)
2. Set training code to "12345ABC"
3. Set delay to 1 second
4. Call `OnMatchEnded(...)`
5. Verify:
   - Timeout scheduled for ~1s
   - Command executed: `load_training 12345ABC`
   - Usage tracker incremented

**Scenario C: Training Mode Fallback to Quick Pick**
1. Set map type to 1 (Training)
2. Set training code to empty string
3. No Quick Pick selected, but DefaultPacks available
4. Call `OnMatchEnded(...)`
5. Verify:
   - Falls back to first DefaultPack
   - Timeout scheduled
   - Log shows fallback message

**Scenario D: Workshop Mode**
1. Set map type to 2 (Workshop)
2. Set workshop path to valid file
3. Set delay to 3 seconds
4. Call `OnMatchEnded(...)`
5. Verify:
   - Timeout scheduled for ~3s
   - Command executed: `load_workshop "path/to/map.upk"`
   - Proper escaping for quoted path

**Scenario E: Auto-Queue**
1. Enable auto-queue with queue delay 1s
2. Select freeplay map with load delay 2s
3. Call `OnMatchEnded(...)`
4. Verify:
   - Both timeouts scheduled
   - Queue scheduled after map load (1s < 2s timeline)
   - Both commands in correct order

**Acceptance Criteria:**
- [ ] Correct timeout delay for each map type
- [ ] Correct command format and arguments
- [ ] CVarManager.executeCommand called exactly once per operation
- [ ] Usage tracker updated for training packs
- [ ] Fallback chain works: code → quick picks → defaults
- [ ] Log entries created for each operation
- [ ] No crashes with null pointers

---

### TC-WF-002: Training Pack Selection and Loading

**Objective:** Verify training pack selection workflow from UI to load execution.

**Setup:**
- Load 2000+ training packs into TrainingPackManager
- Mock GameWrapper and CVarManager
- Prepare SettingsSync

**Test Steps:**
1. Search for "Air Dribble" in UI
2. Filter by difficulty "Diamond"
3. Select first result
4. Verify in SettingsSync: `GetCurrentTrainingCode()` returns correct code
5. Trigger match end
6. Verify `load_training <code>` executed
7. Verify PackUsageTracker incremented load count

**Acceptance Criteria:**
- [ ] Search returns filtered results
- [ ] Selection persists across plugin state
- [ ] AutoLoadFeature uses selected code
- [ ] Usage statistics updated accurately
- [ ] No duplicate loads in concurrent scenarios

---

### TC-WF-003: Workshop Map Downloading and Extraction

**Objective:** Verify end-to-end workshop map download, extraction, and availability.

**Setup:**
- Mock HTTP for search and download
- Prepare real or mock zip file structure
- Initialize MapManager and WorkshopDownloader

**Test Steps:**
1. Search workshop for "custom map"
2. Receive mock search results (3 maps)
3. Select first map, click "Download"
4. Verify:
   - Progress updates visible
   - JSON metadata created
   - ZIP downloaded to correct location
   - Extracted to `.upk` format
5. Refresh workshop list
6. Verify map appears in available workshop maps
7. Select and trigger match end
8. Verify `load_workshop` command executed

**Acceptance Criteria:**
- [ ] Search completes without blocking
- [ ] Download shows progress
- [ ] ZIP extraction successful
- [ ] JSON metadata correct
- [ ] UPK file properly renamed
- [ ] MapManager discovers new map
- [ ] Map loadable via commands

---

### TC-WF-004: Settings Persistence and CVar Synchronization

**Objective:** Verify settings save/load across plugin restarts.

**Setup:**
- Initialize SettingsSync with defaults
- Simulate various setting changes
- Prepare file I/O mocks

**Test Steps:**
1. Change settings:
   - Enable auto-queue
   - Set delay to 5 seconds
   - Select training pack
   - Select freeplay map
2. Trigger settings save
3. Verify JSON file created with all values
4. Simulate plugin restart:
   - Create new SettingsSync instance
   - Load from file
5. Verify all settings restored correctly
6. Change one setting, save
7. Verify only changed setting updated in file

**Acceptance Criteria:**
- [ ] All settings persist across restart
- [ ] CVar names follow `suitespot_` prefix convention
- [ ] JSON structure valid
- [ ] No corrupted state after partial write
- [ ] Settings visible in CVarManager

---

### TC-WF-005: Loadout Switching and Application

**Objective:** Verify car loadout switching works correctly on match end.

**Setup:**
- Mock LoadoutManager with car presets
- Initialize SettingsSync with loadout option
- Prepare 3+ different loadout configurations

**Test Steps:**
1. Save custom loadout "Training Setup"
2. Enable "Switch Loadout on Match End"
3. Set AutoLoadFeature to use this loadout
4. Trigger match end simulation
5. Verify LoadoutManager applies correct loadout
6. Verify colors/decals applied correctly

**Acceptance Criteria:**
- [ ] Loadout applied after specified delay
- [ ] Correct loadout index used
- [ ] No race condition with map loading
- [ ] Works with all training types (freeplay, training pack, workshop)

---

### TC-WF-006: Usage Tracking for Favorites

**Objective:** Verify PackUsageTracker correctly maintains statistics.

**Setup:**
- Initialize PackUsageTracker
- Load 10 training packs
- Simulate multiple match ends

**Test Steps:**
1. Load pack A (5 times)
2. Load pack B (3 times)
3. Load pack C (1 time)
4. Trigger save
5. Verify JSON contains correct counts
6. Verify Quick Picks list: [A, B, C, ...]
7. Simulate plugin restart
8. Verify counts restored
9. Load pack A (1 more time)
10. Verify count now 6 for pack A

**Acceptance Criteria:**
- [ ] Load counts accurate
- [ ] Top packs calculated correctly
- [ ] Persistence across restarts
- [ ] Atomic updates visible to UI thread
- [ ] No overflow on large counts

---

## Part 3: Threading and Concurrency Tests

### TC-THR-001: Concurrent Pack Searches Don't Crash

**Objective:** Verify concurrent search requests don't cause crashes or undefined behavior.

**Setup:**
- Initialize WorkshopDownloader
- Mock HTTP with realistic latencies (50-500ms per request)
- Prepare thread safety tools (ThreadSanitizer, DebugView)

**Test Steps:**
1. Start search for "keyword1" on main thread
2. After 50ms, start search for "keyword2" on UI thread
3. After 100ms, start search for "keyword3" on separate thread
4. Monitor for:
   - Segmentation faults
   - Double-free errors
   - Memory corruption
   - Deadlocks

**Acceptance Criteria:**
- [ ] No crashes under concurrent searches
- [ ] Each search completes independently
- [ ] Results contain only latest generation data
- [ ] No memory leaks detected by sanitizer
- [ ] No deadlocks reported

**Stress Test:**
- Start 10 concurrent searches rapidly
- Verify graceful degradation (last one wins)
- Monitor peak memory usage (<50MB for search data)

---

### TC-THR-002: Atomic Variable Updates Visible Across Threads

**Objective:** Verify std::atomic variables update visibility without synchronization bugs.

**Setup:**
- Create test thread that modifies atomics
- Create reader thread that polls atomics
- Use thread timing tools

**Test Steps:**
1. Reader thread polls `RLMAPS_Download_Progress` at 10ms intervals
2. Modifier thread updates via progress callback at 50ms intervals
3. Verify reader sees updates within 1-2 polling intervals
4. Repeat for all atomic variables:
   - `RLMAPS_Searching`
   - `RLMAPS_NumberOfMapsFound`
   - `RLMAPS_IsDownloadingWorkshop`
   - `RLMAPS_Download_Progress`
   - `completedRequests`
   - `searchGeneration`

**Acceptance Criteria:**
- [ ] All atomics use std::atomic (implicit sequential consistency)
- [ ] Reader observes updates within 50ms
- [ ] No torn reads (partial updates)
- [ ] No compiler optimizations reorder atomic operations
- [ ] Release-acquire semantics work correctly

**Verification:**
```cpp
// Pseudo-test for progress visibility:
std::thread modifier([&]() {
    for(int i = 0; i <= 100; ++i) {
        RLMAPS_Download_Progress = i;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
});

std::vector<int> observed;
for(int j = 0; j < 15; ++j) {
    observed.push_back(RLMAPS_Download_Progress.load());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

// Should see monotonic increase: 0 < val2 <= val3 < val4...
ASSERT_TRUE(std::is_sorted(observed.begin(), observed.end()));
```

---

### TC-THR-003: Mutex Locks Prevent Race Conditions in MapResultList

**Objective:** Verify std::mutex properly protects `RLMAPS_MapResultList` from concurrent access.

**Setup:**
- Multiple threads adding/removing from RLMAPS_MapResultList
- Race detection tools (ThreadSanitizer)
- Prepare heavy concurrent load (10+ threads)

**Test Steps:**
1. Thread 1: Continuously adds results to list
2. Thread 2: Continuously clears list (from GetResults)
3. Thread 3: Continuously reads list (search results dialog)
4. Thread 4: Continuously updates elements (preview download complete)
5. Run for 5 seconds with high lock contention
6. Verify no race conditions detected
7. Verify final list integrity (no corrupted entries)

**Acceptance Criteria:**
- [ ] std::lock_guard used in all access paths
- [ ] No data race reported by ThreadSanitizer
- [ ] List integrity maintained
- [ ] No deadlocks from lock ordering
- [ ] Performance acceptable (no mutex starvation)

**Code Review Points:**
- Line 27-29: `GetResults` clear operation
- Line 219-221: `GetMapResult` add operation
- Line 376-382: `DownloadPreviewImage` update operation

---

### TC-THR-004: Condition Variable Signaling Works Reliably

**Objective:** Verify condition variable properly signals completion of all HTTP requests.

**Setup:**
- Mock HTTP requests with variable completion times
- Track condition variable signals and wakeups
- Prepare failure injection (delayed signals)

**Test Steps:**
1. Start search with 5 map results (5 HTTP requests expected)
2. Set `completedRequests = 0`
3. Main thread calls `resultsCV.wait()`
4. Background threads:
   - Complete requests at staggered times (100ms, 150ms, 200ms, 250ms, 300ms)
   - Call `completedRequests++` and `resultsCV.notify_one()`
5. Verify wait completes when `completedRequests >= expectedRequests`
6. Repeat with generation change (early exit condition)

**Acceptance Criteria:**
- [ ] Wait exits exactly when `completedRequests >= expectedRequests`
- [ ] Wait exits on generation change (line 90 condition)
- [ ] No spurious wakeups cause early exit
- [ ] Lost signals don't hang the wait
- [ ] Multiple notify_one() calls don't deadlock

**Edge Case: No Requests**
```
Input: Search returns 0 results
expectedRequests = 0
completedRequests = 0
Expected: Wait condition immediately true, no hang
```

---

### TC-THR-005: Detached Threads Complete Properly

**Objective:** Verify detached threads for HTTP requests and page count don't leave zombies.

**Setup:**
- Monitor thread count and completion
- Prepare resource leak detection

**Test Steps:**
1. Count initial thread count: `N`
2. Trigger `GetResults()` with 5 maps:
   - 1 detached thread for GetNumPages
   - 5 detached threads for GetMapResult
   - Each GetMapResult spawns HTTP callback thread
3. Wait for search completion (wait condition unblocks)
4. Verify thread count returns to `N`
5. Verify no orphaned threads in debugger
6. Check stack usage doesn't grow unbounded

**Acceptance Criteria:**
- [ ] All detached threads complete before GetResults returns
- [ ] Thread count returns to baseline
- [ ] No memory from thread stacks leaked
- [ ] No zombie threads in system
- [ ] Large search (100+ maps) doesn't exhaust thread pool

**Stress Test:**
- Trigger 50 consecutive searches with 20 maps each
- Monitor total thread count doesn't exceed 2x baseline
- Verify no thread pool exhaustion

---

## Part 4: Edge Cases and Error Conditions

### TC-EDGE-001: Empty Search Results Handling

**Objective:** Verify graceful handling of zero search results.

**Test Scenario:**
```json
// HTTP Response (200 OK)
[]  // Empty array
```

**Test Steps:**
1. Search for obscure keyword that returns 0 results
2. Verify handling at line 74-78
3. Check:
   - `RLMAPS_NumberOfMapsFound = 0`
   - `RLMAPS_Searching = false`
   - No threads spawned
   - Condition variable properly signaled
   - UI displays "No results" gracefully

**Acceptance Criteria:**
- [ ] No crash on empty array
- [ ] Flag properly released
- [ ] No orphaned threads
- [ ] UI responsive (no hang)
- [ ] Clean state for next search

---

### TC-EDGE-002: Network Timeout/Failure Scenarios

**Objective:** Verify handling of network errors at each HTTP stage.

**Test Cases:**

**TC-EDGE-002a: Search Request Timeout**
- HTTP Response: 0 (timeout/connection failed)
- Expected: Log error, set `RLMAPS_Searching = false`, release condition variable

**TC-EDGE-002b: Release Request Failure**
- Search succeeds (HTTP 200)
- Map details succeed (HTTP 200)
- Release lookup fails (HTTP 404/500)
- Expected: Skip this map, count as completed, continue with others

**TC-EDGE-002c: Partial Response**
- Search returns 5 maps
- Only 3 release requests succeed
- 2 fail or timeout
- Expected: 3 maps in results, 5 completedRequests, wait exits normally

**Acceptance Criteria:**
- [ ] Errors logged with HTTP code and context
- [ ] Graceful degradation (partial results acceptable)
- [ ] No hanging on failed requests
- [ ] Search flagreleased even on error
- [ ] Retry/retry policy not needed for automated loads

---

### TC-EDGE-003: Missing Files or Corrupt Downloads

**Objective:** Verify handling of corrupted or missing files.

**Test Cases:**

**TC-EDGE-003a: ZIP Corruption**
- Download succeeds (HTTP 200)
- ZIP file truncated or corrupted
- Extraction fails
- Expected: Log error, set `RLMAPS_IsDownloadingWorkshop = false`

**TC-EDGE-003b: Extraction Timeout**
- Download succeeds
- Extraction hangs >10 seconds
- Loop exits at line 333-336
- Expected: Flag set to false, error logged

**TC-EDGE-003c: Missing UPK After Extraction**
- ZIP extracted
- No `.udk` file found
- UdkInDirectory returns "Null"
- Expected: Error logged, flag released, user notified

**Acceptance Criteria:**
- [ ] Timeout logic prevents indefinite hang (line 331-340)
- [ ] All error paths set download flag to false
- [ ] No orphaned partial files
- [ ] User error message clear and actionable

---

### TC-EDGE-004: Invalid Pack Codes

**Objective:** Verify handling of non-existent or invalid training pack codes.

**Test Scenario:**
1. User manually enters invalid code: "INVALID123"
2. Settings persist this code
3. Trigger match end
4. AutoLoadFeature should:
   - Trust BakkesMod to handle invalid code (FIX at line 74-75)
   - Still execute command (let game decide)
   - Log the attempt
5. Verify command sent: `load_training INVALID123`
6. Game ignores or shows error (not plugin's responsibility)

**Acceptance Criteria:**
- [ ] No validation in AutoLoadFeature against pack cache
- [ ] Command sent even if code not in training list
- [ ] Fresh/shared codes work
- [ ] Fallback logic only if code empty
- [ ] Log shows what code attempted

---

### TC-EDGE-005: Zero-Delay Settings Edge Cases

**Objective:** Verify zero-delay settings handled correctly.

**Test Scenario:**
User sets delay to 0 seconds.

**Expected Behavior (Line 34-37):**
```cpp
float actualDelay = (delaySec <= 0) ? 0.1f : static_cast<float>(delaySec);
```

**Test Steps:**
1. Set all delays to 0
2. Trigger match end
3. Verify actual delays are 0.1s (minimum)
4. Verify commands execute in correct order

**Acceptance Criteria:**
- [ ] Minimum delay enforced (no 0 delay)
- [ ] Game state properly settled before map load
- [ ] No race conditions with immediate execution
- [ ] Timeout precision adequate (0.1s is ~6 frames at 60fps)

---

### TC-EDGE-006: Concurrent Download Attempts

**Objective:** Verify only one download can proceed at a time.

**Test Steps:**
1. Start workshop download A
2. Before completion, attempt to start download B
3. Verify:
   - `RLMAPS_IsDownloadingWorkshop` already true
   - Second download blocked or queued
   - No file overwrites
   - Progress tracking for only one download

**Acceptance Criteria:**
- [ ] No concurrent file I/O corruption
- [ ] Queue/blocking behavior documented
- [ ] User feedback on blocked download attempt

---

## Part 5: Performance Regression Tests

### TC-PERF-001: Workshop Download Completes Without Blocking

**Objective:** Verify download doesn't block game rendering thread.

**Setup:**
- Monitor main thread frame time
- Start large workshop download (>100MB mock)
- Track game responsiveness

**Test Steps:**
1. Start download simulation with 500MB file
2. Trigger progress callbacks simulating:
   - 5 updates per second
   - Realistic bandwidth (5MB/s)
3. Measure main thread frame time
4. Expected: <16ms frame time (60 FPS acceptable)
5. Verify progress updates visible without stutter

**Acceptance Criteria:**
- [ ] HTTP callback on background thread (detached)
- [ ] UI updates via atomic reads (non-blocking)
- [ ] No joins/waits on download thread
- [ ] Frame time <20ms during download
- [ ] Progress bar smooth (10+ updates/sec)

**Code Review:**
- Line 319-353: Callback lambda should be lightweight
- No heavy I/O on main thread
- File writing on callback thread (async)

---

### TC-PERF-002: Search Doesn't Cause Frame Rate Drops

**Objective:** Verify search operation remains responsive.

**Setup:**
- Monitor frame time during search
- Prepare heavy search load (20 maps, 100+ HTTP requests)

**Test Steps:**
1. Start performance monitoring
2. Trigger search for common term
3. Background threads spawn for:
   - GetNumPages (1 thread)
   - GetMapResult (N threads for N maps)
   - HTTP release requests (N threads)
4. Measure main thread frame time every 16ms
5. Expected: No frames exceed 20ms
6. Expected: Average <16ms

**Acceptance Criteria:**
- [ ] Search on background threads only
- [ ] UI callbacks don't block frame
- [ ] Mutex contention minimal
- [ ] Result updates visible smoothly
- [ ] No frame stutters during search

**Stress Test:**
- Concurrent search + map loading
- Concurrent search + workshop download
- Multiple searches back-to-back
- Verify no frame drop >5ms above baseline

---

### TC-PERF-003: Memory Usage Stays Within Bounds

**Objective:** Verify memory doesn't leak or grow unbounded.

**Setup:**
- Enable memory profiling
- Prepare heap snapshot tools
- Set baseline memory: `B0`

**Test Steps:**
1. Take heap snapshot: `B0`
2. Execute 10 searches (20+ maps each)
3. Complete all searches
4. Clear results
5. Take heap snapshot: `B1`
6. Verify: `B1 - B0 < 5MB`
7. Repeat 5 times
8. Verify: No gradual growth across iterations

**Acceptance Criteria:**
- [ ] Per-search memory freed after completion
- [ ] No accumulating leaks
- [ ] Large result lists freed properly
- [ ] Lambda captures don't retain references
- [ ] Total search data memory <10MB for 1000 maps

**Profiling Points:**
- RLMAPS_MapResultList capacity/size
- std::string allocations in result objects
- Thread-local storage cleanup

---

### TC-PERF-004: No Excessive Logging in Tight Loops

**Objective:** Verify logging doesn't cause performance issues.

**Setup:**
- Prepare high-frequency logging during progress
- Monitor logging I/O overhead
- Set up logger with file and console output

**Test Steps:**
1. Simulate download with progress callback every 10ms
2. Progress callback logs progress: `LOG("Progress: {}%", percent)`
3. Monitor:
   - Logging overhead per call
   - Total logging time for 10-second download (1000 logs)
   - File I/O wait time
4. Expected: Logging <1% of total time

**Acceptance Criteria:**
- [ ] No logging in HTTP progress callbacks (or minimal)
- [ ] Strategic logging at milestones only
- [ ] Async logging buffer used
- [ ] No synchronous file writes blocking threads
- [ ] Logging overhead <10% of operation time

**Code Review:**
- Line 314-317: Progress function should not log per update
- Line 326, 332, 343: Log at key milestones only

---

### TC-PERF-005: Search Generation Tracking Overhead

**Objective:** Verify generation tracking adds minimal overhead.

**Setup:**
- Benchmark GetResults performance
- Compare with/without generation checks

**Test Steps:**
1. Baseline: Execute 100 searches without modification
2. Measure: Average search completion time: `T_baseline`
3. Run instrumented version
4. Measure: Average search completion time: `T_instrumented`
5. Calculate overhead: `(T_instrumented - T_baseline) / T_baseline`
6. Expected: <5% overhead

**Acceptance Criteria:**
- [ ] Generation checks are atomic loads (non-blocking)
- [ ] Early returns prevent unnecessary processing
- [ ] No additional HTTP requests
- [ ] Overhead <5ms per search

---

## Part 6: Integration Test Scenarios

### TC-INT-001: Full Match-to-Training Cycle

**Objective:** End-to-end test of real game match scenario.

**Scenario:**
1. Player in competitive match
2. Match ends
3. SuiteSpot triggers with training pack selected
4. Verify:
   - Map loads automatically
   - Correct training pack active
   - Usage stats updated
   - Settings persisted
   - No crashes or hangs

**Acceptance Criteria:**
- [ ] Match end hook fires correctly
- [ ] AutoLoadFeature called with proper parameters
- [ ] Command executed and game responds
- [ ] State consistent after action
- [ ] Logging clear for debugging

---

### TC-INT-002: Workshop Map Full Pipeline

**Objective:** Complete workflow from search to play.

**Scenario:**
1. Open Workshop Browser
2. Search for custom maps
3. Select and download map
4. Wait for extraction
5. Select map in settings
6. End match
7. Verify map loads

**Acceptance Criteria:**
- [ ] Search completes and displays results
- [ ] Download shows progress and completes
- [ ] Extraction successful
- [ ] Map appears in available list
- [ ] Map can be selected and loads on match end

---

### TC-INT-003: Settings Persistence After Update

**Objective:** Verify no settings loss during plugin updates.

**Scenario:**
1. Plugin v1.0 running with settings
2. User upgrades to v1.1 (migration code)
3. Verify all old settings migrated
4. Verify new feature settings have defaults

**Acceptance Criteria:**
- [ ] No default-reset on update
- [ ] Migration logic handles old JSON format
- [ ] User's selections preserved
- [ ] New fields populated with sensible defaults

---

## Part 7: Test Implementation Strategy

### Unit Test Framework

**Technology:** Google Test (gtest) or Catch2 recommended

**Structure:**
```
tests/
├── unit/
│   ├── WorkshopDownloaderTests.cpp
│   ├── AutoLoadFeatureTests.cpp
│   ├── TrainingPackManagerTests.cpp
│   ├── MapManagerTests.cpp
│   └── SettingsSyncTests.cpp
├── integration/
│   ├── MatchEndWorkflowTests.cpp
│   ├── WorkshopPipelineTests.cpp
│   └── SettingsPersistenceTests.cpp
├── threading/
│   ├── ConcurrencyTests.cpp
│   └── AtomicVisibilityTests.cpp
├── performance/
│   ├── PerformanceRegressionTests.cpp
│   └── BenchmarkTests.cpp
└── mocks/
    ├── MockGameWrapper.h
    ├── MockHttpWrapper.h
    └── MockCVarManager.h
```

### Mock Objects

**MockGameWrapper:**
- Capture SetTimeout calls
- Track command execution
- Simulate game state

**MockHttpWrapper:**
- Simulate HTTP responses with latency
- Support callback injection
- Track request counts

**MockCVarManager:**
- Capture executed commands
- Support return value configuration

### Test Execution

**Local Execution:**
```bash
# Build tests
msbuild SuiteSpot.sln /p:Configuration=Debug /p:Platform=x64

# Run all tests
ctest --output-on-failure

# Run specific suite
ctest -R "WorkshopDownloader" --verbose

# Run with thread sanitizer
ctest --memcheck --memcheck-type=ThreadSanitizer
```

**CI/CD Integration:**
- GitHub Actions: Run tests on every commit
- Nightly: Extended stress tests
- Release: Performance regression baseline

---

## Part 8: Test Metrics and Success Criteria

### Coverage Requirements

| Metric | Target | Current |
|--------|--------|---------|
| Line Coverage | >85% | - |
| Branch Coverage | >80% | - |
| Function Coverage | >90% | - |
| Critical Path Coverage | 100% | - |

### Critical Paths (100% Required)
1. WorkshopDownloader.GetResults() callback completion
2. AutoLoadFeature.OnMatchEnded() all branches
3. Generation tracking in callbacks
4. Mutex protection in MapResultList
5. Atomic variable visibility
6. Download completion flag in all paths
7. Thread detach and completion

### Performance Targets

| Metric | Target | Threshold |
|--------|--------|-----------|
| Search completion time | <3s | <5s max |
| Download throughput | Not limited by UI | 0 frame drops |
| Memory per search | <5MB | <10MB |
| Logging overhead | <1% | <5% |
| Generation check overhead | <5ms | <10ms |

### Regression Prevention

- Baseline performance measurements recorded
- Automated regression detection
- Historical tracking dashboard
- Alert on >10% degradation

---

## Part 9: Known Limitations and Assumptions

### Assumptions

1. **BakkesMod API Stable:** Game commands (`load_training`, `load_workshop`) work as documented
2. **File System Reliable:** ZIP extraction, file writes succeed under normal conditions
3. **Network Connectivity:** Tests assume available network for HTTP requests
4. **Thread Count:** System can support 20+ concurrent threads
5. **Memory:** Minimum 100MB free for large searches (1000+ maps)

### Limitations

1. **Game Integration:** Cannot fully simulate Rocket League game state in unit tests
2. **UI Threading:** ImGui integration tested via integration tests only
3. **File System:** Mocked in unit tests, real in integration tests
4. **Network Mocking:** Simulated HTTP responses, not real RLMAPS API

### Mitigations

- Integration tests run against real game
- E2E tests executed manually before release
- Performance tests run on standard hardware
- Thread safety verified with ThreadSanitizer

---

## Part 10: Test Execution Checklist

### Pre-Test Setup
- [ ] Build SuiteSpot.sln Release x64
- [ ] Install gtest/Catch2 framework
- [ ] Prepare test data and fixtures
- [ ] Enable thread sanitizer and memory leak detection
- [ ] Reserve isolated test environment (no other plugins)

### Execution Phases

**Phase 1: Unit Tests (Day 1)**
- [ ] TC-WD-001 through TC-WD-005
- [ ] TC-EDGE-001 through TC-EDGE-006
- [ ] TC-PERF-001 through TC-PERF-005

**Phase 2: Threading Tests (Day 2)**
- [ ] TC-THR-001 through TC-THR-005
- [ ] Run with ThreadSanitizer
- [ ] Verify no data races

**Phase 3: Workflow Tests (Day 3)**
- [ ] TC-WF-001 through TC-WF-006
- [ ] Manual game testing for match-end automation

**Phase 4: Integration Tests (Day 4)**
- [ ] TC-INT-001 through TC-INT-003
- [ ] Real Rocket League game testing

**Phase 5: Performance Regression (Day 5)**
- [ ] Baseline measurements
- [ ] Historical comparison
- [ ] Stress testing

### Sign-Off Criteria

- [ ] 100% critical path coverage
- [ ] 0 high-severity bugs
- [ ] 0 data races (ThreadSanitizer clean)
- [ ] 0 memory leaks (ASan clean)
- [ ] All performance targets met
- [ ] Integration tests passing
- [ ] Documentation complete

---

## Appendix A: Test Case Template

```cpp
// Template for new test cases
class WorkshopDownloaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mocks and objects
        downloader = std::make_unique<WorkshopDownloader>(mockGameWrapper);
    }

    void TearDown() override {
        // Cleanup
        downloader.reset();
    }

    std::unique_ptr<WorkshopDownloader> downloader;
    std::shared_ptr<MockGameWrapper> mockGameWrapper;
    std::shared_ptr<MockHttpWrapper> mockHttpWrapper;
};

TEST_F(WorkshopDownloaderTest, SearchCompletesWithoutPolling) {
    // Arrange
    EXPECT_CALL(*mockHttpWrapper, SendCurlRequest).Times(1);

    // Act
    downloader->GetResults("test", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Assert
    ASSERT_FALSE(downloader->RLMAPS_Searching.load());
    EXPECT_EQ(downloader->completedRequests.load(), 1);
}
```

---

## Appendix B: Debugging Guide

### Common Issues and Solutions

**Issue: Condition Variable Hangs**
- Check: expectedRequests matches actual HTTP requests
- Check: notifyCompletion() called in all code paths
- Debug: Add logging to completedRequests increments

**Issue: Memory Leak in Search**
- Check: std::vector<RLMAPS_MapResult> cleared properly
- Check: Lambda captures don't retain references
- Use: `valgrind --leak-check=full`

**Issue: Thread Race in MapResultList**
- Check: All access guarded by lock_guard
- Check: No lock_guard destructors in exception handlers
- Use: ThreadSanitizer: `clang++ -fsanitize=thread`

**Issue: Atomic Not Visible**
- Check: Using std::atomic (not volatile)
- Check: No compiler optimizations removing atomics
- Check: Sequential consistency default holds

---

## Appendix C: Performance Baseline Template

```
Environment:
  - CPU: [Model]
  - RAM: [GB]
  - Storage: [Type]
  - OS: Windows 10/11

Baseline Measurements (Baseline Date):
  - Average Search Time: [ms]
  - Peak Memory per Search: [MB]
  - Download Throughput: [MB/s]
  - Average Frame Time: [ms]

Current Run (Current Date):
  - Average Search Time: [ms] (Change: [%])
  - Peak Memory per Search: [MB] (Change: [%])
  - Download Throughput: [MB/s] (Change: [%])
  - Average Frame Time: [ms] (Change: [%])

Status: [PASS/REGRESSION]
```

---

**Document Version:** 1.0
**Last Updated:** January 31, 2026
**Author:** QA Specialist
**Status:** Ready for Implementation
