# CanvasWrapper API

The `CanvasWrapper` is used for all 2D drawing on the screen. It provides a set of functions for drawing text, shapes, and textures.

You do not create a `CanvasWrapper` yourself. Instead, you get one by registering a drawable function with the `GameWrapper`.

## Registering a Drawable Function

To draw on the canvas, you must first register a callback function using `gameWrapper->RegisterDrawable()`. This function will be called every frame that it is possible to draw.

```cpp
// In your onLoad() function:
gameWrapper->RegisterDrawable(std::bind(&MyPlugin::Render, this, std::placeholders::_1));

// Make sure to unregister it in onUnload, if your plugin can be unloaded.
// Notifiers and cvars are automatically unregistered, but drawables are not.
void MyPlugin::onUnload()
{
    gameWrapper->UnregisterDrawables();
}

// Your rendering function:
void MyPlugin::Render(CanvasWrapper canvas)
{
    // Use the canvas object to draw here
}
```

## Drawing Basics

The canvas works like a state machine. You set a position and a color, and then subsequent draw calls will use those settings. The top-left corner of the screen is `(0, 0)`.

### Positioning and Color

-   `void SetPosition(Vector2 pos)`: Sets the drawing cursor's position for subsequent drawing operations.
-   `void SetColor(char R, char G, char B, char A)`: Sets the drawing color using 0-255 values.
-   `void SetColor(LinearColor color)`: Sets the drawing color using a `LinearColor` struct.

### Drawing Shapes

Once a position and color are set, you can draw shapes. The `size` parameter is relative to the position set by `SetPosition`.

-   `void DrawBox(Vector2 size)`: Draws the outline of a box.
-   `void FillBox(Vector2 size)`: Draws a filled box.
-   `void DrawLine(Vector2 start, Vector2 end, float width)`: Draws a line between two screen coordinates.
-   `void FillTriangle(Vector2 p1, Vector2 p2, Vector2 p3)`: Draws a filled triangle.

```cpp
void MyPlugin::Render(CanvasWrapper canvas)
{
    // Draw a 100x50 red box at screen position (10, 10)
    canvas.SetPosition({10, 10});
    canvas.SetColor(255, 0, 0, 255);
    canvas.FillBox({100, 50});
}
```

### Drawing Text

-   `void DrawString(std::string text, float xScale = 1.0f, float yScale = 1.0f)`: Draws a string at the current position.
-   `Vector2F GetStringSize(std::string text, float xScale, float yScale)`: Returns the size of a string if it were to be rendered, without actually drawing it.

```cpp
void MyPlugin::Render(CanvasWrapper canvas)
{
    // Draw "Hello World" in white at (100, 200)
    canvas.SetPosition({100, 200});
    canvas.SetColor(255, 255, 255, 255);
    canvas.DrawString("Hello, World!");
}
```

## 3D to 2D Projection

A common and powerful use case is to draw indicators over objects in the 3D game world. The `Project` function converts a 3D world coordinate into a 2D screen coordinate.

-   `Vector2 Project(Vector location)`: Takes a 3D world-space `Vector` and returns the `Vector2` screen-space coordinate. If the location is off-screen, the returned coordinates may be outside the screen bounds.

```cpp
void MyPlugin::Render(CanvasWrapper canvas)
{
    // Check if we are in a match
    if (!gameWrapper->IsInOnlineGame() && !gameWrapper->IsInFreeplay()) {
        return;
    }

    // Get the ball
    BallWrapper ball = gameWrapper->GetCurrentGameState().GetBall();
    if (!ball.IsNull())
    {
        // Get ball's 3D location and project it to 2D screen space
        Vector ballLocation3D = ball.GetLocation();
        Vector2 ballLocation2D = canvas.Project(ballLocation3D);

        // Draw a small circle or box at the ball's screen location
        canvas.SetPosition(ballLocation2D);
        canvas.SetColor(0, 255, 0, 255); // Green
        canvas.FillBox({10, 10}); // Draw a 10x10 pixel box
    }
}
```
