# ImGui & GuiBase API Documentation

Complete reference for the ImGui UI rendering system and GuiBase window management classes used in BakkesMod plugins.

## Overview

**ImGui (Dear ImGui)** is an immediate-mode graphical user interface library designed for rapid UI development. SuiteSpot uses **ImGui v1.75** with DirectX 11 and Win32 platform backends, augmented with custom widgets for specialized functionality.

**GuiBase** provides two base classes that abstract BakkesMod's UI infrastructure:
- `SettingsWindowBase` - Renders in the BakkesMod settings window (F2 menu)
- `PluginWindowBase` - Renders in a standalone plugin window overlay

## GuiBase Classes

### SettingsWindowBase

Settings window that integrates with BakkesMod's settings tab system.

#### Declaration

```cpp
class SettingsWindowBase : public BakkesMod::Plugin::PluginSettingsWindow
{
public:
    std::string GetPluginName() override;
    void SetImGuiContext(uintptr_t ctx) override;
};
```

#### Methods

**`GetPluginName()`**
- **Returns:** `std::string` - Plugin name displayed in settings
- **Purpose:** Returns the plugin's display name in the BakkesMod settings window
- **Implementation Note:** Returns a template string `"$projectname$"` that should be replaced with your actual plugin name

**`SetImGuiContext(uintptr_t ctx)`**
- **Parameter:** `ctx` - ImGui context pointer (as uintptr_t)
- **Purpose:** Binds ImGui rendering context to the settings window
- **Implementation:** Casts the pointer and sets it as current: `ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx))`
- **Critical:** Must be called before rendering to set up ImGui state

#### Usage Pattern

```cpp
class MyPlugin : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{
public:
    void RenderSettings() override;
};

void MyPlugin::RenderSettings()
{
    ImGui::TextUnformatted("My Plugin Settings");
    // Add ImGui widgets here
}
```

#### Lifecycle

- Called automatically by BakkesMod when user opens the plugin's settings tab
- ImGui context is set before `RenderSettings()` is called
- Window lifetime managed by BakkesMod

---

### PluginWindowBase

Standalone plugin window that renders as a floating overlay in-game.

#### Declaration

```cpp
class PluginWindowBase : public BakkesMod::Plugin::PluginWindow
{
public:
    virtual ~PluginWindowBase() = default;

    bool isWindowOpen_ = false;
    std::string menuTitle_ = "$projectname$";

    std::string GetMenuName() override;
    std::string GetMenuTitle() override;
    void SetImGuiContext(uintptr_t ctx) override;
    bool ShouldBlockInput() override;
    bool IsActiveOverlay() override;
    void OnOpen() override;
    void OnClose() override;
    void Render() override;

    virtual void RenderWindow() = 0;
};
```

#### Member Variables

**`isWindowOpen_`** (bool)
- Current window open state
- Automatically updated when user closes window
- Used by base class to toggle menu visibility

**`menuTitle_`** (std::string)
- Title bar text for the standalone window
- Default: `"$projectname$"` (should be replaced with your name)
- Can be modified at runtime

#### Methods

**`GetMenuName()` → std::string**
- Returns the menu identifier used by BakkesMod
- Default: returns `"$projectname$"`
- Used internally for toggling menu visibility

**`GetMenuTitle()` → std::string**
- Returns the window title bar text
- Returns: `menuTitle_` member variable
- Allows dynamic title changes

**`SetImGuiContext(uintptr_t ctx)` → void**
- Binds ImGui rendering context
- Same as SettingsWindowBase implementation
- Must be called before rendering

**`ShouldBlockInput()` → bool**
- Determines if window should capture mouse/keyboard input
- Returns: `true` if ImGui wants to capture input (mouse or keyboard)
- Prevents game input from being processed while window is focused
- Implementation:
  ```cpp
  ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard
  ```

**`IsActiveOverlay()` → bool**
- Indicates if this is an active overlay window
- Returns: `true` (window is always considered active)
- Used by BakkesMod for rendering priority

**`OnOpen()` → void**
- Called when window is opened
- Sets `isWindowOpen_ = true`
- Override to perform custom initialization

**`OnClose()` → void**
- Called when window is closed
- Sets `isWindowOpen_ = false`
- Override to perform cleanup

**`Render()` → void**
- Main rendering function called each frame (if window is active)
- Automatically manages ImGui::Begin/End window lifecycle
- Calls `RenderWindow()` to render window contents
- Handles collapsed window optimization
- Implementation:
  ```cpp
  if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
  {
      ImGui::End();
      return;  // Window collapsed, skip rendering
  }
  RenderWindow();
  ImGui::End();
  if (!isWindowOpen_) {
      cvarManager->executeCommand("togglemenu " + GetMenuName());
  }
  ```

**`RenderWindow()` → void** *(Pure Virtual)*
- Override this method to render your window contents
- Called inside ImGui::Begin/End context
- Example:
  ```cpp
  void MyPlugin::RenderWindow() override
  {
      ImGui::TextUnformatted("My Window Content");
      ImGui::Button("Click Me");
  }
  ```

#### Usage Pattern

```cpp
class MyPlugin : public BakkesMod::Plugin::BakkesModPlugin, public PluginWindowBase
{
public:
    MyPlugin() { menuTitle_ = "My Custom Window"; }
    void RenderWindow() override;
};

void MyPlugin::RenderWindow()
{
    ImGui::TextUnformatted("Standalone window content");
    if (ImGui::Button("Do Something")) {
        // Handle button click
    }
}

// In onLoad():
cvarManager->registerNotifier("toggle_my_window", [this](std::vector<std::string> args) {
    cvarManager->executeCommand("togglemenu " + GetMenuName());
}, "Toggle the window", PERMISSION_ALL);
```

#### Input Handling

The base class automatically prevents game input from being processed while the ImGui window is focused:
- **Mouse Input:** Blocked if `ImGui::GetIO().WantCaptureMouse` is true
- **Keyboard Input:** Blocked if `ImGui::GetIO().WantCaptureKeyboard` is true
- Allows users to type and interact without accidental game commands

---

## ImGui Core API

### Context & Initialization

**`ImGui::SetCurrentContext(ImGuiContext* ctx)`**
- Sets the active ImGui context for rendering
- Required before any ImGui calls
- Called automatically by GuiBase classes

**`ImGui::GetIO() → ImGuiIO&`**
- Accesses ImGui input/output state
- Used to check if ImGui wants to capture input
- Properties:
  - `WantCaptureMouse` - ImGui window is hovered/focused
  - `WantCaptureKeyboard` - ImGui window wants text input
  - `MousePos` - Current mouse position
  - `IsMouseDown[button]` - Mouse button states

---

## ImGui Widgets

### Text & Labels

**`ImGui::TextUnformatted(const char* text)`**
- Renders plain text without formatting
- Fastest text rendering method
- No format string parsing
- Example: `ImGui::TextUnformatted("Hello World");`

**`ImGui::Text(const char* fmt, ...)`**
- Renders formatted text (printf-style)
- Example: `ImGui::Text("Value: %d", 42);`

**`ImGui::TextColored(ImVec4 col, const char* fmt, ...)`**
- Renders colored text
- Example: `ImGui::TextColored(ImVec4(1,0,0,1), "Error!");` // Red text

**`ImGui::LabelText(const char* label, const char* fmt, ...)`**
- Renders a label followed by text
- Example: `ImGui::LabelText("Status", "Ready");`

### Buttons

**`ImGui::Button(const char* label, const ImVec2& size = ImVec2(0,0)) → bool`**
- Returns `true` when clicked
- Size of (0, 0) uses default sizing
- Example:
  ```cpp
  if (ImGui::Button("Click Me")) {
      // Handle click
  }
  ```

**`ImGui::SmallButton(const char* label) → bool`**
- Smaller variant of Button
- Useful for compact UIs

**`ImGui::ImageButton(ImTextureID user_texture_id, const ImVec2& size, ...) → bool`**
- Button with an image texture instead of text
- Parameters include UV coordinates and tint color

### Input Controls

**`ImGui::InputText(const char* label, char* buf, size_t buf_size, ...) → bool`**
- Text input field
- Returns `true` if value was edited this frame
- Example:
  ```cpp
  char buffer[256] = "";
  if (ImGui::InputText("Name##input", buffer, sizeof(buffer))) {
      // User edited the text
  }
  ```

**`ImGui::InputInt(const char* label, int* v, int step = 1, ...) → bool`**
- Integer input field
- Example: `ImGui::InputInt("Count##count", &count, 1);`

**`ImGui::InputFloat(const char* label, float* v, float step = 0.0f, ...) → bool`**
- Float input field
- Example: `ImGui::InputFloat("Speed##speed", &speed, 0.1f);`

**`ImGui::Checkbox(const char* label, bool* v) → bool`**
- Checkbox control
- Returns `true` if toggled this frame
- Example: `ImGui::Checkbox("Enable##enable", &enabled);`

**`ImGui::RadioButton(const char* label, int* v, int v_button) → bool`**
- Radio button (mutually exclusive option)
- Example:
  ```cpp
  static int selected = 0;
  ImGui::RadioButton("Option A##a", &selected, 0);
  ImGui::RadioButton("Option B##b", &selected, 1);
  ```

### Sliders

**`ImGui::Slider<Type>(const char* label, <Type>* v, <Type> v_min, <Type> v_max, ...)`**
- Drag slider for values
- Returns `true` if value changed
- Variants: `SliderInt`, `SliderFloat`, `SliderFloat2`, `SliderFloat3`, `SliderFloat4`
- Example:
  ```cpp
  float value = 50.0f;
  ImGui::SliderFloat("Progress##prog", &value, 0.0f, 100.0f, "%.1f%%");
  ```

**`ImGui::VSlider<Type>(const char* label, const ImVec2& size, ...)`**
- Vertical slider variant
- Useful for compact layouts

### Dropdowns

**`ImGui::Combo(const char* label, int* current_item, const char* const items[], int items_count, ...) → bool`**
- Dropdown combo box
- Returns `true` when selection changed
- Example:
  ```cpp
  const char* maps[] = {"Stadium", "DFH", "Utopia"};
  ImGui::Combo("Map##map", &currentMapIndex, maps, IM_ARRAYSIZE(maps));
  ```

**`ImGui::Combo(const char* label, int* currIndex, std::vector<std::string>& values)` → bool**
- ImGui extension for vector-based combo boxes
- More convenient than array variant
- Example:
  ```cpp
  std::vector<std::string> options = {"A", "B", "C"};
  ImGui::Combo("Choose##combo", &selected, options);
  ```

**`ImGui::ListBox(const char* label, int* current_item, ...) → bool`**
- Dropdown that displays as a list box
- Similar API to Combo but different appearance
- Example:
  ```cpp
  ImGui::ListBox("Items##list", &selectedIndex, items, itemCount, 5);
  ```

### Layout & Containers

**`ImGui::Begin(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) → bool`**
- Starts a new window
- Returns `false` if window is collapsed (optimization)
- Must be paired with `ImGui::End()`
- Example:
  ```cpp
  if (ImGui::Begin("My Window", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Window content");
      ImGui::End();
  }
  ```

**`ImGui::End() → void`**
- Ends the current window
- Must always call, even if Begin returned false

**`ImGui::BeginChild(const char* str_id, const ImVec2& size = ImVec2(0,0), ...) → bool`**
- Creates a child window (sub-region)
- Returns `false` if collapsed
- Must be paired with `ImGui::EndChild()`
- Example:
  ```cpp
  if (ImGui::BeginChild("ChildRegion##child", ImVec2(400, 200))) {
      ImGui::Text("Child window content");
      ImGui::EndChild();
  }
  ```

**`ImGui::EndChild() → void`**
- Ends the current child window

**`ImGui::BeginGroup() → void`**
- Groups multiple items together
- Must be paired with `ImGui::EndGroup()`
- Affects item positioning

**`ImGui::EndGroup() → void`**
- Ends the current group

**`ImGui::Columns(int count = 1, const char* id = NULL, bool border = true) → void`**
- Creates multi-column layout
- Note: Can be used directly without BeginChild (columns will fill available width)
- Auto-proportional column sizing
- Example:
  ```cpp
  ImGui::Columns(3, "TableID##table", true);
  ImGui::Text("Col 1");
  ImGui::NextColumn();
  ImGui::Text("Col 2");
  ImGui::NextColumn();
  ImGui::Text("Col 3");
  ImGui::NextColumn();
  ImGui::Columns(1);  // End columns
  ```

**`ImGui::NextColumn() → void`**
- Moves cursor to next column in a multi-column layout

**`ImGui::Separator() → void`**
- Renders a horizontal separator line
- Example: `ImGui::Separator();`

**`ImGui::SameLine(float offset_from_start = 0.0f, float spacing = -1.0f) → void`**
- Positions next item on same line as previous item
- Useful for horizontal layouts
- Example:
  ```cpp
  ImGui::Button("Button 1##btn1");
  ImGui::SameLine();
  ImGui::Button("Button 2##btn2");  // Same line as Button 1
  ```

**`ImGui::Spacing() → void`**
- Adds vertical spacing between items

**`ImGui::NewLine() → void`**
- Moves to next line (same as pressing Enter)

### Tabs

**`ImGui::BeginTabBar(const char* str_id, ImGuiTabBarFlags flags = 0) → bool`**
- Starts a tab bar
- Returns `false` if no tabs should be rendered
- Must be paired with `ImGui::EndTabBar()`
- Example:
  ```cpp
  if (ImGui::BeginTabBar("MyTabs##tabs")) {
      if (ImGui::BeginTabItem("Tab 1##tab1")) {
          ImGui::Text("Tab 1 content");
          ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Tab 2##tab2")) {
          ImGui::Text("Tab 2 content");
          ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
  }
  ```

**`ImGui::BeginTabItem(const char* label, bool* p_open = NULL, ImGuiTabBarFlags flags = 0) → bool`**
- Starts a tab item
- Returns `true` if tab is active
- Must be paired with `ImGui::EndTabItem()`

**`ImGui::EndTabItem() → void`**
- Ends the current tab item

**`ImGui::EndTabBar() → void`**
- Ends the current tab bar

### Collapsible Headers

**`ImGui::CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0) → bool`**
- Collapsible header (tree node header style)
- Returns `true` if expanded
- Example:
  ```cpp
  if (ImGui::CollapsingHeader("Settings##settings")) {
      ImGui::Text("Setting 1");
      ImGui::Text("Setting 2");
  }
  ```

**`ImGui::TreeNode(const char* label) → bool`**
- Tree node (expandable parent)
- Returns `true` if expanded
- Must be paired with `ImGui::TreePop()` if expanded
- Example:
  ```cpp
  if (ImGui::TreeNode("Parent##parent")) {
      ImGui::Text("Child item 1");
      ImGui::Text("Child item 2");
      ImGui::TreePop();
  }
  ```

**`ImGui::TreePop() → void`**
- Ends the current tree node

### Popups & Modals

**`ImGui::OpenPopup(const char* str_id) → void`**
- Opens a popup by ID
- Must be paired with ImGui::BeginPopup or ImGui::BeginPopupModal

**`ImGui::BeginPopup(const char* str_id, ImGuiWindowFlags flags = 0) → bool`**
- Begins a popup window
- Returns `false` if popup is not open
- Closed when clicking outside or pressing ESC
- Example:
  ```cpp
  if (ImGui::BeginPopup("Context##ctx")) {
      if (ImGui::MenuItem("Option 1")) { /* ... */ }
      if (ImGui::MenuItem("Option 2")) { /* ... */ }
      ImGui::EndPopup();
  }
  ```

**`ImGui::EndPopup() → void`**
- Ends the current popup

### Styling & Colors

**`ImGui::GetStyle() → ImGuiStyle&`**
- Returns the current ImGui style (colors, sizes, etc.)
- Used to access default colors and modify appearance

**`ImGui::PushStyleColor(ImGuiCol idx, ImU32 col) → void`**
- Pushes a color onto the style stack
- Must be paired with `ImGui::PopStyleColor()`
- Example:
  ```cpp
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 1));  // Red
  ImGui::Button("Red Button");
  ImGui::PopStyleColor();
  ```

**`ImGui::PopStyleColor(int count = 1) → void`**
- Pops color(s) from the style stack

**`ImGui::GetColorU32(ImGuiCol idx) → ImU32`**
- Gets the current color for an ImGui element as U32 format
- Useful for custom drawing

### IDs & Uniqueness

**`ImGui::PushID(const char* str_id) → void`**
- Pushes a string-based ID onto the ID stack
- Must be paired with `ImGui::PopID()`
- Used to disambiguate widgets with same label

**`ImGui::PopID() → void`**
- Pops an ID from the stack

**ID Suffixes (##)**
- Any `##` in a label creates a hidden ID suffix
- Example: `"Button##button_id"` renders as "Button" with ID "button_id"
- Allows multiple widgets with same visible text but different IDs

---

## Custom ImGui Extensions

### RangeSlider

Two-ended slider for selecting a range of values.

**`ImGui::RangeSliderFloat(const char* label, float* v1, float* v2, float v_min, float v_max, const char* format = "(%.3f, %.3f)", float power = 1.0f) → bool`**
- Creates a range slider for two float values
- Returns `true` if either value changed
- Parameters:
  - `v1, v2` - Pointers to min and max values
  - `v_min, v_max` - Range bounds
  - `format` - Display format string
  - `power` - Curve power for non-linear scaling
- Variants: `RangeSliderInt`, `RangeSliderFloat2`, `RangeSliderFloat3`, `RangeSliderFloat4`
- Example:
  ```cpp
  float min_val = 20.0f, max_val = 80.0f;
  ImGui::RangeSliderFloat("Range##range", &min_val, &max_val, 0.0f, 100.0f);
  ```

**`ImGui::RangeVSliderFloat(const char* label, const ImVec2& size, float* v1, float* v2, float v_min, float v_max, ...)`**
- Vertical range slider variant

---

### SearchableCombo

Searchable dropdown with text filter.

**`ImGui::SearchableCombo(const char* label, int* current_item, std::vector<std::string> items, const char* default_preview_text, const char* input_preview_value, int popup_max_height_in_items = -1) → bool`**
- Dropdown with real-time search filtering
- Returns `true` when selection changed
- Parameters:
  - `label` - Widget label
  - `current_item` - Selected index
  - `items` - Vector of selectable items
  - `default_preview_text` - Text shown when nothing selected
  - `input_preview_value` - Placeholder text in search field
  - `popup_max_height_in_items` - Max visible items (-1 = auto)
- Example:
  ```cpp
  std::vector<std::string> maps = {"Stadium", "DFH", "Utopia", "Pillars"};
  ImGui::SearchableCombo("Select Map##mapselect", &selectedIndex, maps, "Choose a map...", "Search maps...", 5);
  ```

**`ImGui::BeginSearchableCombo(...) → bool`** / **`ImGui::EndSearchableCombo() → void`**
- Lower-level API for custom combo rendering
- Allows more control over appearance and behavior

---

### Timeline

Timeline widget for visualizing and editing events over time.

**`ImGui::BeginTimeline(const char* str_id, float max_time) → bool`**
- Starts a timeline
- Parameters:
  - `str_id` - Unique identifier
  - `max_time` - Maximum time value on timeline
- Returns `false` if timeline should skip rendering
- Must be paired with `ImGui::EndTimeline()`
- Example:
  ```cpp
  if (ImGui::BeginTimeline("MyTimeline##timeline", 10.0f)) {
      ImGui::TimelineEvent("Event1##ev1", times);
      ImGui::TimelineEvent("Event2##ev2", otherTimes);
      ImGui::EndTimeline(5, currentTime);
  }
  ```

**`ImGui::TimelineEvent(const char* str_id, float times[2]) → bool`**
- Adds an event to the timeline
- Parameters:
  - `str_id` - Event identifier
  - `times` - Array of 2 floats: [start_time, end_time]
- Returns `true` if event was modified
- Can only be called between `BeginTimeline()` and `EndTimeline()`

**`ImGui::EndTimeline(float current_time = -1) → void`**
- Ends the timeline
- Parameter `current_time` shows playhead position (-1 = hidden)

---

### VariousControls

Miscellaneous widgets and utilities.

**`ImGui::CheckButton(const char* label, bool* pvalue) → bool`**
- Checkbox rendered as a button
- Returns `true` if toggled
- Example: `ImGui::CheckButton("Enable##en", &enabled);`

**`ImGui::SmallCheckButton(const char* label, bool* pvalue) → bool`**
- Smaller variant of CheckButton

**`ImGui::ProgressBar(const char* optionalPrefixText, float value, const float minValue = 0.f, const float maxValue = 1.f, const char* format = "%1.0f%%", const ImVec2& sizeOfBarWithoutTextInPixels = ImVec2(-1,-1), ...) → float`**
- Progress bar visualization
- Returns the normalized value (0.0-1.0)
- Parameters:
  - `optionalPrefixText` - Text before the bar
  - `value` - Current progress value
  - `minValue, maxValue` - Value range
  - `format` - Display format string
- Example:
  ```cpp
  ImGui::ProgressBar("Loading", 65.0f, 0.0f, 100.0f, "%.0f%%");
  ```

**`ImGui::PopupMenuSimple(...) → int`**
- Context menu that appears at mouse position
- Returns index of selected item (-1 if none)
- Disappears when mouse moves away

**`ImGui::InputTextMultilineWithHorizontalScrolling(...)`**
- Multi-line text editor with horizontal scrolling support
- Useful for code editing or large text blocks

**`ImGui::ColorChooser(bool* open, ImVec4* pColorOut = NULL, bool supportsAlpha = true) → bool`**
- Color picker widget
- Returns `true` when color is selected
- Parameters:
  - `open` - Opens/closes the picker
  - `pColorOut` - Output color (RGBA)
  - `supportsAlpha` - Include alpha channel

**`ImGui::ImageButtonWithText(ImTextureID texId, const char* label, const ImVec2& imageSize = ImVec2(0,0), ...) → bool`**
- Button combining image and text
- Returns `true` when clicked
- Example:
  ```cpp
  if (ImGui::ImageButtonWithText(textureId, "Load Map", ImVec2(32, 32))) {
      // Handle click
  }
  ```

**`ImGui::KnobFloat(const char* label, float* p_value, float v_min, float v_max, float v_step = 50.f) → bool`**
- Rotary knob control (like a volume dial)
- Returns `true` if value changed
- Example: `ImGui::KnobFloat("Volume##vol", &volume, 0.0f, 100.0f, 1.0f);`

---

## TreeView Widget

Advanced hierarchical tree/list widget with multiple selection modes and state management.

### TreeViewNode

Individual node in the tree structure.

**`TreeViewNode::Data`** - Node display information
- `displayName` - Text shown in the tree
- `tooltip` - Hover tooltip text
- `userText` - Custom user data
- `userId` - Numeric user identifier

**`TreeViewNode::State`** - Node state flags
- `STATE_OPEN` - Node is expanded
- `STATE_SELECTED` - Node is selected
- `STATE_CHECKED` - Node checkbox is checked
- `STATE_DISABLED` - Node is grayed out
- `STATE_HIDDEN` - Node not visible
- `STATE_COLOR1/2/3` - User color states

**Node Operations**
- `addChildNode(Data& data)` - Add child node
- `addSiblingNode(Data& data)` - Add sibling node
- `getChildNode(int index)` - Get child by index
- `getNumChildNodes()` - Number of children
- `deleteAllChildNodes()` - Remove all children
- `sortChildNodes(recursive, comparator)` - Sort children
- `addState(int flag)` - Add state flag
- `removeState(int flag)` - Remove state flag
- `isStatePresent(int flag)` - Check state

### TreeView

Root tree container with rendering and event handling.

**Creation**
```cpp
TreeView treeView(
    TreeView::MODE_ALL,           // Selection mode
    true,                         // Allow multiple selection
    TreeView::MODE_LEAF,          // Checkbox mode
    true,                         // Auto checkbox behavior
    true                          // Inherit disabled look
);

// Add root nodes
TreeView::Data data("Node Name", "Tooltip text", "user text", 0);
TreeViewNode* root = treeView.addRootNode(data);
root->addChildNode(TreeView::Data("Child"));
```

**Rendering**
```cpp
if (treeView.render()) {
    TreeViewNode::Event event = treeView.getLastEvent();
    if (event.type == TreeViewNode::EVENT_STATE_CHANGED) {
        // Handle state change
    }
}
```

**Key Properties**
- `selectionMode` - Controls how items are selected
- `allowMultipleSelection` - Toggle multiple selection
- `checkboxMode` - Which nodes show checkboxes
- `getLastEvent()` - Recent user interaction

---

## Common Patterns

### Safe Widget IDs

Use `##` suffix to create hidden IDs while keeping readable labels:

```cpp
// Multiple widgets with same label, different IDs
ImGui::Button("Load##load1");
ImGui::Button("Load##load2");
ImGui::Button("Load##load3");
```

### Conditional Rendering

Check window collapsed state to optimize:

```cpp
if (ImGui::Begin("Window##win")) {
    // Only render content if not collapsed
    ImGui::Text("Content");
    ImGui::End();
}
// Always safe - ImGui::End() not called if Begin() returned false
```

### Per-Frame State Management

Use local/static variables for temporary state:

```cpp
static bool popup_open = false;
if (ImGui::Button("Open##btn")) {
    popup_open = true;
}
if (popup_open && ImGui::BeginPopup("MyPopup##popup")) {
    if (ImGui::MenuItem("Option")) {
        // Handle selection
        popup_open = false;
    }
    ImGui::EndPopup();
}
```

### Responsive Layouts

Let ImGui handle sizing automatically:

```cpp
ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);  // Auto-size on first frame
if (ImGui::Begin("Window", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Content");
    ImGui::End();
}
```

---

## Vector Helpers

ImGui extensions for std::vector support.

**`ImGui::Combo(const char* label, int* currIndex, std::vector<std::string>& values) → bool`**
- Combo box from string vector
- Cleaner than managing parallel arrays

**`ImGui::ListBox(const char* label, int* currIndex, std::vector<std::string>& values) → bool`**
- List box from string vector

---

## Important Notes

### ImGui Context Requirements

- ImGui context must be set via `ImGui::SetCurrentContext()` before any widget calls
- GuiBase classes handle this automatically in their Render methods
- Calling ImGui functions without a valid context will crash

### DirectX 11 Backend

- Uses `imgui_impl_dx11.cpp` and `imgui_impl_dx11.h` for rendering
- Win32 backend for window/input (`imgui_impl_win32.cpp`)
- Automatically handled by BakkesMod integration

### Performance Considerations

1. **Immediate Mode:** ImGui redraws every frame - cache expensive computations
2. **Column Performance:** Direct use of `ImGui::Columns()` is efficient; no BeginChild wrapper needed
3. **Large Lists:** Consider using `ImGuiListClipper` for very large item lists
4. **State Caching:** Cache filter/sort results when dealing with 2000+ items

### Common Gotchas

1. **ID Stack:** Two widgets with identical labels and same scope will conflict. Use `##` suffix to disambiguate.
2. **Begin/End Pairs:** Always match `Begin()` with `End()`, even if Begin() returns false.
3. **Child Windows:** Don't nest too many child windows - performance degrades with deep hierarchies.
4. **Input Blocking:** ImGui automatically blocks game input when focused - use `ShouldBlockInput()` if you need custom behavior.
5. **Static IDs:** Window positions/sizes are persisted across frames using IDs. Different IDs = new windows.

---

## Version Information

- **ImGui Version:** 1.75
- **DirectX Version:** DirectX 11
- **Platform:** Windows (Win32)
- **SuiteSpot Integration:** BakkesMod Plugin SDK v95

## References

- ImGui Official: https://github.com/ocornut/imgui
- ImGui FAQ: http://dearimgui.org/faq
- ImGui Wiki: https://github.com/ocornut/imgui/wiki
