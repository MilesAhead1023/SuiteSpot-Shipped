# TeamWrapper API

## Overview

A `TeamWrapper` represents one of the two teams in a match (Blue or Orange). It holds information about the team's score, name, and the players on the team. It inherits from `TeamInfoWrapper`, which in turn inherits from `ActorWrapper`.

### How to Get a `TeamWrapper`

-   **From a Server:** `serverWrapper->GetTeams()` returns an `ArrayWrapper<TeamWrapper>`. A standard soccar match will have two teams in this array.
-   **From a Player:** `priWrapper->GetTeam()` returns the `TeamWrapper` for the team a specific player is on.

## API Description

### Header
`bakkesmod/wrappers/GameObject/TeamWrapper.h`
`bakkesmod/wrappers/GameObject/TeamInfoWrapper.h`

### Inheritance
`TeamWrapper` -> `TeamInfoWrapper` -> `ActorWrapper` -> `ObjectWrapper`

### Key Team Information

These functions are fundamental for identifying the team and its score. Most of these are inherited from `TeamInfoWrapper`.

-   `int GetTeamIndex()`: Returns the team's index. `0` is typically Blue, and `1` is Orange. This is the most reliable way to identify the team.
-   `int GetScore()`: Returns the team's current score.
-   `void SetScore(int newScore)`: Sets the team's score.
-   `UnrealStringWrapper GetTeamName()`: Returns the team's default name (e.g., "Blue", "Orange").
-   `UnrealStringWrapper GetCustomTeamName()`: Returns the custom name for the team, if set (e.g., for a tournament or club match).

### Team Roster

-   `ArrayWrapper<PriWrapper> GetMembers()`: This is a crucial function that returns an array of `PriWrapper` objects for all players currently on this team.
-   `int GetNumHumans()`: Returns the number of human players on the team.
-   `int GetNumBots()`: Returns the number of AI bots on the team.
-   `int GetSize()`: Returns the total number of players on the team.

### Team Visuals

-   `LinearColor GetPrimaryColor()`: Returns the primary color of the team.
-   `LinearColor GetSecondaryColor()`: Returns the secondary color of the team, used for accents.

### Example: Finding the Blue and Orange teams

```cpp
void MyPlugin::GetTeamScores()
{
    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) { return; }

    TeamWrapper blueTeam, orangeTeam;
    ArrayWrapper<TeamWrapper> teams = server.GetTeams();

    if (teams.Count() != 2)
    {
        LOG("Could not find two teams.");
        return;
    }
    
    // Identify teams by their index
    if (teams.Get(0).GetTeamIndex() == 0)
    {
        blueTeam = teams.Get(0);
        orangeTeam = teams.Get(1);
    }
    else
    {
        blueTeam = teams.Get(1);
        orangeTeam = teams.Get(0);
    }

    if (blueTeam && orangeTeam)
    {
        int blueScore = blueTeam.GetScore();
        int orangeScore = orangeTeam.GetScore();
        LOG("Current score: Blue {} - {} Orange", blueScore, orangeScore);
    }
}
```
