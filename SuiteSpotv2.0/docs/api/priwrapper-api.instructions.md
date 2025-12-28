# PriWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

PriWrapper (PlayerReplicationInfo) represents a player in Rocket League. It contains all player metadata, statistics, loadout information, and player-specific settings. This is where you access player names, teams, scores, and inventory.

## Overview

PriWrapper gives you access to:
- Player identity (name, unique ID, player index)
- Team association
- Match statistics (goals, assists, saves, shots, demolitions)
- Vehicle/car association
- Camera settings and preferences
- Club information
- Loadout and cosmetics data
- MVP status and achievements
- Network quality information

## Constructor/Initialization

Get instances via ServerWrapper or from cars:

```cpp
auto gameWrapper = GetGameWrapper();
auto server = gameWrapper->GetCurrentGameState();
if (!server) return;

// Get all players
auto players = server->GetPRIs();
for (int i = 0; i < players.Count(); i++) {
    auto player = players.Get(i);
    if (!player) continue;
    // Process player
}

// Get player from a car
auto cars = server->GetCars();
if (cars.Count() > 0) {
    auto car = cars.Get(0);
    if (car) {
        auto ownerPRI = car->GetOwner();
        if (ownerPRI) {
            // Process player
        }
    }
}

// Get specific team's players
auto teams = server->GetTeams();
if (teams.Count() > 0) {
    auto team = teams.Get(0);
    if (team) {
        auto members = team->GetMembers();
        for (int i = 0; i < members.Count(); i++) {
            auto player = members.Get(i);
            // Process player
        }
    }
}
```

## Player Identity Methods

### Basic Information
```cpp
// Get player name
std::string playerName = player->GetPlayerName();

// Get unique player ID (UniqueIDWrapper)
UniqueIDWrapper uniqueID = player->GetUniqueID();

// Get player index (0-7 typically)
int playerIndex = player->GetPlayerIndex();

// Get team number (0 = blue, 1 = orange)
int teamNum = player->GetTeamNum();

// Check if player is ready
bool isReady = player->GetbReady();

// Check if player is spectating
bool isSpectating = player->IsSpectator();

// Check if player is valid
bool isValidPlayer = player->IsPlayer();
```

## Match Statistics Methods

All statistics are accumulated during the current match:

```cpp
// Scoring
int score = player->GetScore();

// Goals scored
int goals = player->GetMatchGoals();

// Assists
int assists = player->GetMatchAssists();

// Saves
int saves = player->GetMatchSaves();

// Shots on goal
int shots = player->GetMatchShots();

// Demolitions dealt
int demolitions = player->GetDemolishes();

// Demolitions received
int timesDemo = player->GetTimesDemo();

// Bumps dealt
int bumps = player->GetBumps();

// Ball touches
int touches = player->GetBallTouches();

// Boost pickups
int boostPickups = player->GetBoostPickups();

// MVP points (in exhibition)
int mvpPoints = player->GetMatchMVP();

// Total gameplay time
float timePlayedSeconds = player->GetTotalGameTimePlayed();
```

## Vehicle & Loadout Methods

```cpp
// Get currently assigned car
CarWrapper playerCar = player->GetCar();
if (playerCar) {
    // Check car properties
}

// Get loadout information
int bodyLoadout = player->GetLoadoutBody();  // Car body ID

// Get current loadout (includes all slots)
LoadoutWrapper loadout = player->GetLoadout();
if (loadout) {
    // Read loadout details
}

// Get current loadout name (custom)
UnrealStringWrapper loadoutName = player->GetLoadoutName();
std::string name = loadoutName.ToString();
```

## Camera & Input Preferences

```cpp
// Get camera settings
ProfileCameraSettings cameraSettings = player->GetCameraSettings();
// Includes: FOV, height, distance, angle, stiffness, swivel speed, invert options

// Get camera pitch/yaw (0-255 scale)
unsigned char pitch = player->GetCameraPitch();
unsigned char yaw = player->GetCameraYaw();

// Get steering sensitivity
float steerSensitivity = player->GetSteeringSensitivity();

// Get air control sensitivity
float airControlSensitivity = player->GetAirControlSensitivity();

// Get dodge input threshold
float dodgeThreshold = player->GetDodgeInputThreshold();

// Check if using freecam
bool usingFreecam = player->GetbUsingFreecam();

// Check if using behind-view camera
bool usingBehindView = player->GetbUsingBehindView();

// Check if in split-screen
bool inSplitScreen = player->GetbIsInSplitScreen();
```

## Club & Social Information

```cpp
// Get club ID
unsigned long long clubID = player->GetClubID();

// Get club details
ClubDetailsWrapper clubDetails = player->GetClubDetails();
if (clubDetails) {
    // Read club name, tag, etc.
}

// Check if player is match admin
bool isMatchAdmin = player->GetbIsMatchAdmin();

// Check if won MVP this round
bool wonMVP = player->GetbNotifyWonMVP();

// Get respawn time remaining (in milliseconds)
int respawnTimeMs = player->GetRespawnTimeRemaining();

// Get team index for paint/cosmetics
int teamIndex = player->GetLoadoutTeamIndex();
```

## Network & Status Methods

```cpp
// Get network quality (0-255, lower is better)
unsigned char netQuality = player->GetReplicatedWorstNetQualityBeyondLatency();

// Check if player is a bot
bool isBot = player->IsBot();

// Get public IP (may be empty for privacy)
UnrealStringWrapper publicIP = player->GetPublicIP();
std::string ipString = publicIP.ToString();

// Check if ready to start
bool ready = player->GetbReady();

// Check if player is busy
bool busy = player->GetbBusy();

// Check if player is waiting
bool waitingToStart = player->GetbWaitingPlayer();

// Get match join time
int waitingStartTime = player->GetWaitingStartTime();
```

## Advanced Player Management Methods

### State Changes
```cpp
// Set player ready state (server-side only)
player->SetReady(true);

// Change player team (server-side)
player->ServerChangeTeam(0);  // 0 = blue, 1 = orange

// Start spectating (server-side)
player->ServerSpectate();

// Vote to forfeit (server-side)
player->ServerVoteToForfeit();

// Set player car preferences
player->SetUserCarPreferences(
    0.65f,      // New dodge threshold
    1.0f,       // New steering sensitivity
    0.85f       // New air control sensitivity
);
```

### Loadout Management
```cpp
// Load current active loadout
player->UpdateFromLoadout();

// Validate loadout after changes
bool valid = player->AreLoadoutsSet();

// Validate specific aspects
bool slotsValid = player->ValidateLoadoutSlots();
bool paintsValid = player->ValidateLoadoutTeamPaints();

// Handle loadout selection
player->HandleLoadoutSelected(player);

// Update on cosmetics change
player->OnClubColorsChanged();
```

### Statistics Management
```cpp
// Record a goal
player->ScorePoint2(1);

// Remove points
player->RemovePoints(5);

// Reset all scores
player->ResetScore();

// Commit stats to backend (if ranked)
player->CommitStats();

// Record time-restricted stat event
if (player->CanAwardTimeRestrictedStatEvent()) {
    player->RecordTimeRestrictedStatEvent();
}

// Reset time-restricted event timer
player->ResetTimeRestrictedStatEvent();
```

### Achievement & Progress
```cpp
// Get match XP earned
int xpEarned = player->GetMatchXP();

// Notify achievement progress (client-side)
// Note: Limited use in plugins, mainly for game code

// Update player title from loadout
player->UpdateTitle2();

// Get player title/rank
std::string playerTitle = player->GetTitle().ToString();
```

## Player Role Checks

```cpp
// Is this a regular player
bool isPlayer = player->IsPlayer();

// Is this a spectator
bool isSpectator = player->IsSpectator();

// Is this a bot (AI)
bool isBot = player->IsBot();

// Is this the local player (your client)
bool isLocalPlayer = player->IsLocalPlayerPRI();

// Is this the client player (current user)
bool isClientPlayer = player->IsClientPlayerPRI();

// Is player editor/admin
bool isEditor = player->IsEditor();
```

## Event Hooks (PRI-Related)

```cpp
// Hook player score update
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.ScorePoint",
    [](PriWrapper player, void* params) {
        std::string name = player->GetPlayerName();
        int score = player->GetScore();
        std::cout << name << " score: " << score << "\n";
    }
);

// Hook player team change
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.OnTeamChanged",
    [](PriWrapper player, void* params) {
        std::string name = player->GetPlayerName();
        int team = player->GetTeamNum();
        std::cout << name << " changed to team " << team << "\n";
    }
);

// Hook car assignment
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.SetCar",
    [](PriWrapper player, void* params) {
        auto car = *(CarWrapper*)params;
        std::string name = player->GetPlayerName();
        std::cout << name << " assigned car\n";
    }
);

// Hook respawn
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.OnRespawnTimeRemainingChanged",
    [](PriWrapper player, void* params) {
        int respawnTime = player->GetRespawnTimeRemaining();
        std::cout << "Respawn time: " << respawnTime << "ms\n";
    }
);

// Hook player ready change
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.SetReady",
    [](PriWrapper player, void* params) {
        bool ready = player->GetbReady();
        std::string name = player->GetPlayerName();
        std::cout << name << (ready ? " is ready" : " is not ready") << "\n";
    }
);
```

## Common Patterns

### Get All Players with Names and Teams
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto players = server->GetPRIs();
    for (int i = 0; i < players.Count(); i++) {
        auto player = players.Get(i);
        if (!player) continue;
        
        std::string name = player->GetPlayerName();
        int teamNum = player->GetTeamNum();
        int goals = player->GetMatchGoals();
        
        std::cout << (teamNum == 0 ? "BLUE: " : "ORANGE: ") 
                  << name << " - Goals: " << goals << "\n";
    }
}, 1.0f);
```

### Track Player Statistics
```cpp
std::map<std::string, int> previousGoals;

gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto players = server->GetPRIs();
    for (int i = 0; i < players.Count(); i++) {
        auto player = players.Get(i);
        if (!player) continue;
        
        std::string name = player->GetPlayerName();
        int currentGoals = player->GetMatchGoals();
        
        if (previousGoals[name] < currentGoals) {
            std::cout << name << " scored a goal!\n";
            previousGoals[name] = currentGoals;
        }
    }
}, 0.5f);
```

### Find Player by Name
```cpp
PriWrapper FindPlayerByName(std::string targetName) {
    auto gameWrapper = GetGameWrapper();
    auto server = gameWrapper->GetCurrentGameState();
    if (!server) return PriWrapper(nullptr, nullptr);
    
    auto players = server->GetPRIs();
    for (int i = 0; i < players.Count(); i++) {
        auto player = players.Get(i);
        if (!player) continue;
        
        if (player->GetPlayerName() == targetName) {
            return player;
        }
    }
    return PriWrapper(nullptr, nullptr);
}
```

### Get Team Totals
```cpp
struct TeamStats {
    int totalGoals = 0;
    int totalAssists = 0;
    int playerCount = 0;
};

gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    std::map<int, TeamStats> teamStats;  // 0 = blue, 1 = orange
    
    auto players = server->GetPRIs();
    for (int i = 0; i < players.Count(); i++) {
        auto player = players.Get(i);
        if (!player) continue;
        
        int team = player->GetTeamNum();
        teamStats[team].totalGoals += player->GetMatchGoals();
        teamStats[team].totalAssists += player->GetMatchAssists();
        teamStats[team].playerCount++;
    }
    
    std::cout << "Blue: " << teamStats[0].totalGoals << " goals, " 
              << teamStats[0].playerCount << " players\n";
    std::cout << "Orange: " << teamStats[1].totalGoals << " goals, " 
              << teamStats[1].playerCount << " players\n";
}, 5.0f);
```

## Thread Safety Notes

- **Safe to call from**: Game thread (inside hooks, SetTimeout callbacks)
- **Not safe to call from**: Other threads without synchronization
- **Wrapper lifetime**: Valid only during match; becomes null when player disconnects
- **Always null-check**: `if (!player) continue;` in loops

```cpp
// SAFE - Re-fetch each frame
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    auto players = server->GetPRIs();
    for (int i = 0; i < players.Count(); i++) {
        auto player = players.Get(i);
        if (!player) continue;
        auto name = player->GetPlayerName();
    }
}, 0.016f);

// UNSAFE - Caching player across frame
std::vector<PriWrapper> cachedPlayers;
// ... later frame ...
for (auto& player : cachedPlayers) {
    if (player) { }  // player might be invalid!
}
```

## Performance Considerations

- **GetPlayerName()** does string conversion; cache if using multiple times per frame
- **GetPRIs() in loop** is efficient; PRIs don't change frequently
- **Getting loadout data** is relatively expensive; cache if needed in tight loops
- **Statistics queries** are O(1) field accesses; no performance concern

```cpp
// INEFFICIENT
for (int i = 0; i < players.Count(); i++) {
    auto player = players.Get(i);
    std::string name1 = player->GetPlayerName();
    std::string name2 = player->GetPlayerName();
    std::string name3 = player->GetPlayerName();
    // Three conversions per loop!
}

// EFFICIENT
for (int i = 0; i < players.Count(); i++) {
    auto player = players.Get(i);
    std::string name = player->GetPlayerName();
    // Use 'name' multiple times
}
```

## SuiteSpot-Specific Usage

While SuiteSpot focuses on map loading, you could extend it to:

```cpp
// Log player count when queue completes
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    [this](void* params) {
        auto server = GetGameWrapper()->GetCurrentGameState();
        if (server) {
            int playerCount = server->GetPRIs().Count();
            // Log player count with map info
        }
    }
);
```

## Important Notes for AI Agents

1. **Null-check in loops** - Players can disconnect; check before using
2. **PlayerName is string conversion** - Expensive; cache if used multiple times
3. **GetCar() can be null** - Player exists but may not have a car yet
4. **Statistics are match-local** - They reset at match start, not persistent
5. **Team changes can happen** - Re-fetch team number if important
6. **UniqueID is persistent** - Use for tracking players across matches
7. **Bot names are generated** - Can check `IsBot()` to identify AI players
8. **Camera settings are preferences** - May not reflect actual camera state
9. **Loadout validation is important** - Always check before assuming data is complete
10. **Score points are immediate** - Changes reflect in GetScore() immediately after

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/GameObject/PriWrapper.h
