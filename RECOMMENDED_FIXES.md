# SuiteSpot - Recommended Code Fixes

## Priority 1: Fix Detached Thread Lifecycle Safety

**File:** `WorkshopDownloader.cpp`
**Lines:** 35-36, 81-82
**Severity:** HIGH
**Effort:** 2-3 hours

### Current Code (UNSAFE)
```cpp
// Line 35-36
std::thread t2(&WorkshopDownloader::GetNumPages, this, keyWord);
t2.detach();

// Line 81-82
std::thread t2(&WorkshopDownloader::GetMapResult, this, actualJson, index, currentGeneration);
t2.detach();
```

### Problem
- Detached thread holds raw `this` pointer with no lifetime guarantee
- If `WorkshopDownloader` destroyed while thread running, dangling pointer
- Thread would access freed memory
- In practice: Safe due to plugin lifecycle, but violates RAII

### Recommended Fix

**Option A: Store Threads in Vector (Best for This Codebase)**
```cpp
// In WorkshopDownloader.h
private:
    std::vector<std::thread> activeThreads;  // Add this member
    std::mutex threadsMutex;                 // Protect the vector

// In ~WorkshopDownloader() (add destructor if not present)
WorkshopDownloader::~WorkshopDownloader() {
    // Wait for all threads to complete
    for (auto& thread : activeThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// Replace detach pattern with:
{
    std::lock_guard<std::mutex> lock(threadsMutex);
    activeThreads.emplace_back(&WorkshopDownloader::GetNumPages, this, keyWord);
    // No detach - threads stored and will be joined in destructor
}
```

**Option B: Use std::async (Simplest)**
```cpp
// In WorkshopDownloader.h
private:
    std::vector<std::future<void>> activeFutures;

// Replace detach with:
{
    std::lock_guard<std::mutex> lock(threadsMutex);
    auto future = std::async(std::launch::async,
                            &WorkshopDownloader::GetNumPages, this, keyWord);
    activeFutures.push_back(std::move(future));
}

// In destructor:
for (auto& future : activeFutures) {
    try {
        future.get();  // Wait for completion
    } catch (...) {
        // Handle exception if needed
    }
}
```

**Option C: Use Thread Pool (Most Professional)**
```cpp
// Would require adding a thread pool library or implementing simple pool
// See refactoring section for long-term approach
```

### Recommendation
Use **Option A** for this sprint. Provides maximum control and clarity.

---

## Priority 2: Protect FolderErrorText Data Race

**File:** `WorkshopDownloader.cpp` and `.h`
**Lines:** Line 75 (.h), Line 287 (.cpp)
**Severity:** HIGH
**Effort:** 30 minutes

### Current Code (UNSAFE)
```cpp
// WorkshopDownloader.h - Line 75
std::atomic<bool> FolderErrorBool = false;
std::string FolderErrorText;  // ← NO SYNCHRONIZATION!

// WorkshopDownloader.cpp - Line 287 (written in callback thread)
FolderErrorText = ex.what();
FolderErrorBool = true;

// UI thread reads this somewhere...
// (likely in SettingsUI.cpp or render code)
```

### Problem
- `FolderErrorText` written by HTTP callback thread
- Potentially read by UI thread
- No mutex protects access
- Could cause torn reads or writes
- String modification is not atomic

### Recommended Fix

**Option A: Protect Under Existing Mutex (RECOMMENDED)**
```cpp
// In WorkshopDownloader.cpp around line 287
if (code == 200) {
    // ... existing success code ...
} else {
    LOG("Workshop download failed with code {}", code);
    {
        std::lock_guard<std::mutex> lock(resultsMutex);  // Use existing mutex
        FolderErrorText = "Download failed with HTTP code " + std::to_string(code);
        FolderErrorBool = true;
    }
}

// Also wrap line 287 (in earlier error path)
catch (const std::exception& ex) {
    LOG("Failed to create directory: {}", ex.what());
    {
        std::lock_guard<std::mutex> lock(resultsMutex);
        FolderErrorText = ex.what();
        FolderErrorBool = true;
    }
}
```

**Option B: Create Separate Mutex (More Explicit)**
```cpp
// In WorkshopDownloader.h
private:
    std::atomic<bool> FolderErrorBool = false;
    std::string FolderErrorText;
    mutable std::mutex folderErrorMutex;  // New mutex

// Then use:
{
    std::lock_guard<std::mutex> lock(folderErrorMutex);
    FolderErrorText = ex.what();
}
FolderErrorBool = true;  // Atomic, no lock needed
```

### Recommendation
Use **Option A**. Reuses existing `resultsMutex` which already protects similar data.

### Testing
```cpp
// Add unit test to verify thread safety:
TEST(WorkshopDownloader, FolderErrorThreadSafety) {
    WorkshopDownloader wd(...);

    // Simulate concurrent write and read
    std::thread writer([&]() {
        for (int i = 0; i < 100; ++i) {
            // trigger error condition
        }
    });

    std::thread reader([&]() {
        for (int i = 0; i < 100; ++i) {
            std::string text = wd.FolderErrorText;
            // Should not crash or return garbage
        }
    });

    writer.join();
    reader.join();
}
```

---

## Priority 3: Migrate to Smart Pointers

**File:** `SuiteSpot.cpp`
**Lines:** 322-328 (construction), 371-382 (destruction)
**Severity:** MEDIUM
**Effort:** 1-2 hours

### Current Code (MANUAL MANAGEMENT)
```cpp
// SuiteSpot.cpp:319 - In onLoad()
void SuiteSpot::onLoad() {
    // ...

    mapManager = new MapManager();           // Line 322
    settingsSync = new SettingsSync();       // Line 323
    autoLoadFeature = new AutoLoadFeature(); // Line 324
    trainingPackMgr = new TrainingPackManager();  // Line 325
    settingsUI = new SettingsUI(this);       // Line 326
    workshopDownloader = std::make_unique<WorkshopDownloader>(...); // 327 (already good!)
    loadoutUI = new LoadoutUI(this);         // Line 328

    // ...
}

// SuiteSpot.cpp:369 - In onUnload()
void SuiteSpot::onUnload() {
    // Note: Order differs from construction!
    delete settingsUI;      // Was line 326
    delete loadoutUI;       // Was line 328
    delete trainingPackMgr; // Was line 325
    delete autoLoadFeature; // Was line 324
    delete settingsSync;    // Was line 323
    delete mapManager;      // Was line 322
}
```

### Problems
1. Manual cleanup required
2. If exception thrown during onLoad(), allocated objects leak
3. Deletion order differs from construction
4. Inconsistent with already-smart-pointer members (loadoutManager, workshopDownloader)

### Recommended Fix

**Step 1: Update SuiteSpot.h Members**
```cpp
// SuiteSpot.h - Replace raw pointers
private:
    // BEFORE:
    // MapManager* mapManager = nullptr;
    // SettingsSync* settingsSync = nullptr;
    // AutoLoadFeature* autoLoadFeature = nullptr;
    // TrainingPackManager* trainingPackMgr = nullptr;
    // SettingsUI* settingsUI = nullptr;
    // LoadoutUI* loadoutUI = nullptr;

    // AFTER:
    std::unique_ptr<MapManager> mapManager;
    std::unique_ptr<SettingsSync> settingsSync;
    std::unique_ptr<AutoLoadFeature> autoLoadFeature;
    std::unique_ptr<TrainingPackManager> trainingPackMgr;
    std::unique_ptr<SettingsUI> settingsUI;
    std::unique_ptr<LoadoutUI> loadoutUI;
```

**Step 2: Update SuiteSpot.cpp onLoad()**
```cpp
void SuiteSpot::onLoad() {
    // ... event handling setup ...

    // Create all components with std::make_unique
    mapManager = std::make_unique<MapManager>();
    settingsSync = std::make_unique<SettingsSync>();
    autoLoadFeature = std::make_unique<AutoLoadFeature>();
    trainingPackMgr = std::make_unique<TrainingPackManager>();
    settingsUI = std::make_unique<SettingsUI>(this);
    loadoutUI = std::make_unique<LoadoutUI>(this);

    // ... rest of initialization ...
}
```

**Step 3: Remove onUnload() Manual Cleanup**
```cpp
// BEFORE:
void SuiteSpot::onUnload() {
    delete settingsUI;
    delete loadoutUI;
    delete trainingPackMgr;
    delete autoLoadFeature;
    delete settingsSync;
    delete mapManager;
}

// AFTER: (can be completely removed!)
// unique_ptr handles cleanup automatically
void SuiteSpot::onUnload() {
    // All cleanup is automatic via unique_ptr destructors
    // No manual delete needed
}
```

**Step 4: Update Accessors (Minor Changes)**
```cpp
// Already using null checks, which still work:
std::filesystem::path SuiteSpot::GetDataRoot() const {
    return mapManager ? mapManager->GetDataRoot() : std::filesystem::path();
    // ↑ This still works because unique_ptr has operator bool
}
```

### Benefits
- ✅ Exception safe during initialization
- ✅ Automatic cleanup
- ✅ Matches modern C++ style (like existing workshopDownloader)
- ✅ No need for manual destructor
- ✅ Prevents double-delete bugs

### Migration Checklist
- [ ] Update SuiteSpot.h member declarations
- [ ] Update SuiteSpot.cpp onLoad() allocations
- [ ] Remove SuiteSpot.cpp onUnload() deletions
- [ ] Verify SettingsUI constructor takes raw pointer (OK)
- [ ] Verify LoadoutUI constructor takes raw pointer (OK)
- [ ] Test plugin loads and unloads correctly
- [ ] Test no memory leaks on exit

---

## Priority 4: Add Search Completion Timeout

**File:** `WorkshopDownloader.cpp`
**Lines:** 87-91
**Severity:** MEDIUM
**Effort:** 1 hour

### Current Code (NO TIMEOUT)
```cpp
// Line 87-91
std::unique_lock<std::mutex> lock(resultsMutex);
resultsCV.wait(lock, [this, expectedRequests, currentGeneration]() {
    return completedRequests.load() >= expectedRequests ||
           searchGeneration.load() != currentGeneration;
});
// No timeout! If callback never fires, waits forever.
```

### Problem
- If one HTTP request hangs/times out
- `completedRequests` never reaches `expectedRequests`
- Condition variable waits indefinitely
- UI thread is blocked
- User sees frozen/unresponsive UI

### Recommended Fix

**Replace with wait_until**
```cpp
// Line 87-95 (revised)
std::unique_lock<std::mutex> lock(resultsMutex);
auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(30);
bool completed = resultsCV.wait_until(lock, deadline, [this, expectedRequests, currentGeneration]() {
    return completedRequests.load() >= expectedRequests ||
           searchGeneration.load() != currentGeneration;
});

if (!completed) {
    LOG("Workshop search timeout after 30 seconds (completed {}/{})",
        completedRequests.load(), expectedRequests);
    RLMAPS_Searching = false;  // Release lock for next search
    return;  // Early exit
}

RLMAPS_Searching = false;
```

### Alternative: Configurable Timeout
```cpp
// In WorkshopDownloader.h
private:
    int search_timeout_seconds = 30;  // Configurable

// In WorkshopDownloader.cpp
auto deadline = std::chrono::system_clock::now()
              + std::chrono::seconds(search_timeout_seconds);
```

### Testing
```cpp
TEST(WorkshopDownloader, SearchTimesOutAfter30Seconds) {
    // Mock HttpWrapper to never call callback
    // Search should timeout and release lock
    auto start = std::chrono::system_clock::now();
    downloader.GetResults("test", 1);
    auto elapsed = std::chrono::system_clock::now() - start;

    EXPECT_GT(elapsed, std::chrono::seconds(30));
    EXPECT_LT(elapsed, std::chrono::seconds(35));
    EXPECT_FALSE(downloader.RLMAPS_Searching.load());
}
```

---

## Priority 5: Fix Cleanup Order

**File:** `SuiteSpot.cpp`
**Lines:** 371-382
**Severity:** LOW
**Effort:** 15 minutes

### Current Code (WRONG ORDER)
```cpp
// Constructed order (lines 322-328):
mapManager = new MapManager();           // 1st
settingsSync = new SettingsSync();       // 2nd
autoLoadFeature = new AutoLoadFeature(); // 3rd
trainingPackMgr = new TrainingPackManager();  // 4th
settingsUI = new SettingsUI(this);       // 5th
workshopDownloader = ...                 // 6th
loadoutUI = new LoadoutUI(this);         // 7th

// Deleted order (lines 371-382):
delete settingsUI;      // 5th constructed, 1st deleted ✗
delete loadoutUI;       // 7th constructed, 2nd deleted ✗
delete trainingPackMgr; // 4th constructed, 3rd deleted ✗
delete autoLoadFeature; // 3rd constructed, 4th deleted ✗
delete settingsSync;    // 2nd constructed, 5th deleted ✗
delete mapManager;      // 1st constructed, 6th deleted ✗
```

### Problem
- UI components deleted before managers they depend on
- If SettingsUI destructor accesses settingsSync, will be freed already
- Works now but fragile design
- Violates RAII principle of "reverse order" cleanup

### Recommended Fix

**Correct Order (Reverse of Construction)**
```cpp
// In onUnload() - Lines 371-382 (revised)
void SuiteSpot::onUnload() {
    // Delete in REVERSE construction order

    delete loadoutUI;           // 7th → 1st delete
    delete workshopDownloader;  // 6th → 2nd delete (if manually allocated)
    delete settingsUI;          // 5th → 3rd delete
    delete trainingPackMgr;     // 4th → 4th delete
    delete autoLoadFeature;     // 3rd → 5th delete
    delete settingsSync;        // 2nd → 6th delete
    delete mapManager;          // 1st → 7th delete
}
```

**Or Better: Use Smart Pointers (See Priority 3)**
```cpp
// If using smart pointers, no manual cleanup needed at all!
// The destructors run automatically in the correct reverse order
```

### Verification
```cpp
// Add comment documenting order
void SuiteSpot::onUnload() {
    // Destroy in reverse construction order
    // Construction:    mapManager, settingsSync, autoLoadFeature, trainingPackMgr, settingsUI, workshopDownloader, loadoutUI
    // Destruction:     loadoutUI, workshopDownloader, settingsUI, trainingPackMgr, autoLoadFeature, settingsSync, mapManager

    delete loadoutUI;
    delete workshopDownloader;
    delete settingsUI;
    delete trainingPackMgr;
    delete autoLoadFeature;
    delete settingsSync;
    delete mapManager;
}
```

---

## Implementation Timeline

### This Sprint (1 Week)
1. **Priority 1** (2-3 hours): Fix detached thread lifecycle
2. **Priority 2** (30 min): Protect FolderErrorText
3. **Priority 5** (15 min): Fix cleanup order
4. **Testing** (2 hours): Verify no regressions

### Next Sprint (1 Week)
1. **Priority 3** (1-2 hours): Migrate to smart pointers
2. **Priority 4** (1 hour): Add search timeout
3. **Refactoring** (2-3 hours): Extract search logic
4. **Testing** (3 hours): Unit tests for new patterns

### Total Effort
- **This Sprint:** ~6 hours
- **Next Sprint:** ~7-8 hours
- **Total:** ~13-14 hours over 2 sprints

---

## Testing Recommendations

### Unit Tests to Add
```cpp
// WorkshopDownloaderTest.cpp
TEST(WorkshopDownloader, ThreadLifecycleThreadSafety);
TEST(WorkshopDownloader, FolderErrorThreadSafety);
TEST(WorkshopDownloader, SearchTimesOutAfter30Seconds);
TEST(WorkshopDownloader, MultipleSearchesCancelPrevious);

// SuiteSpotTest.cpp
TEST(SuiteSpot, ComponentsInitializeInOrder);
TEST(SuiteSpot, ComponentsCleanupInReverseOrder);
TEST(SuiteSpot, NoMemoryLeaksOnPluginLoad);
```

### Manual Testing Checklist
- [ ] Plugin loads without crashes
- [ ] Can search workshop maps
- [ ] Can download maps
- [ ] UI responsive during download
- [ ] Progress bar updates smoothly
- [ ] Multiple downloads work sequentially
- [ ] Network timeout handled gracefully
- [ ] Plugin unloads cleanly

---

## Questions? Clarifications?

Each fix includes:
- ✅ Current problematic code
- ✅ Explanation of the problem
- ✅ Recommended solution(s)
- ✅ Code snippet showing fix
- ✅ Testing approach
- ✅ Effort estimate

Implement in priority order. All fixes are independent and can be done in any order within a sprint.

**Recommendation:** Start with Priority 1 & 2 this week (high risk, low effort). Move Priority 3 to next sprint (larger refactor).
