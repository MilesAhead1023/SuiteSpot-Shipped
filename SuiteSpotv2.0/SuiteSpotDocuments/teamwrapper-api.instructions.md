# TeamWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

TeamWrapper represents a team in Rocket League. It contains team data, members, colors, score, and team-specific events. Essential for plugins that track team information, colors, or team-level logic.

## Overview

TeamWrapper gives you access to:
- Team score and statistics
- Team members (array of PriWrappers)
- Team colors (primary, secondary, font)
- Team name (custom or default)
- Club information
- Team events and callbacks
- Forfeit status

## Constructor/Initialization

Get instances via ServerWrapper:

```cpp
auto gameWrapper = GetGameWrapper();
auto server = gameWrapper->GetCurrentGameState();
if (!server) return;

// Get all teams
auto teams = server->GetTeams();
for (int i = 0; i < teams.Count(); i++) {
    auto team = teams.Get(i);
    if (!team) continue;
    // Process team
}

// Get specific team (0 = blue, 1 = orange)
if (teams.Count() > 0) {
    auto blueTeam = teams.Get(0);
    auto orangeTeam = teams.Get(1);
}
```

## Team Score & Statistics

### Score Information
```cpp
// Get team number (0 = blue, 1 = orange)
int teamNum = team->GetTeamNum();

// Get team score
int score = team->GetScore();
team->SetScore(3);

// Update score (add points)
team->ScorePoint(1);  // Add 1 point
team->ScorePoint(2);  // Add 2 points

// Remove points
team->RemovePoints(1);

// Reset score to 0
team->ResetScore();

// Check if team forfeited
bool forfeited = team->GetbForfeit();
team->SetbForfeit(false);

// Forfeit the match
team->Forfeit2();
```

## Team Members

### Access Players on Team
```cpp
// Get all members
ArrayWrapper<PriWrapper> members = team->GetMembers();

// Iterate team members
for (int i = 0; i < members.Count(); i++) {
    auto player = members.Get(i);
    if (!player) continue;
    
    std::string name = player->GetPlayerName();
    int goals = player->GetMatchGoals();
    
    std::cout << name << ": " << goals << " goals\n";
}

// Find specific player by name
PriWrapper FindTeamMember(std::string playerName) {
    auto members = team->GetMembers();
    for (int i = 0; i < members.Count(); i++) {
        auto player = members.Get(i);
        if (!player) continue;
        
        if (player->GetPlayerName() == playerName) {
            return player;
        }
    }
    return PriWrapper(nullptr, nullptr);
}

// Get player by name from team
PriWrapper player = team->GetTeamMemberNamed("PlayerName");
if (player) {
    // Use player
}

// Get number of human players
int humanCount = team->GetNumHumans();

// Get number of bots
int botCount = team->GetNumBots();

// Check if single party (all grouped)
bool singleParty = team->IsSingleParty();

// Enable forfeit voting for all members
team->EnableAllMembersStartVoteToForfeit2();
team->EnableAllMembersStartVoteToForfeitIfNecessary();
```

## Team Colors

### Color Data
```cpp
// Get primary color (RGBA)
LinearColor primaryColor = team->GetPrimaryColor();

// Get secondary color (RGBA)
LinearColor secondaryColor = team->GetSecondaryColor();

// Get font color
LinearColor fontColor = team->GetFontColor();
team->SetFontColor(LinearColor{1, 1, 1, 1});

// Get colorblind font color
LinearColor colorblindFont = team->GetColorBlindFontColor();
team->SetColorBlindFontColor(LinearColor{1, 1, 1, 1});

// Get controller color (for UI)
UnrealColor controllerColor = team->GetTeamControllerColor();
team->SetTeamControllerColor(UnrealColor{0, 0, 255, 255});

// Get score strobe color
UnrealColor strobeColor = team->GetTeamScoreStrobeColor();
team->SetTeamScoreStrobeColor(UnrealColor{0, 0, 255, 255});

// Get default color list
StructArrayWrapper<LinearColor> defaultColors = team->GetDefaultColorList();

// Get colorblind color list
StructArrayWrapper<LinearColor> colorblindColors = team->GetColorBlindColorList();

// Get current color list (in use)
StructArrayWrapper<LinearColor> currentColors = team->GetCurrentColorList();

// Update colors
team->UpdateColors();

// Set to default colors
team->SetDefaultColors();

// Notify color changed
team->OnClubColorsChanged();
team->NotifyKismetTeamColorChanged();
```

### Team Customization
```cpp
// Get/set logo
// Logo is referenced by ID
team->SetLogo(1, false);  // Logo ID and swap colors flag

// Get/set custom team name
UnrealStringWrapper customName = team->GetCustomTeamName();
team->SetCustomTeamName("My Team");

// Get sanitized team name (safe for display)
UnrealStringWrapper sanitizedName = team->GetSanitizedTeamName();

// Handle name sanitization event
team->HandleTeamNameSanitized("Original", "Sanitized");
```

## Club Information

```cpp
// Get club ID
unsigned long long clubID = team->GetClubID();
team->SetClubID(12345);

// Handle club ID change
team->SetClubID2(12345);

// Handle club colors changed
team->OnClubColorsChanged();
```

## Team Events

### Event Hook
```cpp
// Get team game event wrapper
TeamGameEventWrapper gameEvent = team->GetGameEvent();
team->SetGameEvent(gameEvent);

// Handle game event change
team->OnGameEventSet();
team->SetGameEvent2(gameEvent);
```

## Spawn Management

```cpp
// Add temporary spawn spot
auto spawnActor = ...;  // Get ActorWrapper
team->AddTemporarySpawnSpot(spawnActor);

// Expire temporary spawn spots
team->ExpireTemporarySpawnSpots();

// Clear all temporary spawn spots
team->ClearTemporarySpawnSpots();
```

## Team Control

```cpp
// Mute other team
TeamWrapper otherTeam = ...;  // Get other team
team->MuteOtherTeam(otherTeam, true);

// Vote to mute
team->MuteOtherTeam(otherTeam, false);
```

## Common Patterns

### Get Team Totals
```cpp
struct TeamStats {
    int totalGoals = 0;
    int totalAssists = 0;
    int playerCount = 0;
    std::string teamName;
};

gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto teams = server->GetTeams();
    for (int i = 0; i < teams.Count(); i++) {
        auto team = teams.Get(i);
        if (!team) continue;
        
        TeamStats stats;
        stats.teamName = team->GetSanitizedTeamName().ToString();
        
        auto members = team->GetMembers();
        for (int j = 0; j < members.Count(); j++) {
            auto player = members.Get(j);
            if (!player) continue;
            
            stats.totalGoals += player->GetMatchGoals();
            stats.totalAssists += player->GetMatchAssists();
            stats.playerCount++;
        }
        
        std::cout << stats.teamName << ": " << stats.totalGoals 
                  << " goals, " << stats.playerCount << " players\n";
    }
}, 5.0f);
```

### Track Team Score Changes
```cpp
std::map<int, int> previousScores;  // 0=blue, 1=orange

gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto teams = server->GetTeams();
    for (int i = 0; i < teams.Count(); i++) {
        auto team = teams.Get(i);
        if (!team) continue;
        
        int teamNum = team->GetTeamNum();
        int currentScore = team->GetScore();
        
        if (previousScores[teamNum] < currentScore) {
            std::cout << (teamNum == 0 ? "Blue" : "Orange") 
                      << " scored!\n";
            previousScores[teamNum] = currentScore;
        }
    }
}, 0.5f);
```

### List All Players with Colors
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto teams = server->GetTeams();
    for (int i = 0; i < teams.Count(); i++) {
        auto team = teams.Get(i);
        if (!team) continue;
        
        std::string teamColor = (i == 0) ? "BLUE" : "ORANGE";
        auto members = team->GetMembers();
        
        std::cout << teamColor << " Team (" << members.Count() << " players):\n";
        
        for (int j = 0; j < members.Count(); j++) {
            auto player = members.Get(j);
            if (!player) continue;
            
            std::cout << "  - " << player->GetPlayerName() << "\n";
        }
    }
}, 10.0f);
```

### Hook Team Score Update
```cpp
gameWrapper->HookEventWithCaller<TeamWrapper>(
    "Function TAGame.Team_TA.OnScoreUpdated",
    [](TeamWrapper team, void* params) {
        int teamNum = team->GetTeamNum();
        int score = team->GetScore();
        std::string teamName = (teamNum == 0) ? "Blue" : "Orange";
        
        std::cout << teamName << " team score: " << score << "\n";
    }
);
```

### Hook Team Forfeit
```cpp
gameWrapper->HookEventWithCaller<TeamWrapper>(
    "Function TAGame.Team_TA.Forfeit",
    [](TeamWrapper team, void* params) {
        int teamNum = team->GetTeamNum();
        std::cout << (teamNum == 0 ? "Blue" : "Orange") << " team forfeited\n";
    }
);
```

## Thread Safety Notes

- **Safe to call from**: Game thread (inside hooks, SetTimeout callbacks)
- **Not safe to call from**: Other threads without synchronization
- **Wrapper lifetime**: Valid only during match; becomes null on match end
- **Always null-check**: `if (!team) continue;` in loops

```cpp
// SAFE
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    auto teams = server->GetTeams();
    for (int i = 0; i < teams.Count(); i++) {
        auto team = teams.Get(i);
        if (!team) continue;  // NULL CHECK
        
        auto score = team->GetScore();
    }
}, 0.5f);

// UNSAFE - Storing team
auto cachedTeam = team;
// ... later ...
cachedTeam->GetScore();  // Might be invalid!
```

## Performance Considerations

- **GetMembers()** is efficient; member list doesn't change frequently
- **Iterating members** is fast; safe to do every frame
- **GetScore()** is O(1); no performance concern
- **Color queries** are fast; no performance concern

```cpp
// EFFICIENT - Cache if iterating multiple times
auto members = team->GetMembers();
for (int i = 0; i < members.Count(); i++) {
    // First loop
}
for (int i = 0; i < members.Count(); i++) {
    // Second loop using same cached members
}

// INEFFICIENT - GetMembers() each time
for (int i = 0; i < team->GetMembers().Count(); i++) {}
for (int i = 0; i < team->GetMembers().Count(); i++) {}
```

## SuiteSpot-Specific Usage

While SuiteSpot doesn't track team data, you could extend it:

```cpp
// Example: Log final team scores on match end
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    [this](void* params) {
        auto server = GetGameWrapper()->GetCurrentGameState();
        if (server) {
            auto teams = server->GetTeams();
            for (int i = 0; i < teams.Count(); i++) {
                auto team = teams.Get(i);
                if (team) {
                    std::cout << (i == 0 ? "Blue" : "Orange") << ": " 
                              << team->GetScore() << "\n";
                }
            }
        }
    }
);
```

## Important Notes for AI Agents

1. **Null-check in loops** - Teams should exist, but always check
2. **Team count is typically 2** - Always use `.Count()` to be safe
3. **Team numbers are 0 (blue) and 1 (orange)** - Consistent across methods
4. **Members list changes dynamically** - Players can disconnect
5. **Score updates are immediate** - Visible in GetScore() right after goal
6. **Colors are LinearColor or UnrealColor** - Different formats for different purposes
7. **SetScore() overwrites score** - Use ScorePoint() to add incrementally
8. **Custom team names might be sanitized** - GetCustomTeamName() vs GetSanitizedTeamName()
9. **Forfeit is team-wide** - All members lose together
10. **Team events mirror player events** - Most updates propagate from individual players

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/GameObject/TeamWrapper.h
