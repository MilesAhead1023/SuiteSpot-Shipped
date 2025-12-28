# Streaming & Networking API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

This reference covers networking, online features, and streaming-related systems in BakkesMod plugins. Includes HttpWrapper for web requests and networking utilities.

## Overview

Streaming & Networking provides:
- HTTP requests (GET, POST)
- Online game detection
- Connection status
- Network information
- Web communication

## HttpWrapper

HttpWrapper provides simple HTTP functionality:

```cpp
auto gameWrapper = GetGameWrapper();
auto httpWrapper = gameWrapper->GetHttpWrapper();
if (!httpWrapper) return;

// Make HTTP requests
```

### HTTP Requests

```cpp
// GET request (fetch data)
httpWrapper->Get("https://api.example.com/data", 
    [this](std::string response) {
        std::cout << "Response: " << response << "\n";
    }
);

// POST request (send data)
httpWrapper->Post(
    "https://api.example.com/submit",
    "key=value&other=data",
    [this](std::string response) {
        std::cout << "Posted, response: " << response << "\n";
    }
);
```

## Online Game Detection

### Check Online vs Offline
```cpp
auto gameWrapper = GetGameWrapper();

// Check if currently in online match
bool onlineGame = gameWrapper->IsOnlineGame();

if (onlineGame) {
    std::cout << "Playing online\n";
} else {
    std::cout << "Playing offline (freeplay/training)\n";
}
```

## Connection & Network State

### Game State Queries
```cpp
auto server = gameWrapper->GetCurrentGameState();
if (!server) return;

// Get world info
WorldInfoWrapper worldInfo = server->GetWorldInfo();

// Get game info
GameEventWrapper gameEvent = server->GetGameEvent();

// Server-level queries
bool inGame = gameWrapper->IsInGame();
bool inFreeplay = gameWrapper->IsInFreeplay();
bool inReplay = gameWrapper->IsInReplay();
bool inOnline = gameWrapper->IsOnlineGame();
```

## Common Patterns

### Send Match Stats to Web Server
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    [this](void* params) {
        auto gw = GetGameWrapper();
        auto server = gw->GetCurrentGameState();
        
        if (!server) return;
        
        // Gather match data
        auto teams = server->GetTeams();
        std::string blueScore = std::to_string(teams.Get(0)->GetScore());
        std::string orangeScore = std::to_string(teams.Get(1)->GetScore());
        
        // Send to server
        auto httpWrapper = gw->GetHttpWrapper();
        if (httpWrapper) {
            std::string postData = "blue=" + blueScore + "&orange=" + orangeScore;
            
            httpWrapper->Post(
                "https://yourserver.com/match-end",
                postData,
                [this](std::string response) {
                    std::cout << "Match stats posted\n";
                }
            );
        }
    }
);
```

### Fetch Configuration from Web
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto httpWrapper = gw->GetHttpWrapper();
    if (!httpWrapper) return;
    
    // Fetch config
    httpWrapper->Get(
        "https://yourserver.com/config.json",
        [this](std::string response) {
            // Parse JSON response
            std::cout << "Config: " << response << "\n";
        }
    );
}, 30.0f);  // Every 30 seconds
```

### Detect Online vs Offline
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    bool isOnline = gw->IsOnlineGame();
    
    if (isOnline) {
        std::cout << "Currently in online match\n";
        // Handle online-specific logic
    } else {
        std::cout << "Currently offline (freeplay/training/replay)\n";
        // Handle offline-specific logic
    }
}, 1.0f);
```

### Retry HTTP with Exponential Backoff
```cpp
class HttpManager {
    GameWrapper gameWrapper;
    int retryCount = 0;
    const int MAX_RETRIES = 3;
    const float INITIAL_DELAY = 1.0f;
    
public:
    void FetchWithRetry(std::string url) {
        auto httpWrapper = gameWrapper->GetHttpWrapper();
        if (!httpWrapper) return;
        
        httpWrapper->Get(url, 
            [this, url](std::string response) {
                if (response.empty()) {
                    // Request failed
                    if (retryCount < MAX_RETRIES) {
                        retryCount++;
                        float delay = INITIAL_DELAY * (1 << retryCount);  // Exponential
                        
                        gameWrapper->SetTimeout([this, url](GameWrapper gw) {
                            FetchWithRetry(url);
                        }, delay);
                    }
                } else {
                    // Success
                    retryCount = 0;
                    std::cout << "Fetched: " << response << "\n";
                }
            }
        );
    }
};
```

## Network Information

### Game Network Data
```cpp
auto server = gameWrapper->GetCurrentGameState();
if (!server) return;

// Network quality data (via PriWrapper)
auto players = server->GetPRIs();
for (int i = 0; i < players.Count(); i++) {
    auto player = players.Get(i);
    if (!player) continue;
    
    // Get network quality (0-255, lower is better)
    unsigned char netQuality = player->GetReplicatedWorstNetQualityBeyondLatency();
    
    // Get public IP (if available)
    UnrealStringWrapper publicIP = player->GetPublicIP();
    
    std::cout << player->GetPlayerName() << " net quality: " << (int)netQuality << "\n";
}
```

## HTTP Best Practices

### Handle Network Timeouts
```cpp
auto httpWrapper = gameWrapper->GetHttpWrapper();

// Implement timeout logic
std::chrono::steady_clock::time_point requestStart;

httpWrapper->Get("https://api.example.com/data",
    [this](std::string response) {
        auto requestEnd = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            requestEnd - requestStart
        );
        
        if (response.empty()) {
            std::cout << "Request timeout after " << duration.count() << "ms\n";
        } else {
            std::cout << "Response in " << duration.count() << "ms\n";
        }
    }
);

requestStart = std::chrono::steady_clock::now();
```

### Safe JSON Parsing
```cpp
httpWrapper->Get("https://api.example.com/data.json",
    [this](std::string response) {
        // Always validate response
        if (response.empty()) {
            std::cout << "Empty response\n";
            return;
        }
        
        // Check if valid JSON
        if (response[0] != '{' && response[0] != '[') {
            std::cout << "Invalid JSON\n";
            return;
        }
        
        // Parse (use JSON library)
        // nlohmann::json j = nlohmann::json::parse(response);
    }
);
```

## Thread Safety Notes

- **HTTP callbacks** run on HTTP thread; marshal to game thread to modify game state
- **GameWrapper** is thread-safe to query state
- **SetTimeout** must be called from game thread

```cpp
// SAFE - HTTP callback queues work for later
httpWrapper->Get(url, [this](std::string response) {
    // This runs on HTTP thread
    if (response.empty()) return;
    
    // Queue work for game thread
    gameWrapper->SetTimeout([this, response](GameWrapper gw) {
        // Now on game thread, safe to modify state
        auto server = gw->GetCurrentGameState();
    }, 0.0f);
});

// UNSAFE - Modifying game state from HTTP callback
httpWrapper->Get(url, [this](std::string response) {
    auto server = gameWrapper->GetCurrentGameState();
    // Can crash! gameWrapper state access from HTTP thread
});
```

## Performance Considerations

- **HTTP requests are async** - Don't block game thread
- **Limit request frequency** - Don't hammer server every frame
- **Batch requests** - Combine multiple requests into one
- **Cache responses** - Avoid redundant requests

```cpp
// GOOD - Batched, infrequent requests
gameWrapper->SetTimeout([this](GameWrapper gw) {
    // One request per 10 seconds
    FetchAllData();
}, 10.0f);

// BAD - Request every frame
gameWrapper->SetTimeout([this](GameWrapper gw) {
    // 60 requests per second! Don't do this
    FetchOneData();
}, 0.016f);
```

## SuiteSpot-Specific Usage

While SuiteSpot is for local map loading, you could extend it for web features:

```cpp
// Example: Notify server when map is loaded
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventBegin",
    [this](void* params) {
        auto gw = GetGameWrapper();
        auto httpWrapper = gw->GetHttpWrapper();
        
        if (httpWrapper) {
            httpWrapper->Post(
                "https://yourserver.com/match-started",
                "map=utopia",
                [](std::string response) {}
            );
        }
    }
);
```

## Important Notes for AI Agents

1. **HTTP callbacks are async** - Don't block; responses come later
2. **Empty response means error** - Network failure, timeout, or server error
3. **POST data is form-encoded** - Key=value&other=data format
4. **IsOnlineGame() is definitive** - True means server-authoritative match
5. **Network quality is byte value** - 0-255; lower is better (lower latency)
6. **Public IP might be empty** - Privacy reasons or network configuration
7. **Marshall callbacks to game thread** - HTTP callbacks run on separate thread
8. **Requests are fire-and-forget** - No built-in retry; you must implement
9. **URL must be HTTPS** - HTTP not supported
10. **Keep token alive** - Destroy HttpWrapper ends pending requests

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/http/ and networking utilities
