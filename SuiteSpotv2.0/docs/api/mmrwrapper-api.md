# MMRWrapper API

The `MMRWrapper` is used to access players' Matchmaking Rank (MMR) and skill rating information. You can get an instance of this wrapper by calling `gameWrapper->GetMMRWrapper()`.

MMR data is not always available instantly. BakkesMod syncs this data in the background. You should always check if the data is synced before trying to access it.

## Checking Sync Status

-   `bool IsSyncing(UniqueIDWrapper playerID)`: Returns `true` if BakkesMod is currently fetching MMR data for the given player.
-   `bool IsSynced(UniqueIDWrapper playerID, int playlistID)`: Returns `true` if the MMR data for a specific player in a specific playlist is available.

## Getting Player MMR and Rank

Once you have confirmed the data is synced, you can retrieve it.

-   `float GetPlayerMMR(UniqueIDWrapper playerID, int playlistID)`: Returns the raw MMR of a player as a `float`.
-   `SkillRank GetPlayerRank(UniqueIDWrapper playerID, int playlistID)`: Returns a `SkillRank` struct, which contains the player's tier, division, and number of games played in the season.
-   `SkillRating GetPlayerSkillRating(UniqueIDWrapper playerID, int playlistID)`: Returns the full `SkillRating` struct.

### `playerID`

The `UniqueIDWrapper` can be obtained from a `PriWrapper` (Player Replication Info) object. You can get a list of all `PriWrapper`s in a match from `serverWrapper->GetPRIs()`.

```cpp
// Get the local player's PRI
PriWrapper localUserPRI = gameWrapper->GetPlayerController().GetPRI();
if (localUserPRI.IsNull()) return;

// Get the UniqueID from the PRI
UniqueIDWrapper uniqueId = localUserPRI.GetUniqueIdWrapper();
```

### `playlistID`

The `playlistID` is an integer that represents a specific game mode. A list of common playlist IDs can be found through community resources or by using `gameWrapper->GetCurrentPlaylist()` to see what the current playlist is. Some common examples:

-   `11`: Ranked Doubles (2v2)
-   `13`: Ranked Standard (3v3)
-   `10`: Ranked Duel (1v1)
-   `27`: Hoops
-   `28`: Rumble

### Example

```cpp
void MyPlugin::ShowLocalPlayerMMR()
{
    // Get the MMRWrapper
    MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();

    // Get the local player's Unique ID
    PriWrapper localUserPRI = gameWrapper->GetPlayerController().GetPRI();
    if (localUserPRI.IsNull()) return;
    UniqueIDWrapper uid = localUserPRI.GetUniqueIdWrapper();

    // Get the current playlist ID
    int playlistId = mmrWrapper.GetCurrentPlaylist();

    // Check if data is synced
    if (mmrWrapper.IsSynced(uid, playlistId))
    {
        float mmr = mmrWrapper.GetPlayerMMR(uid, playlistId);
        cvarManager->log("Your MMR in this playlist is: " + std::to_string(mmr));
    }
    else
    {
        cvarManager->log("MMR is not synced yet.");
    }
}
```

## Subscribing to MMR Updates

For plugins that need to react to MMR changes in real-time, you can register a notifier.

-   `std::unique_ptr<MMRNotifierToken> RegisterMMRNotifier(std::function<void(UniqueIDWrapper id)> callback)`: Registers a function to be called whenever a player's MMR is updated.

This function returns a `std::unique_ptr<MMRNotifierToken>`. The callback will be automatically unregistered when this token is destroyed. You should store this token as a member of your plugin class.

```cpp
// In your plugin's header file (e.g., MyPlugin.h)
class MyPlugin : public BakkesMod::Plugin::BakkesModPlugin {
    // ...
    std::unique_ptr<MMRNotifierToken> MmrToken;
    // ...
};

// In your plugin's onLoad() function
void MyPlugin::onLoad()
{
    MmrToken = gameWrapper->GetMMRWrapper().RegisterMMRNotifier(
        [this](UniqueIDWrapper id) {
            // This code will run when a player's MMR is updated.
            // You can check if the ID matches the local player or another player you are tracking.
            cvarManager->log("MMR updated for player " + id.GetIdString());
        }
    );
}

// The notifier is automatically cleaned up when MyPlugin is destroyed.
```
