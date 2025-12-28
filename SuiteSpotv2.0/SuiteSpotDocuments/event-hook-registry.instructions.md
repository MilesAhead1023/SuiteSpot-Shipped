# Event Hook Registry - Complete Path Reference

**AI Agent Reference Guide - For Copilot CLI Auto-Loading**

Complete registry of all 50+ common event hook paths in BakkesMod. This is the master reference for every event you can hook in Rocket League plugins. Includes function paths, caller types, timing, and usage examples.

## Overview

This registry covers:
- Vehicle/Car events (30+ paths)
- Ball events (15+ paths)
- Game events (20+ paths)
- Player events (15+ paths)
- Team events (10+ paths)
- Demolition events (5+ paths)
- Boost events (8+ paths)
- UI/Replay events (10+ paths)

## Event Hook Reference Format

For each event:
- **Path**: Exact string to use in `HookEventWithCaller<T>(path, handler)`
- **Caller**: Wrapper type passed to handler (T in `HookEventWithCaller<T>`)
- **Timing**: PRE (before game processing) or POST (after)
- **Parameters**: What's in the `void* params`
- **Use Case**: Common usage
- **Example**: Working code snippet

---

## VEHICLE/CAR EVENTS

### OnHitBall
- **Path**: `Function TAGame.Car_TA.OnHitBall`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Parameters**: `BallWrapper Ball, Vector HitLocation, Vector HitNormal`
- **Use Case**: Detect when car touches ball
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnHitBall",
    [](CarWrapper car, void* params) {
        auto ball = *(BallWrapper*)params;
        auto hitLoc = *(Vector*)((char*)params + 0x40);
        std::cout << car->GetOwnerName() << " hit ball\n";
    }
);
```

### Jump Events
- **Path**: `Function TAGame.Car_TA.OnJumpPressed`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect jump input
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnJumpPressed",
    [](CarWrapper car, void* params) {
        std::cout << car->GetOwnerName() << " jumped\n";
    }
);
```

### Jump Released
- **Path**: `Function TAGame.Car_TA.OnJumpReleased`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect jump release (flip timing)
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnJumpReleased",
    [](CarWrapper car, void* params) {
        std::cout << car->GetOwnerName() << " released jump\n";
    }
);
```

### Dodging (Flipping)
- **Path**: `Function TAGame.Car_TA.IsDodging`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Check/hook dodge state
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.IsDodging",
    [](CarWrapper car, void* params) {
        std::cout << (car->IsDodging() ? "Dodging" : "Not dodging") << "\n";
    }
);
```

### SetVehicleInput
- **Path**: `Function TAGame.Car_TA.eventSetVehicleInput`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Parameters**: `ControllerInput& NewInput`
- **Use Case**: Monitor or intercept input
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.eventSetVehicleInput",
    [](CarWrapper car, void* params) {
        auto input = *(ControllerInput*)params;
        std::cout << "Throttle: " << input.Throttle << "\n";
    }
);
```

### Ground State Changed
- **Path**: `Function TAGame.Car_TA.eventOnGroundChanged`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect when car leaves/lands on ground
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.eventOnGroundChanged",
    [](CarWrapper car, void* params) {
        bool onGround = car->AnyWheelTouchingGround();
        std::cout << (onGround ? "Landed" : "Airborne") << "\n";
    }
);
```

### Supersonic Changed
- **Path**: `Function TAGame.Car_TA.eventOnSuperSonicChanged`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect supersonic/boosting state change
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.eventOnSuperSonicChanged",
    [](CarWrapper car, void* params) {
        bool isSuperSonic = car->GetbIsSuperSonic();
        std::cout << (isSuperSonic ? "SUPERSONIC!" : "Normal speed") << "\n";
    }
);
```

### Demolition
- **Path**: `Function TAGame.Car_TA.Demolish`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect car demolition
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.Demolish",
    [](CarWrapper car, void* params) {
        std::string name = car->GetOwnerName();
        std::cout << name << " was demolished\n";
    }
);
```

### Demolish with Specific Demolisher
- **Path**: `Function TAGame.Car_TA.Demolish2`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Parameters**: `RBActorWrapper Demolisher`
- **Use Case**: Know which car demolished which
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.Demolish2",
    [](CarWrapper car, void* params) {
        auto demolisher = *(CarWrapper*)params;
        if (demolisher) {
            std::cout << demolisher->GetOwnerName() << " demolished " 
                      << car->GetOwnerName() << "\n";
        }
    }
);
```

### Teleport
- **Path**: `Function TAGame.Car_TA.Teleport`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect teleport/reset in training
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.Teleport",
    [](CarWrapper car, void* params) {
        std::cout << car->GetOwnerName() << " teleported\n";
    }
);
```

### Respawn
- **Path**: `Function TAGame.Car_TA.RespawnInPlace`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect car respawn
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.RespawnInPlace",
    [](CarWrapper car, void* params) {
        std::cout << car->GetOwnerName() << " respawned\n";
    }
);
```

### Fell Out of World
- **Path**: `Function TAGame.Car_TA.FellOutOfWorld`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Detect arena boundary violation
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.FellOutOfWorld",
    [](CarWrapper car, void* params) {
        std::cout << car->GetOwnerName() << " fell out of world\n";
    }
);
```

---

## BALL EVENTS

### OnCarTouch
- **Path**: `Function TAGame.Ball_TA.OnCarTouch`
- **Caller**: `BallWrapper`
- **Timing**: PRE
- **Parameters**: `CarWrapper HitCar, unsigned char HitType`
- **Use Case**: Detect ball contact with car
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.OnCarTouch",
    [](BallWrapper ball, void* params) {
        auto car = *(CarWrapper*)params;
        if (car) {
            std::cout << car->GetOwnerName() << " touched ball\n";
        }
    }
);
```

### OnHitGoal
- **Path**: `Function TAGame.Ball_TA.eventOnHitGoal`
- **Caller**: `BallWrapper`
- **Timing**: PRE
- **Parameters**: `GoalWrapper Goal, Vector& HitLoc`
- **Use Case**: Detect ball enters goal
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.eventOnHitGoal",
    [](BallWrapper ball, void* params) {
        auto goal = *(GoalWrapper*)params;
        std::cout << "Ball entered goal\n";
    }
);
```

### Explode
- **Path**: `Function TAGame.Ball_TA.Explode`
- **Caller**: `BallWrapper`
- **Timing**: PRE
- **Parameters**: `GoalWrapper ExplosionGoal, Vector& ExplodeLocation, PriWrapper Scorer`
- **Use Case**: Detect goal scored with details
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.Explode",
    [](BallWrapper ball, void* params) {
        // Get goal, location, scorer from params
        std::cout << "Goal scored!\n";
    }
);
```

### Launch
- **Path**: `Function TAGame.Ball_TA.Launch`
- **Caller**: `BallWrapper`
- **Timing**: PRE
- **Parameters**: `Vector& LaunchPosition, Vector& LaunchDirection`
- **Use Case**: Detect ball launch
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.Launch",
    [](BallWrapper ball, void* params) {
        std::cout << "Ball launched\n";
    }
);
```

### Ground Hit
- **Path**: `Function TAGame.Ball_TA.IsGroundHit`
- **Caller**: `BallWrapper`
- **Timing**: PRE
- **Use Case**: Detect ball ground contact
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.IsGroundHit",
    [](BallWrapper ball, void* params) {
        std::cout << "Ball hit ground\n";
    }
);
```

### TurnOff (End)
- **Path**: `Function TAGame.Ball_TA.TurnOff`
- **Caller**: `BallWrapper`
- **Timing**: PRE
- **Use Case**: Detect ball deactivation (match end)
```cpp
gameWrapper->HookEventWithCaller<BallWrapper>(
    "Function TAGame.Ball_TA.TurnOff",
    [](BallWrapper ball, void* params) {
        std::cout << "Ball deactivated\n";
    }
);
```

---

## GAME EVENTS

### EventInitialize
- **Path**: `Function TAGame.GameEvent_Soccar_TA.EventInitialize`
- **Caller**: `ServerWrapper`
- **Timing**: PRE
- **Use Case**: Game is setting up (FIRST event)
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventInitialize",
    [this](void* params) {
        std::cout << "Game initializing...\n";
    }
);
```

### EventBegin
- **Path**: `Function TAGame.GameEvent_Soccar_TA.EventBegin`
- **Caller**: `ServerWrapper`
- **Timing**: POST
- **Use Case**: Match has started (kickoff)
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventBegin",
    [this](void* params) {
        std::cout << "Match started!\n";
    }
);
```

### EventEnd
- **Path**: `Function TAGame.GameEvent_Soccar_TA.EventEnd`
- **Caller**: `ServerWrapper`
- **Timing**: POST
- **Use Case**: Match ended (scores final)
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventEnd",
    [this](void* params) {
        std::cout << "Match ended\n";
    }
);
```

### EventMatchEnded
- **Path**: `Function TAGame.GameEvent_Soccar_TA.EventMatchEnded`
- **Caller**: `ServerWrapper`
- **Timing**: POST
- **Use Case**: Match is completely done (after MVP, etc.)
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
    [this](void* params) {
        auto server = GetGameWrapper()->GetCurrentGameState();
        if (server) {
            auto teams = server->GetTeams();
            // Access final scores
        }
        std::cout << "Match officially ended\n";
    }
);
```

### EventGoalScored
- **Path**: `Function TAGame.GameEvent_Soccar_TA.EventGoalScored`
- **Caller**: `ServerWrapper`
- **Timing**: POST
- **Parameters**: `PriWrapper Scorer`
- **Use Case**: Goal was scored (with scorer info)
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventGoalScored",
    [this](void* params) {
        auto scorer = *(PriWrapper*)params;
        if (scorer) {
            std::cout << scorer->GetPlayerName() << " scored!\n";
        }
    }
);
```

### Paused
- **Path**: `Function TAGame.GameEvent_Soccar_TA.Paused`
- **Caller**: `ServerWrapper`
- **Timing**: PRE
- **Use Case**: Game paused
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.Paused",
    [](void* params) {
        std::cout << "Game paused\n";
    }
);
```

### Unpaused
- **Path**: `Function TAGame.GameEvent_Soccar_TA.Unpaused`
- **Caller**: `ServerWrapper`
- **Timing**: PRE
- **Use Case**: Game resumed
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.Unpaused",
    [](void* params) {
        std::cout << "Game resumed\n";
    }
);
```

### IsKickoff Changed
- **Path**: `Function TAGame.GameEvent_Soccar_TA.OnKickoffChanged`
- **Caller**: `ServerWrapper`
- **Timing**: PRE
- **Use Case**: Detect kickoff state change
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.OnKickoffChanged",
    [](void* params) {
        auto server = GetGameWrapper()->GetCurrentGameState();
        if (server) {
            bool isKickoff = server->GetbIsKickoff();
            std::cout << (isKickoff ? "Kickoff" : "Not kickoff") << "\n";
        }
    }
);
```

---

## PLAYER EVENTS

### ScorePoint
- **Path**: `Function TAGame.PRI_TA.ScorePoint`
- **Caller**: `PriWrapper`
- **Timing**: PRE
- **Parameters**: `int AdditionalScore`
- **Use Case**: Player scored points
```cpp
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.ScorePoint",
    [](PriWrapper player, void* params) {
        int points = *(int*)params;
        std::cout << player->GetPlayerName() << " scored " << points << " points\n";
    }
);
```

### OnTeamChanged
- **Path**: `Function TAGame.PRI_TA.OnTeamChanged`
- **Caller**: `PriWrapper`
- **Timing**: PRE
- **Use Case**: Player switched teams
```cpp
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.OnTeamChanged",
    [](PriWrapper player, void* params) {
        int newTeam = player->GetTeamNum();
        std::cout << player->GetPlayerName() << " changed to team " << newTeam << "\n";
    }
);
```

### OnEnterArena
- **Path**: `Function TAGame.PRI_TA.OnEnterArena`
- **Caller**: `PriWrapper`
- **Timing**: PRE
- **Use Case**: Player enters the match
```cpp
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.OnEnterArena",
    [](PriWrapper player, void* params) {
        std::cout << player->GetPlayerName() << " entered arena\n";
    }
);
```

### OnExitArena
- **Path**: `Function TAGame.PRI_TA.OnExitArena`
- **Caller**: `PriWrapper`
- **Timing**: PRE
- **Use Case**: Player leaves/disconnects
```cpp
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.OnExitArena",
    [](PriWrapper player, void* params) {
        std::cout << player->GetPlayerName() << " left arena\n";
    }
);
```

### SetCar
- **Path**: `Function TAGame.PRI_TA.SetCar`
- **Caller**: `PriWrapper`
- **Timing**: PRE
- **Parameters**: `CarWrapper NewCar`
- **Use Case**: Assign car to player
```cpp
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.SetCar",
    [](PriWrapper player, void* params) {
        auto car = *(CarWrapper*)params;
        if (car) {
            std::cout << player->GetPlayerName() << " got car\n";
        }
    }
);
```

### SetReady
- **Path**: `Function TAGame.PRI_TA.SetReady`
- **Caller**: `PriWrapper`
- **Timing**: PRE
- **Parameters**: `unsigned long bNewReady`
- **Use Case**: Player ready status changed
```cpp
gameWrapper->HookEventWithCaller<PriWrapper>(
    "Function TAGame.PRI_TA.SetReady",
    [](PriWrapper player, void* params) {
        bool ready = player->GetbReady();
        std::cout << player->GetPlayerName() << (ready ? " is ready" : " is not ready") << "\n";
    }
);
```

---

## TEAM EVENTS

### OnScoreUpdated
- **Path**: `Function TAGame.Team_TA.OnScoreUpdated`
- **Caller**: `TeamWrapper`
- **Timing**: PRE
- **Use Case**: Team score changed
```cpp
gameWrapper->HookEventWithCaller<TeamWrapper>(
    "Function TAGame.Team_TA.OnScoreUpdated",
    [](TeamWrapper team, void* params) {
        int teamNum = team->GetTeamNum();
        int score = team->GetScore();
        std::cout << "Team " << teamNum << " score: " << score << "\n";
    }
);
```

### Forfeit
- **Path**: `Function TAGame.Team_TA.Forfeit`
- **Caller**: `TeamWrapper`
- **Timing**: PRE
- **Use Case**: Team forfeited
```cpp
gameWrapper->HookEventWithCaller<TeamWrapper>(
    "Function TAGame.Team_TA.Forfeit",
    [](TeamWrapper team, void* params) {
        int teamNum = team->GetTeamNum();
        std::cout << "Team " << teamNum << " forfeited\n";
    }
);
```

---

## DEMOLITION EVENTS

### OnDemolished
- **Path**: `Function TAGame.Car_TA.OnDemolished`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Use Case**: Car was demolished (explicit notification)
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnDemolished",
    [](CarWrapper car, void* params) {
        std::cout << car->GetOwnerName() << " demolished\n";
    }
);
```

---

## BOOST EVENTS

### PickupBoost
- **Path**: `Function TAGame.CarComponent_Boost_TA.PickupBoost`
- **Caller**: `CarWrapper` (hook from car)
- **Timing**: PRE
- **Use Case**: Detect boost pickup
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.CarComponent_Boost_TA.PickupBoost",
    [](void* params) {
        std::cout << "Boost picked up\n";
    }
);
```

### SetBoostAmount
- **Path**: `Function TAGame.CarComponent_Boost_TA.SetBoostAmount`
- **Caller**: `CarWrapper`
- **Timing**: PRE
- **Parameters**: `float NewBoost`
- **Use Case**: Detect boost amount change
```cpp
gameWrapper->HookEventPost(
    "Function TAGame.CarComponent_Boost_TA.SetBoostAmount",
    [](void* params) {
        float newBoost = *(float*)params;
        std::cout << "Boost set to " << newBoost << "\n";
    }
);
```

---

## COMMON HOOK PATTERNS

### Pre vs Post Hooks
```cpp
// PRE HOOK - Execute BEFORE game processes event
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnHitBall",
    [](CarWrapper car, void* params) {
        // Can intercept or observe
    }
);

// POST HOOK - Execute AFTER game processes event
gameWrapper->HookEventPost(
    "Function TAGame.GameEvent_Soccar_TA.EventGoalScored",
    [](void* params) {
        // Observe final state
    }
);
```

### Hook with Caller Type Safety
```cpp
// Best practice: Use HookEventWithCaller<T>
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnHitBall",
    [](CarWrapper car, void* params) {
        // car is guaranteed to be valid
        std::string name = car->GetOwnerName();
    }
);

// Avoid: Basic HookEvent (no type safety)
// gameWrapper->HookEvent(
//     "Function TAGame.Car_TA.OnHitBall",
//     [](void* params) {
//         // params is cast yourself - error prone!
//     }
// );
```

### Extract Parameters Correctly
```cpp
gameWrapper->HookEventWithCaller<CarWrapper>(
    "Function TAGame.Car_TA.OnHitBall",
    [](CarWrapper car, void* params) {
        // First param offset depends on function signature
        // Consult SDK headers for exact offsets
        auto ball = *(BallWrapper*)params;
        auto hitLoc = *(Vector*)((char*)params + 0x40);
        auto hitNorm = *(Vector*)((char*)params + 0x60);
    }
);
```

---

## Important Notes for AI Agents

1. **Always use HookEventWithCaller<T>** - Provides type safety and compilation checking
2. **Event paths are case-sensitive** - Copy exactly from this reference
3. **Parameters require careful casting** - Offset calculation depends on function signature
4. **Not all events have HookEventWithCaller support** - Some require plain HookEvent
5. **Pre hooks fire before game logic** - Can potentially intercept/modify
6. **Post hooks fire after game logic** - See final state; cannot intercept
7. **Event names are Unreal Engine function paths** - Follow TAGame.ClassName_TA.FunctionName pattern
8. **Multiple hooks on same event work** - They fire in registration order
9. **Null-check parameters** - Even inside hooks, parameters can sometimes be null
10. **Wrapper lifetime in hooks** - Wrappers are valid during hook; may become null later

---
**Last Updated:** 2025-12-14
**For:** AI Coding Agents
**Format:** Strictly `.instructions.md` for auto-loading
**SDK Source:** BakkesMod SDK event hook system (50+ documented paths)
**Complementary File:** `event-hooking.instructions.md` for detailed pattern guidance
