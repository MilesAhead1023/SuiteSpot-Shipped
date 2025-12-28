# BallWrapper API

The `BallWrapper` represents the ball in the game. You can get the ball by calling `serverWrapper->GetBall()`.

## Ball Prediction

The most important feature of the `BallWrapper` is its ability to predict the ball's future position.

-   `PredictionInfo PredictPosition(float timeAhead)`: Predicts the ball's position, velocity, and angular velocity at a given time in the future. It returns a `PredictionInfo` struct:

```cpp
struct PredictionInfo
{
    unsigned char IsValid;
    Vector Location;
    Vector Velocity;
    Vector AngularVelocity;
};
```

## Touch Information

-   `float GetLastTouchTime()`: Returns the game time when the ball was last touched.
-   `CarWrapper GetLastTouchedBy()`: Returns the last car that touched the ball. (Note: This function might not be directly available and might need to be retrieved from another wrapper, like `ServerWrapper`).

## State and Properties

-   `float GetRadius()`: Returns the ball's radius.
-   `void SetRadius(float newRadius)`: Sets the ball's radius.
-   `Vector GetLocation()`: Returns the ball's current location.
-   `Vector GetVelocity()`: Returns the ball's current velocity.
-   `Vector GetAngularVelocity()`: Returns the ball's current angular velocity.
-   `void SetLocation(Vector newLocation)`: Sets the ball's location.
-   `void SetVelocity(Vector newVelocity)`: Sets the ball's velocity.
-   `void SetAngularVelocity(Vector newAngularVelocity)`: Sets the ball's angular velocity.

## Physics

-   `void SetGravityScale(float newGravityScale)`: Sets the ball's gravity scale.
-   `void SetWorldBounceScale(float newScale)`: Sets the scale of the bounce when the ball hits the world.
-   `void SetCarBounceScale(float newScale)`: Sets the scale of the bounce when the ball hits a car.

## Other

-   `void Launch(Vector& LaunchPosition, Vector& LaunchDirection)`: Launches the ball from a specific position with a given direction.
-   `void Explode(GoalWrapper ExplosionGoal, Vector& ExplodeLocation, PriWrapper Scorer)`: Makes the ball explode.