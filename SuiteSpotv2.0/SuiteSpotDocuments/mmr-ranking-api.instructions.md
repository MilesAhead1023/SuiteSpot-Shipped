# MMRWrapper Complete API Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

MMRWrapper provides access to Matchmaking Rating (MMR) data in Rocket League. It allows querying player skill ratings, ranks, MMR values, and subscribing to MMR update notifications.

## Overview

MMRWrapper gives you access to:
- Player skill ratings and ranks
- MMR (Matchmaking Rating) values
- Playlist information
- Syncing/synced status
- MMR change notifications
- Ranking calculations

## Constructor/Initialization

Get via GameWrapper:

```cpp
auto gameWrapper = GetGameWrapper();
auto mmrWrapper = gameWrapper->GetMMRWrapper();
if (!mmrWrapper) return;

// Query MMR data
```

## Query MMR Data

### Player Skill Information
```cpp
// Get player skill rating by UniqueID
UniqueIDWrapper playerID = ...;  // Get from PriWrapper
int playlistID = 10;  // Competitive 1v1
SkillRating skillRating = mmrWrapper->GetPlayerSkillRating(playerID, playlistID);

// Get player rank
SkillRank skillRank = mmrWrapper->GetPlayerRank(playerID, playlistID);

// Get raw MMR value
float mmrValue = mmrWrapper->GetPlayerMMR(playerID, playlistID);

// Alternative: Query by SteamID
SteamID steamID = ...;
SkillRating skillRatingSteam = mmrWrapper->GetPlayerSkillRating(steamID, playlistID);
SkillRank skillRankSteam = mmrWrapper->GetPlayerRank(steamID, playlistID);
float mmrValueSteam = mmrWrapper->GetPlayerMMR(steamID, playlistID);
```

### Check Syncing Status
```cpp
// Check if player MMR is currently syncing (loading from server)
bool isSyncing = mmrWrapper->IsSyncing(playerID);

// Check if player MMR for specific playlist is synced
bool isSynced = mmrWrapper->IsSynced(playerID, playlistID);

// Alternative with SteamID
bool isSyncingSteam = mmrWrapper->IsSyncing(steamID);
bool isSyncedSteam = mmrWrapper->IsSynced(steamID, playlistID);
```

### Playlist Information
```cpp
// Check if playlist is ranked
int playlistID = 10;
bool isRanked = mmrWrapper->IsRanked(playlistID);

// Get current active playlist
int currentPlaylist = mmrWrapper->GetCurrentPlaylist();
```

## MMR Calculations

```cpp
// Calculate MMR from skill rating
SkillRating sr = ...;  // Get or create
bool disregardPlacements = false;
float calculatedMMR = mmrWrapper->CalculateMMR(sr, disregardPlacements);

// This converts SkillRating struct to float MMR value
```

## MMR Notifications

### Subscribe to MMR Changes
```cpp
// Register for MMR update notifications
std::unique_ptr<MMRNotifierToken> token = 
    mmrWrapper->RegisterMMRNotifier(
        [this](UniqueIDWrapper playerID) {
            // Called when this player's MMR updates
            std::cout << "MMR updated for player\n";
            
            // Re-query MMR to get new value
            auto mmr = GetGameWrapper()->GetMMRWrapper();
            if (mmr) {
                float newMMR = mmr->GetPlayerMMR(playerID, 10);
                std::cout << "New MMR: " << newMMR << "\n";
            }
        }
    );

// Token automatically unsubscribes when destroyed
// Keep token alive as long as you want notifications
```

## Common Patterns

### Get All Players' MMR
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto mmrWrapper = gw->GetMMRWrapper();
    if (!mmrWrapper) return;
    
    auto players = server->GetPRIs();
    int playlistID = 10;  // Competitive 1v1
    
    for (int i = 0; i < players.Count(); i++) {
        auto player = players.Get(i);
        if (!player) continue;
        
        auto playerID = player->GetUniqueID();
        float mmr = mmrWrapper->GetPlayerMMR(playerID, playlistID);
        
        if (mmrWrapper->IsSynced(playerID, playlistID)) {
            std::cout << player->GetPlayerName() << ": " << mmr << " MMR\n";
        } else {
            std::cout << player->GetPlayerName() << ": (syncing...)\n";
        }
    }
}, 5.0f);
```

### Monitor Team Average MMR
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto mmrWrapper = gw->GetMMRWrapper();
    if (!mmrWrapper) return;
    
    auto teams = server->GetTeams();
    int playlistID = 10;
    
    for (int t = 0; t < teams.Count(); t++) {
        auto team = teams.Get(t);
        if (!team) continue;
        
        auto members = team->GetMembers();
        float totalMMR = 0;
        int count = 0;
        
        for (int p = 0; p < members.Count(); p++) {
            auto player = members.Get(p);
            if (!player) continue;
            
            float playerMMR = mmrWrapper->GetPlayerMMR(player->GetUniqueID(), playlistID);
            totalMMR += playerMMR;
            count++;
        }
        
        float avgMMR = (count > 0) ? (totalMMR / count) : 0;
        std::string teamName = (t == 0) ? "Blue" : "Orange";
        
        std::cout << teamName << " average MMR: " << avgMMR << "\n";
    }
}, 10.0f);
```

### Rank Distribution
```cpp
gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto mmrWrapper = gw->GetMMRWrapper();
    if (!mmrWrapper) return;
    
    std::map<std::string, int> rankCounts;
    auto players = server->GetPRIs();
    int playlistID = 10;
    
    for (int i = 0; i < players.Count(); i++) {
        auto player = players.Get(i);
        if (!player) continue;
        
        auto rank = mmrWrapper->GetPlayerRank(player->GetUniqueID(), playlistID);
        std::string rankName = GetRankName(rank);  // You'd define this
        rankCounts[rankName]++;
    }
    
    for (const auto& entry : rankCounts) {
        std::cout << entry.first << ": " << entry.second << "\n";
    }
}, 10.0f);
```

### Wait for MMR to Sync
```cpp
std::unique_ptr<MMRNotifierToken> mmrToken;

gameWrapper->SetTimeout([this](GameWrapper gw) {
    auto mmrWrapper = gw->GetMMRWrapper();
    if (!mmrWrapper) return;
    
    auto server = gw->GetCurrentGameState();
    if (!server) return;
    
    auto players = server->GetPRIs();
    if (players.Count() < 1) return;
    
    auto player = players.Get(0);
    if (!player) return;
    
    auto playerID = player->GetUniqueID();
    int playlistID = 10;
    
    // Check if synced
    if (mmrWrapper->IsSynced(playerID, playlistID)) {
        // MMR is ready
        float mmr = mmrWrapper->GetPlayerMMR(playerID, playlistID);
        std::cout << "MMR: " << mmr << "\n";
    } else {
        // Register for notification when synced
        if (!mmrToken) {
            mmrToken = mmrWrapper->RegisterMMRNotifier(
                [this, playerID, playlistID](UniqueIDWrapper id) {
                    if (id.GetIdString() == playerID.GetIdString()) {
                        std::cout << "MMR synced!\n";
                    }
                }
            );
        }
    }
}, 1.0f);
```

## Playlist ID Reference

Common playlist IDs:
```
1: 1v1 Duel
2: 2v2 Doubles
3: 3v3 Standard
10: Competitive 1v1
11: Competitive 2v2
12: Competitive 3v3
13: Competitive 4v4
14-29: Various LTM playlists
30+: Private playlists
```

## Thread Safety Notes

- **Safe to call from**: Game thread
- **Not safe to call from**: Other threads
- **Wrapper lifetime**: Valid as long as connected to server
- **Always null-check**: `if (!mmrWrapper) return;`

## Performance Considerations

- **MMR queries** are O(1); fast
- **Syncing status checks** are fast
- **Notifications** are event-driven; only fire on changes
- **Safe to query all players** every frame

## SuiteSpot-Specific Usage

While SuiteSpot doesn't use MMR, you could extend it:

```cpp
// Example: Log player ratings on match start
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventBegin",
    [this](void* params) {
        auto gw = GetGameWrapper();
        auto mmrWrapper = gw->GetMMRWrapper();
        auto server = gw->GetCurrentGameState();
        
        if (mmrWrapper && server) {
            // Log ratings
        }
    }
);
```

## Important Notes for AI Agents

1. **MMR values vary by playlist** - Each playlist has separate MMR
2. **Syncing is asynchronous** - Check IsSynced() before relying on data
3. **UniqueIDWrapper vs SteamID** - Both work, UniqueIDWrapper is preferred
4. **Skill rating is struct** - Contains division, tier, and other data
5. **Register before querying** - Notifications help track MMR changes
6. **Token lifetime controls subscription** - Destroy token to unsubscribe
7. **Current playlist changes** - GetCurrentPlaylist() reflects current selection
8. **Ranked check per playlist** - Only some playlists are ranked
9. **MMR calculation uses skill rating** - CalculateMMR() converts struct to float
10. **Notifications are per-player** - Not per-playlist; filter playlist yourself

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK include/bakkesmod/wrappers/MMRWrapper.h
