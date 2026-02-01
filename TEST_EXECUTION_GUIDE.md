# SuiteSpot Test Execution Guide

**Version:** 1.0
**Date:** January 31, 2026
**Status:** Ready for Implementation

---

## Quick Start

### Prerequisites

1. **Visual Studio 2022** with C++20 support
2. **Google Test (gtest)** framework
3. **Google Mock (gmock)** for mocking
4. **CMake** or add tests directly to Visual Studio project

### Setup Instructions

```bash
# Step 1: Install gtest via vcpkg (already in your environment)
cd C:\Users\bmile\vcpkg
.\vcpkg install gtest:x64-windows

# Step 2: Create test project structure
mkdir C:\Users\bmile\Source\SuiteSpot\tests
mkdir C:\Users\bmile\Source\SuiteSpot\tests\unit
mkdir C:\Users\bmile\Source\SuiteSpot\tests\integration
mkdir C:\Users\bmile\Source\SuiteSpot\tests\threading
mkdir C:\Users\bmile\Source\SuiteSpot\tests\performance
mkdir C:\Users\bmile\Source\SuiteSpot\tests\mocks

# Step 3: Create CMakeLists.txt for tests
# (See section below)
```

---

## Test Project Structure

```
SuiteSpot/
├── SuiteSpot.sln
├── CMakeLists.txt (main project)
├── tests/
│   ├── CMakeLists.txt (test configuration)
│   ├── unit/
│   │   ├── WorkshopDownloaderTests.cpp
│   │   ├── AutoLoadFeatureTests.cpp
│   │   ├── SettingsSyncTests.cpp
│   │   ├── TrainingPackManagerTests.cpp
│   │   └── MapManagerTests.cpp
│   ├── integration/
│   │   ├── AutoLoadFeatureTests.cpp
│   │   ├── WorkshopPipelineTests.cpp
│   │   └── SettingsPersistenceTests.cpp
│   ├── threading/
│   │   ├── ConcurrencyTests.cpp
│   │   └── ThreadSafeSemaphoreTests.cpp
│   ├── performance/
│   │   ├── PerformanceRegressionTests.cpp
│   │   └── BenchmarkTests.cpp
│   └── mocks/
│       ├── MockGameWrapper.h
│       ├── MockCVarManager.h
│       └── MockHttpWrapper.h
└── TEST_PLAN.md
```

---

## CMakeLists.txt Configuration

### For tests/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(SuiteSpotTests)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(GTest REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories(${CMAKE_SOURCE_DIR}/mocks)

# Test executable
add_executable(SuiteSpotUnitTests
    unit/WorkshopDownloaderTests.cpp
    unit/AutoLoadFeatureTests.cpp
    unit/SettingsSyncTests.cpp
)

target_link_libraries(SuiteSpotUnitTests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

# Integration tests
add_executable(SuiteSpotIntegrationTests
    integration/AutoLoadFeatureTests.cpp
    integration/WorkshopPipelineTests.cpp
)

target_link_libraries(SuiteSpotIntegrationTests
    GTest::gtest
    GTest::gtest_main
)

# Threading tests
add_executable(SuiteSpotThreadingTests
    threading/ConcurrencyTests.cpp
)

target_link_libraries(SuiteSpotThreadingTests
    GTest::gtest
    GTest::gtest_main
)

# Enable testing
enable_testing()
add_test(NAME SuiteSpotUnitTests COMMAND SuiteSpotUnitTests)
add_test(NAME SuiteSpotIntegrationTests COMMAND SuiteSpotIntegrationTests)
add_test(NAME SuiteSpotThreadingTests COMMAND SuiteSpotThreadingTests)
```

---

## Building Tests

### Option A: Using CMake

```bash
# Generate build files
cd C:\Users\bmile\Source\SuiteSpot
cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# Run all tests
ctest --output-on-failure
```

### Option B: Visual Studio (Direct Integration)

1. Create "SuiteSpot.Tests" project in Visual Studio
2. Add test files to the project
3. Configure project properties:
   - C++ Language Standard: C++20
   - Add gtest include directory: `C:\Users\bmile\vcpkg\installed\x64-windows\include`
   - Add gtest library directory: `C:\Users\bmile\vcpkg\installed\x64-windows\lib`
   - Link libraries: `gtest.lib`, `gtest_main.lib`, `gmock.lib`
4. Build and run from Test Explorer (Ctrl+E, T)

---

## Running Tests

### Basic Test Execution

```bash
# Run all tests
.\build\Debug\SuiteSpotUnitTests

# Run specific test suite
.\build\Debug\SuiteSpotUnitTests --gtest_filter=WorkshopDownloaderTest.*

# Run specific test case
.\build\Debug\SuiteSpotUnitTests --gtest_filter=WorkshopDownloaderTest.SearchCompletesWithoutPolling

# List all tests without running
.\build\Debug\SuiteSpotUnitTests --gtest_list_tests
```

### Visual Studio Test Explorer

```
Test → Test Explorer → Run Tests → Debug Tests
(Or press Ctrl+R, A to run all tests)
```

### With Detailed Output

```bash
# Verbose output
.\build\Debug\SuiteSpotUnitTests --gtest_print_time=1 --gtest_verbose

# Repeat tests multiple times
.\build\Debug\SuiteSpotUnitTests --gtest_repeat=10
```

---

## Test Execution Phases

### Phase 1: Unit Tests (Critical Path)

**Duration:** ~30 minutes
**Focus:** WorkshopDownloader HTTP callback fixes

```bash
# Run WorkshopDownloader tests
.\build\Debug\SuiteSpotUnitTests --gtest_filter=WorkshopDownloaderTest.*

# Expected: All TC-WD-* tests pass
# Critical: TC-WD-001 through TC-WD-005
```

**Sign-off Criteria:**
- [ ] TC-WD-001: Callback completion without polling
- [ ] TC-WD-002: Concurrent search isolation
- [ ] TC-WD-003: Generation invalidation
- [ ] TC-WD-004: Progress tracking
- [ ] TC-WD-005a/b/c/d: Download flag in all paths
- [ ] Edge case tests all pass

---

### Phase 2: Threading Safety Tests

**Duration:** ~1 hour
**Focus:** Concurrency and thread safety

```bash
# Run threading tests
.\build\Debug\SuiteSpotThreadingTests --gtest_filter=ConcurrencyTest.*

# Run with thread sanitizer (optional, requires Clang)
clang++ -fsanitize=thread build/threading/*.obj -o test_threaded
./test_threaded
```

**Sign-off Criteria:**
- [ ] TC-THR-001: Concurrent searches don't crash
- [ ] TC-THR-002: Atomic visibility across threads
- [ ] TC-THR-003: Mutex protects shared data
- [ ] TC-THR-004: Condition variable signals correctly
- [ ] TC-THR-005: Detached threads complete properly
- [ ] 0 data races reported

---

### Phase 3: Integration Tests

**Duration:** ~1 hour
**Focus:** Workflow correctness

```bash
# Run integration tests
.\build\Debug\SuiteSpotIntegrationTests --gtest_filter=AutoLoadFeatureTest.*

# Include manual testing with actual game
# (Documented in separate section)
```

**Sign-off Criteria:**
- [ ] TC-WF-001: Match-end automation all modes
- [ ] TC-WF-002: Pack selection and loading
- [ ] TC-WF-003: Workshop download pipeline
- [ ] TC-WF-004: Settings persistence
- [ ] TC-WF-005: Loadout switching
- [ ] TC-WF-006: Usage tracking

---

### Phase 4: Edge Cases and Error Handling

**Duration:** ~45 minutes
**Focus:** Robustness and error conditions

```bash
# Run edge case tests
.\build\Debug\SuiteSpotUnitTests --gtest_filter=*EdgeCase*

# Expected: Graceful handling of all error scenarios
```

**Sign-off Criteria:**
- [ ] TC-EDGE-001: Empty search results
- [ ] TC-EDGE-002: Network timeouts
- [ ] TC-EDGE-003: Corrupt downloads
- [ ] TC-EDGE-004: Invalid pack codes
- [ ] TC-EDGE-005: Zero-delay handling
- [ ] TC-EDGE-006: Concurrent downloads

---

### Phase 5: Performance Regression

**Duration:** ~2 hours
**Focus:** Performance baselines and limits

```bash
# Run performance tests
.\build\Release\SuiteSpotPerformanceTests

# Measure memory usage
.\build\Release\SuiteSpotPerformanceTests --gtest_filter=*Memory*

# Measure search speed
.\build\Release\SuiteSpotPerformanceTests --gtest_filter=*Performance*
```

**Sign-off Criteria:**
- [ ] Search completes in <5 seconds
- [ ] No frame drops >5ms during operations
- [ ] Memory usage <10MB per search (1000 maps)
- [ ] Logging overhead <1%

---

## Debugging Failed Tests

### Enable Detailed Logging

```cpp
// In your test setup
class MyTest : public ::testing::Test {
    void SetUp() override {
        // Enable verbose logging
        logging::SetLevel(logging::LEVEL_DEBUG);
        logging::EnableFile("test_output.log");
    }
};
```

### Run Single Test with Breakpoint

```cpp
// Add this to your test or test fixture
#ifdef _DEBUG
    __debugbreak();  // Triggers debugger
#endif
```

### Common Issues and Solutions

**Issue: Test hangs indefinitely**
```
Likely cause: Condition variable deadlock or infinite loop
Solution: Check mutex lock ordering, ensure notify_one() called
Debug: Set timeout in wait_for() instead of wait()
```

**Issue: Race condition on access to std::vector**
```
Likely cause: Missing lock_guard around RLMAPS_MapResultList access
Solution: Add std::lock_guard<std::mutex> lock(resultsMutex)
Verify: Use ThreadSanitizer to detect data races
```

**Issue: Callback never called**
```
Likely cause: Mock not configured correctly or callback destroyed
Solution: Verify mock EXPECT_CALL() matches actual call signature
Check: Ensure callback lambda not out of scope
```

**Issue: Atomic variable not visible**
```
Likely cause: Using volatile instead of std::atomic
Solution: Verify using std::atomic<T> with default sequential consistency
Check: No compiler optimizations removing atomic ops
```

---

## Test Maintenance

### Updating Tests After Code Changes

1. **Identify affected test cases:**
   ```bash
   grep -r "FunctionName" tests/
   ```

2. **Update test expectations:**
   ```cpp
   // Old expectation
   EXPECT_CALL(mock, Method()).Times(1);

   // New expectation (if behavior changed)
   EXPECT_CALL(mock, Method()).Times(2);
   ```

3. **Re-run affected tests:**
   ```bash
   cmake --build build
   ctest --output-on-failure
   ```

### Adding New Tests

1. Create test file in appropriate directory (unit/integration/threading)
2. Follow naming convention: `*Tests.cpp`
3. Use test fixture pattern:
   ```cpp
   class MyFeatureTest : public ::testing::Test {
       void SetUp() override { /* initialize */ }
       void TearDown() override { /* cleanup */ }
   };
   ```
4. Add to CMakeLists.txt
5. Rebuild and verify

---

## Continuous Integration Setup

### GitHub Actions Configuration

Create `.github/workflows/test.yml`:

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest

    steps:
      - uses: actions/checkout@v2

      - name: Set up Visual Studio
        uses: microsoft/setup-msbuild@v1

      - name: Install vcpkg dependencies
        run: |
          vcpkg install gtest:x64-windows

      - name: Build tests
        run: cmake --build build --config Release

      - name: Run tests
        run: ctest --output-on-failure
```

### Local CI Simulation

```bash
# Script: run_all_tests.ps1

# Clean
Remove-Item -Recurse build -ErrorAction SilentlyContinue

# Build
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# Run tests
ctest --output-on-failure --repeat until-pass:3

# Generate report
ctest --output-on-failure -T coverage

exit $LastExitCode
```

---

## Test Report Generation

### Create Test Summary

```bash
# Generate XML report
.\build\Release\SuiteSpotUnitTests --gtest_output=xml:test_results.xml

# Parse and display
powershell -Command {
    [xml]$results = Get-Content test_results.xml
    Write-Host "Total Tests: $($results.testsuites.tests)"
    Write-Host "Passed: $($results.testsuites.tests - $results.testsuites.failures)"
    Write-Host "Failed: $($results.testsuites.failures)"
}
```

### Coverage Report

```bash
# Using OpenCppCoverage (Windows)
OpenCppCoverage.exe --sources SuiteSpot --modules SuiteSpot.dll -- build\Release\SuiteSpotUnitTests

# View HTML report
start coverage\index.html
```

---

## Performance Baseline

### Establish Baseline

```bash
# Run once to establish baseline
.\build\Release\SuiteSpotPerformanceTests --gtest_filter=*Perf* > baseline.txt

# Extract metrics
powershell -Command {
    $content = Get-Content baseline.txt
    $content | Select-String "PASSED|FAILED|time:"
}
```

### Compare Against Baseline

```bash
# Run current tests
.\build\Release\SuiteSpotPerformanceTests > current.txt

# Compare
Compare-Object -ReferenceObject (Get-Content baseline.txt) -DifferenceObject (Get-Content current.txt)
```

---

## Checklist for Test Sign-Off

### Pre-Test
- [ ] All source files compile without errors
- [ ] Test environment clean (no stale builds)
- [ ] Required dependencies installed (gtest, gmock)
- [ ] Network connectivity available (for integration tests)

### Test Execution
- [ ] Unit tests pass (100% of TC-WD-* and TC-EDGE-*)
- [ ] Threading tests pass (0 data races)
- [ ] Integration tests pass (all match-end scenarios)
- [ ] Edge case tests pass (error handling)
- [ ] Performance tests pass (within thresholds)

### Post-Test
- [ ] All logs reviewed for unexpected errors
- [ ] Memory leak check passed (no leaks detected)
- [ ] Thread safety verified (ThreadSanitizer clean)
- [ ] Coverage report reviewed (>85% coverage)
- [ ] Performance metrics within baseline ±10%

### Final Sign-Off
- [ ] QA Lead approval
- [ ] Code review complete
- [ ] No open critical issues
- [ ] Documentation updated
- [ ] Ready for release

---

## Support and Troubleshooting

### Get Test Help

```bash
# Show gtest help
.\build\Debug\SuiteSpotUnitTests --help

# Common flags
--gtest_filter=<pattern>        # Run specific tests
--gtest_repeat=<count>          # Repeat tests N times
--gtest_shuffle                 # Randomize test order
--gtest_death_test_style=<type> # Death test style
```

### Debug in Visual Studio

1. **Set breakpoint** in test code (F9)
2. **Run tests** from Test Explorer → Debug Selected Tests
3. **Step through** with F10/F11
4. **Inspect variables** in Watch window

### Memory Debugging

```bash
# Enable memory tracking in Debug build
set CFLAGS=/D_DEBUG /D_CRTDBG_MAP_ALLOC

# Run with leak detection
_CrtCheckMemory();  // In test teardown
```

---

## Test Documentation Template

When creating new tests, use this template:

```cpp
// TC-ID: Test Case Identifier (e.g., TC-WD-001)
// Objective: What this test verifies
// Setup: Initial state and mocks
// Test Steps: Numbered steps
// Acceptance Criteria: What must be true

TEST_F(FixtureClass, TestName) {
    // ARRANGE: Setup test conditions

    // ACT: Execute the code under test

    // ASSERT: Verify expected outcomes
}
```

---

**Document Version:** 1.0
**Last Updated:** January 31, 2026
**Status:** Ready for Implementation
