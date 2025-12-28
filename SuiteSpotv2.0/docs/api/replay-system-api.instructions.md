# ReplayWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

ReplayWrapper represents a replay being viewed or analyzed. It extends ServerWrapper with replay-specific functionality for seeking, playback control, and frame access. Use this for replay analysis plugins.

## Overview

ReplayWrapper gives you access to:
- Current playback frame/time
- Playback control (seek, play, pause)
- Replay FPS and frame count
- View target and camera control
- Key frame management
- Server state during replay (inherits from ServerWrapper)

## Constructor/Initialization

ReplayWrapper is obtained from GameWrapper when viewing a replay:

```cpp
auto gameWrapper = GetGameWrapper();

// Check if in replay
if (gameWrapper->IsInReplay()) {
    // Get replay wrapper
    auto replay = gameWrapper->GetCurrentGameState();  // Returns ReplayWrapper in replay
    if (replay) {
        // Use replay-specific methods
    }
}
```

## Playback State Methods

### Get Replay Information
```cpp
// Get elapsed replay time (in seconds)
float timeElapsed = replay->GetReplayTimeElapsed();

// Get replay FPS (frames per second)
int fps = replay->GetReplayFPS();

// Get current frame number
int currentFrame = replay->GetCurrentReplayFrame();

// Get total frame count (calculated from time/fps)
// Note: Must calculate: totalFrames = timeElapsed * fps
float totalTime = replay->GetReplayTimeElapsed();
int totalFPS = replay->GetReplayFPS();
int totalFrames = (int)(totalTime * totalFPS);
```

## Camera/View Control

### View Target
```cpp
// Get current view target (what camera is looking at)
ActorWrapper viewTarget = replay->GetViewTarget();
if (viewTarget) {
    // Get view target location
    Vector targetLoc = viewTarget->GetLocation();
}
```

## Playback Control Methods

### Seek & Skip
```cpp
// Skip to specific frame
replay->SkipToFrame(100);  // Jump to frame 100

// Skip to specific time (seconds)
replay->SkipToTime(15.5f);  // Jump to 15.5 seconds

// Start playback at frame
replay->StartPlaybackAtFrame(50);

// Start playback at time
replay->StartPlaybackAtTime(10.0f);  // Start at 10 seconds
```

## Key Frame Management

Key frames are bookmarks in the replay (like chapters):

```cpp
// Add key frame at current position
int frame = replay->GetCurrentReplayFrame();
replay->AddKeyFrame(frame, "Goal Moment");

// Remove key frame
replay->RemoveKeyFrame(100);

// Key frames help navigate long replays
// Can be used to jump to important moments (goals, saves, etc.)
```

## Common Patterns

### Replay Analysis - Get Game State at Frame
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (!gw->IsInReplay()) return;
    
    auto replay = gw->GetCurrentGameState();
    if (!replay) return;
    
    // Get current state during replay
    auto cars = replay->GetCars();
    auto ball = replay->GetBall();
    auto players = replay->GetPRIs();
    
    // Analyze current frame
    int frame = replay->GetCurrentReplayFrame();
    float time = replay->GetReplayTimeElapsed();
    
    std::cout << "Frame " << frame << " at " << time << "s\n";
}, 0.016f);  // Update every frame
```

### Jump to Goal Moments
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (!gw->IsInReplay()) return;
    
    auto replay = gw->GetCurrentGameState();
    if (!replay) return;
    
    // Find frames where goals were scored
    // Then skip to them
    
    // Example: Skip to frame 150 where goal happened
    replay->SkipToFrame(150);
}, 5.0f);
```

### Analyze Car Movement Over Replay
```cpp
std::vector<Vector> carTrajectory;

gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (!gw->IsInReplay()) return;
    
    auto replay = gw->GetCurrentGameState();
    if (!replay) return;
    
    auto cars = replay->GetCars();
    if (cars.Count() > 0) {
        auto car = cars.Get(0);
        if (car) {
            carTrajectory.push_back(car->GetLocation());
        }
    }
    
    // Continuously record car positions as replay plays
}, 0.05f);
```

### Navigate Replay with Key Frames
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventGoalScored",
    [this](void* params) {
        auto gw = GetGameWrapper();
        if (!gw->IsInReplay()) return;
        
        auto replay = gw->GetCurrentGameState();
        if (replay) {
            // Add key frame when goal scored in replay
            int frame = replay->GetCurrentReplayFrame();
            replay->AddKeyFrame(frame, "Goal!");
        }
    }
);
```

### Get View Target Trajectory
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (!gw->IsInReplay()) return;
    
    auto replay = gw->GetCurrentGameState();
    if (!replay) return;
    
    auto viewTarget = replay->GetViewTarget();
    if (viewTarget) {
        Vector location = viewTarget->GetLocation();
        float time = replay->GetReplayTimeElapsed();
        
        std::cout << "View at " << time << "s: (" 
                  << location.X << ", " << location.Y << ", " 
                  << location.Z << ")\n";
    }
}, 1.0f);
```

### Scrub Through Replay Frame-by-Frame
```cpp
int targetFrame = 0;
int maxFrame = 300;

gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (!gw->IsInReplay()) return;
    
    auto replay = gw->GetCurrentGameState();
    if (!replay) return;
    
    // Skip to target frame
    replay->SkipToFrame(targetFrame);
    
    // Get state at this frame
    auto cars = replay->GetCars();
    auto ball = replay->GetBall();
    
    std::cout << "Frame " << targetFrame << ": "
              << cars.Count() << " cars, "
              << (ball ? "ball exists" : "no ball") << "\n";
    
    // Move to next frame (depends on FPS)
    targetFrame += 5;  // Skip 5 frames at a time
    
    if (targetFrame > maxFrame) {
        targetFrame = 0;  // Loop back
    }
}, 2.0f);
```

## Inheritance: ServerWrapper Methods

ReplayWrapper extends ServerWrapper, so you also have access to:
- GetCars()
- GetBall()
- GetTeams()
- GetPRIs()
- GetScore()
- GetGameMode()
- And all other ServerWrapper methods

```cpp
auto replay = gw->GetCurrentGameState();

// Inherited from ServerWrapper
auto cars = replay->GetCars();
auto ball = replay->GetBall();
auto teams = replay->GetTeams();
int gameMode = replay->GetGameMode();
float timeRemaining = replay->GetGameTimeRemaining();
```

## Thread Safety Notes

- **Safe to call from**: Game thread (inside hooks, SetTimeout callbacks)
- **Not safe to call from**: Other threads without synchronization
- **Wrapper lifetime**: Valid only during replay viewing; becomes null when exiting replay
- **Always null-check**: `if (!replay) return;` before using

```cpp
// SAFE
gameWrapper->SetTimeout([this](GameWrapper gw) {
    if (!gw->IsInReplay()) return;
    
    auto replay = gw->GetCurrentGameState();
    if (!replay) return;  // NULL CHECK
    
    int frame = replay->GetCurrentReplayFrame();
}, 0.016f);

// UNSAFE - Storing replay
auto cachedReplay = replay;
// ... later, if exit replay ...
cachedReplay->GetCurrentReplayFrame();  // Might be invalid!
```

## Performance Considerations

- **GetCurrentReplayFrame()** is O(1); fast
- **SkipToFrame()** can be slow depending on seek direction and distance
- **GetCars(), GetBall()** are inherited ServerWrapper calls; safe to call frequently
- **Playback during analysis** - Continuous calls to SkipToFrame() can stutter playback

```cpp
// EFFICIENT - Cache state
auto gw = GetGameWrapper();
auto replay = gw->GetCurrentGameState();
if (replay) {
    int frame = replay->GetCurrentReplayFrame();
    float time = replay->GetReplayTimeElapsed();
    auto cars = replay->GetCars();
    // Use frame, time, cars
}

// INEFFICIENT - Multiple GetCurrentGameState calls
int frame = gw->GetCurrentGameState()->GetCurrentReplayFrame();
float time = gw->GetCurrentGameState()->GetReplayTimeElapsed();
auto cars = gw->GetCurrentGameState()->GetCars();
```

## SuiteSpot-Specific Usage

While SuiteSpot is for map loading, you could extend it for replay analysis:

```cpp
// Example: Detect replay viewing
if (gameWrapper->IsInReplay()) {
    // User is watching a replay
    // Could disable map loading or show replay info
}
```

## Important Notes for AI Agents

1. **Check IsInReplay() first** - Never assume GetCurrentGameState() is a ReplayWrapper
2. **Frame numbers are sequential** - Start at 0, increment by 1
3. **Time in seconds** - GetReplayTimeElapsed() is in seconds
4. **SkipToFrame is seekable** - Can jump forward or backward instantly
5. **Key frames are optional** - Used for navigation, not required
6. **View target can be null** - Not all replays have a tracked view target
7. **All ServerWrapper data is accessible** - Use inherited methods
8. **Continuous skipping can be slow** - Batch seeks when possible
9. **Replay data is read-only** - You can't modify replay data
10. **FPS varies by replay** - Always query GetReplayFPS()

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/replaywrapper.h
**Parent Class:** ServerWrapper (inherited methods available)
