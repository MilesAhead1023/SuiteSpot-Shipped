# ServerWrapper API

The `ServerWrapper` is used to control and interact with the game state during a match. You can get the current `ServerWrapper` instance by calling `gameWrapper->GetCurrentGameState()`.

## Accessing Game Objects

-   `BallWrapper GetBall()`: Returns the `BallWrapper` for the ball in the match.
-   `ArrayWrapper<CarWrapper> GetCars()`: Returns an array of all cars in the match.
-   `ArrayWrapper<PriWrapper> GetPRIs()`: Returns an array of all player replication infos (PRIs) in the match. PRIs represent the players in the match and can be used to get their name, score, and other information.

## Spawning Objects

-   `void SpawnCar(int carBody, std::string name)`: Spawns a new car with the specified body and name.
-   `void SpawnBot(int carBody, std::string name)`: Spawns a new bot with the specified body and name.
-   `BallWrapper SpawnBall(const Vector position, bool wake, bool spawnCannon)`: Spawns a new ball at the specified position.

## Game State Control

-   `float GetGameSpeed()`: Returns the current game speed.
-   `void SetGameSpeed(float newSpeed)`: Sets the game speed. `1.0` is normal speed.
-   `float GetSecondsElapsed()`: Returns the number of seconds that have elapsed in the match.
-   `void SetSecondsElapsed(float newTime)`: Sets the elapsed time.
-   `int GetSecondsRemaining()`: Returns the number of seconds remaining in the match.
-   `void SetSecondsRemaining(int newTime)`: Sets the remaining time.
-   `void DisableGoalReset()`: Disables the goal reset after a goal is scored.
-   `void EnableGoalReset()`: Enables the goal reset.

## Ball Control

-   `Vector GenerateShot(Vector startPos, Vector destination, float speed)`: Generates a shot from a start position to a destination with a given speed. Returns the direction vector of the shot.

## Goal Information

-   `Vector GetGoalLocation(int goalNumber)`: Returns the location of the specified goal. `0` for the blue goal, `1` for the orange goal.
-   `Vector GetGoalExtent(int goalNumber)`: Returns the extent (size) of the specified goal.

## Other Useful Functions

-   `void Rematch()`: Starts a rematch.
-   `bool IsInOvertime()`: Returns `true` if the game is in overtime.
-   `void StartOvertime()`: Forces the game into overtime.
-   `void EndRound()`: Ends the current round.
-   `void StartRound()`: Starts a new round.