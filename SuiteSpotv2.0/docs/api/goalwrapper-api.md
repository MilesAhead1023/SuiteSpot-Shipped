# GoalWrapper API

## Overview

The `GoalWrapper` represents a goal area on the field. It provides information about the goal's location, orientation, size, and which team it belongs to.

### How to Get a `GoalWrapper`

You can get an array of all goals in the current match from the `ServerWrapper`:

```cpp
ServerWrapper server = gameWrapper->GetCurrentGameState();
if (server)
{
    ArrayWrapper<GoalWrapper> goals = server.GetGoals();
    // A standard match will have 2 goals in this array.
}
```

## API Description

### Header
`bakkesmod/wrappers/GameObject/GoalWrapper.h`

### Inheritance
`GoalWrapper` -> `ObjectWrapper`

### Key Functions

#### Team Identification

-   `unsigned char GetTeamNum()`: Returns the team index that this goal belongs to.
    -   `0`: Blue team's goal.
    -   `1`: Orange team's goal.

#### Spatial Properties

These functions are crucial for determining the position and volume of the goal.

-   `Vector GetLocation()`: Returns the `Vector` representing the center of the goal on the goal line.
-   `Vector GetDirection()`: Returns the normal `Vector` that points *out* of the goal (i.e., towards the center of the field).
-   `Vector GetWorldCenter()`: Returns the `Vector` for the center of the goal's entire bounding box volume.
-   `Vector GetWorldExtent()`: Returns a `Vector` representing the extents (half the full size) of the goal's bounding box, from its center. You can use this to calculate the full volume of the goal.

#### Other Information

-   `int GetPointsToAward()`: Returns the number of points awarded when a goal is scored in this goal (typically `1`).

## Example: Find the center of the blue team's goal

```cpp
void MyPlugin::FindBlueGoalCenter()
{
    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) { return; }

    GoalWrapper blueGoal;
    ArrayWrapper<GoalWrapper> goals = server.GetGoals();
    for (int i = 0; i < goals.Count(); i++)
    {
        GoalWrapper goal = goals.Get(i);
        if (goal && goal.GetTeamNum() == 0) // Team 0 is blue
        {
            blueGoal = goal;
            break;
        }
    }

    if (blueGoal)
    {
        Vector goalCenter = blueGoal.GetWorldCenter();
        LOG("The blue goal is centered at ({}, {}, {})", goalCenter.X, goalCenter.Y, goalCenter.Z);
    }
}
```
