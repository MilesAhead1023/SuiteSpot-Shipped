# PriWrapper (PlayerReplicationInfo) API

## Overview

A `PriWrapper` represents a single player within a match. "PRI" stands for `PlayerReplicationInfo`. This wrapper is the primary way to access a player's stats, state, and other identifying information. It inherits from `ActorWrapper`.

### How to Get a `PriWrapper`

-   **For the local player:** `gameWrapper->GetPlayerController().GetPRI()`
-   **For all players in a match:** `serverWrapper->GetPRIs()` (returns an `ArrayWrapper<PriWrapper>`)

## API Description

### Header
`bakkesmod/wrappers/GameObject/PriWrapper.h`

### Inheritance
`PriWrapper` -> `PriXWrapper` -> ... -> `ActorWrapper` -> `ObjectWrapper`

### Key Player Information

-   `UnrealStringWrapper GetPlayerName()`: Returns the player's current name.
-   `UniqueIDWrapper GetUniqueIdWrapper()`: Returns the player's unique platform ID (`SteamID`, `EpicID`, etc.). This is essential for tracking players and fetching their MMR.
-   `bool IsLocalPlayerPRI()`: Returns `true` if this PRI belongs to the local user.
-   `bool IsSpectator()`: Returns `true` if the player is currently spectating.
-   `bool IsBot()`: Returns `true` if the PRI represents an AI bot.

### Match Statistics

`PriWrapper` provides direct access to the scoreboard and other match statistics for the player.

-   `int GetMatchScore()`: Gets the player's current score.
-   `int GetMatchGoals()`: Gets the number of goals the player has scored.
-   `int GetMatchAssists()`: Gets the number of assists.
-   `int GetMatchSaves()`: Gets the number of saves.
-   `int GetMatchShots()`: Gets the number of shots.
-   `int GetMatchDemolishes()`: Gets the number of demolitions.

### Player and Team Associations

-   `CarWrapper GetCar()`: Returns the `CarWrapper` currently being used by the player. Can be null if the player is not in a car (e.g., spectating or between spawns).
-   `TeamWrapper GetTeam()`: Returns the `TeamWrapper` for the team the player is on. Can be null.

### Player Settings

-   `ProfileCameraSettings GetCameraSettings()`: Returns a struct containing the player's camera settings (FOV, height, distance, etc.).
-   `float GetSteeringSensitivity()`: Gets the player's steering sensitivity.

### Example: Logging all player stats

```cpp
void MyPlugin::LogAllPlayerStats()
{
    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) { return; }

    ArrayWrapper<PriWrapper> pris = server.GetPRIs();
    for (int i = 0; i < pris.Count(); i++)
    {
        PriWrapper pri = pris.Get(i);
        if (!pri) { continue; }

        std::string name = pri.GetPlayerName().ToString();
        int score = pri.GetMatchScore();
        int goals = pri.GetMatchGoals();
        int assists = pri.GetMatchAssists();

        LOG("{} | Score: {} | Goals: {} | Assists: {}", name, score, goals, assists);
    }
}
```
