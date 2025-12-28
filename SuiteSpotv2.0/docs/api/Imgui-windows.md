# Creating ImGui Windows in BakkesMod

Complete guide for creating standalone ImGui windows that use BakkesMod's ImGui context as the backing.

## Overview

BakkesMod provides ImGui integration at two levels:

1. **Settings Window** (`PluginSettingsWindow`) - Built into the main settings menu
2. **Standalone Windows** (`PluginWindow`) - Separate overlay windows with their own menu entry

Both use BakkesMod's ImGui context and rendering system.

---

## Architecture

```
BakkesMod ImGui Context (shared)
    ↓
    ├─→ SettingsWindow (in main settings menu)
    │   └─ RenderSettings() is called by BakkesMod
    │
    └─→ PluginWindow(s) (separate overlay windows)
        ├─ Window 1: Training Stats
        ├─ Window 2: Custom Window
        └─ Window N: Any custom window
```

**Key Point:** BakkesMod handles the ImGui context (`ImGui::SetCurrentContext()`), you just provide the window rendering.

---

## Step-by-Step: Create a Standalone Window

### Step 1: Create Header File

**`MyCustomWindow.h`**
```cpp
#pragma once
#include "GuiBase.h"  // Provides PluginWindowBase
#include <memory>

class MyCustomWindow : public PluginWindowBase {
public:
    MyCustomWindow();
    ~MyCustomWindow() = default;
    
    // Override from PluginWindowBase
    void RenderWindow() override;
    
    // Control window state
    void Open();
    void Close();
    bool IsOpen() const { return isWindowOpen_; }

private:
    // Your window state here
    std::string someData;
    int selectedIndex = 0;
    
    // Render helper methods
    void RenderHeader();
    void RenderContent();
    void RenderFooter();
};
```

### Step 2: Create Implementation File

**`MyCustomWindow.cpp`**
```cpp
#include "pch.h"
#include "MyCustomWindow.h"

MyCustomWindow::MyCustomWindow() {
    // Set window title (shown in menu and title bar)
    menuTitle_ = "My Custom Window";
}

void MyCustomWindow::RenderWindow() {
    // This is called every frame by BakkesMod
    // The ImGui::Begin/End is handled by PluginWindowBase::Render()
    
    RenderHeader();
    ImGui::Separator();
    RenderContent();
    ImGui::Separator();
    RenderFooter();
}

void MyCustomWindow::RenderHeader() {
    ImGui::TextUnformatted("My Custom Window");
    ImGui::SameLine(400);
    if (ImGui::Button("Close##header")) {
        Close();
    }
}

void MyCustomWindow::RenderContent() {
    ImGui::TextUnformatted("Window content goes here");
    
    // Example: Simple combo
    const char* items[] = { "Option 1", "Option 2", "Option 3" };
    if (ImGui::BeginCombo("Select##combo", items[selectedIndex])) {
        for (int i = 0; i < 3; ++i) {
            if (ImGui::Selectable(items[i], i == selectedIndex)) {
                selectedIndex = i;
            }
        }
        ImGui::EndCombo();
    }
    
    // Example: Button action
    if (ImGui::Button("Do Something")) {
        someData = "Button was clicked!";
    }
    
    ImGui::TextUnformatted(("Status: " + someData).c_str());
}

void MyCustomWindow::RenderFooter() {
    ImGui::TextUnformatted("Footer text here");
}

void MyCustomWindow::Open() {
    isWindowOpen_ = true;
}

void MyCustomWindow::Close() {
    isWindowOpen_ = false;
}
```

### Step 3: Register Window in Plugin

**`SuiteSpot.h`**
```cpp
#pragma once
#include "MyCustomWindow.h"
#include <memory>

class SuiteSpot : public BakkesModPlugin, public SettingsWindowBase {
public:
    // ... existing code ...
    
    void onLoad() override;
    void onUnload() override;
    
private:
    // Your windows
    std::shared_ptr<MyCustomWindow> myCustomWindow;
};
```

**`SuiteSpot.cpp` in onLoad()**
```cpp
void SuiteSpot::onLoad() {
    LOG("SuiteSpot loaded");
    
    // ... existing initialization code ...
    
    // Create and register your custom window
    myCustomWindow = std::make_shared<MyCustomWindow>();
    
    // Now players can access it from the BakkesMod menu!
    // The window will appear as "My Custom Window" in the togglemenu
}
```

### Step 4: Provide Access to Window

Add a method to open the window from settings or commands:

**`Source.cpp` (in RenderSettings())**
```cpp
void SuiteSpot::RenderSettings() {
    ImGui::TextUnformatted("SuiteSpot - Auto Map Loader");
    ImGui::Separator();
    
    // ... existing settings ...
    
    // Button to open custom window
    if (ImGui::Button("Open My Custom Window", ImVec2(200, 0))) {
        myCustomWindow->Open();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click to open the custom window");
    }
}
```

Or via console command:

**`SuiteSpot.cpp` in onLoad()**
```cpp
cvarManager->registerCvar("ss_open_custom_window", "0", "Open custom window", 
    true, true, 0, true, 1)
    .addOnValueChanged([this](string oldValue, CVarWrapper cvar) {
        if (cvar.getBoolValue()) {
            myCustomWindow->Open();
            cvarManager->getCvar("ss_open_custom_window").setValue(0);
        }
    });
```

Then players can use: `ss_open_custom_window 1`

---

## How BakkesMod Backing Works

### The Flow

1. **BakkesMod's main loop** calls your window's `Render()` method
2. **PluginWindowBase::Render()** does:
   - Sets ImGui context (line 51): `ImGui::SetCurrentContext()`
   - Calls `ImGui::Begin()` (line 51)
   - Calls your `RenderWindow()` override
   - Calls `ImGui::End()` (line 60)

```cpp
void PluginWindowBase::Render() {
    if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }
    
    RenderWindow();  // YOUR CODE HERE
    
    ImGui::End();
}
```

3. **BakkesMod's renderer** then draws all queued ImGui commands to the screen

### Key Points

- ✅ ImGui context is managed by BakkesMod
- ✅ Window lifecycle is managed by BakkesMod
- ✅ Input handling is managed by BakkesMod
- ✅ Rendering is managed by BakkesMod
- ✅ You only implement `RenderWindow()`

---

## Complete Example: Training Pack Browser

Here's the real implementation from SuiteSpot:

**Header:**
```cpp
class TrainingStatsWindow : public PluginWindowBase {
public:
    TrainingStatsWindow(std::shared_ptr<TrainingStats> stats) 
        : trainingStats(stats), currentView(View::Dashboard) {}
    
    void RenderWindow() override;

private:
    enum class View {
        Dashboard,
        SessionSummary,
        DetailedStats,
        Analytics
    };
    
    std::shared_ptr<TrainingStats> trainingStats;
    View currentView;
    std::string selectedPackCode;
    
    void RenderDashboard();
    void RenderSessionSummary();
    void RenderDetailedStats();
    void RenderAnalytics();
};
```

**Implementation:**
```cpp
void TrainingStatsWindow::RenderWindow() {
    ImGui::TextUnformatted("TRAINING STATS DASHBOARD");
    ImGui::Separator();
    
    // View tabs
    if (ImGui::Button("Dashboard##view", ImVec2(120, 0))) {
        currentView = View::Dashboard;
    }
    ImGui::SameLine();
    if (ImGui::Button("Detailed Stats##view", ImVec2(120, 0))) {
        currentView = View::DetailedStats;
    }
    
    ImGui::Separator();
    
    // Render current view
    switch (currentView) {
        case View::Dashboard:
            RenderDashboard();
            break;
        case View::DetailedStats:
            RenderDetailedStats();
            break;
        // ...
    }
}
```

---

## Important Patterns

### 1. Window State Management

```cpp
// In header
class MyWindow : public PluginWindowBase {
private:
    bool isWindowOpen_ = false;  // Inherited from PluginWindowBase
    std::string data;
    int selectedItem = 0;
};

// When user clicks close button
if (ImGui::Button("Close")) {
    isWindowOpen_ = false;  // Window closes
}
```

### 2. Window Title

```cpp
// In constructor
menuTitle_ = "My Window Title";  // Shows in menu and title bar
```

### 3. Window Persistence

```cpp
// Window state is preserved between frames
// But resets when plugin unloads
class MyWindow : public PluginWindowBase {
    // This persists while plugin is loaded
    std::vector<std::string> data;
};
```

### 4. Multiple Windows

```cpp
// In SuiteSpot.h
std::shared_ptr<TrainingStatsWindow> trainingStatsWindow;
std::shared_ptr<MyCustomWindow> customWindow;
std::shared_ptr<AnotherWindow> anotherWindow;

// In onLoad()
trainingStatsWindow = std::make_shared<TrainingStatsWindow>(...);
customWindow = std::make_shared<MyCustomWindow>();
anotherWindow = std::make_shared<AnotherWindow>();

// Each window is independent but uses same ImGui context
```

---

## Common ImGui Patterns in Windows

### Tabs/Views Pattern (from TrainingStatsWindow)
```cpp
enum class View { Dashboard, Stats, Settings };
View currentView = View::Dashboard;

if (ImGui::Button("Dashboard##view")) currentView = View::Dashboard;
ImGui::SameLine();
if (ImGui::Button("Stats##view")) currentView = View::Stats;

switch (currentView) {
    case View::Dashboard: RenderDashboard(); break;
    case View::Stats: RenderStats(); break;
}
```

### Child Windows
```cpp
ImGui::BeginChild("ChildName", ImVec2(400, 300), true);
    // Content here
ImGui::EndChild();
```

### Tables/Lists
```cpp
ImGui::Columns(4);  // 4 columns
ImGui::SetColumnWidth(0, 200);
ImGui::SetColumnWidth(1, 100);

ImGui::TextUnformatted("Column 1");
ImGui::NextColumn();
ImGui::TextUnformatted("Column 2");
ImGui::NextColumn();
// ... more rows

ImGui::Columns(1);  // Reset to 1 column
```

### Input Handling
```cpp
static char buffer[256] = {0};

if (ImGui::InputText("Name##input", buffer, IM_ARRAYSIZE(buffer))) {
    std::string input(buffer);
    // Handle input change
}
```

---

## Build and Test

1. **Header:** Create class inheriting from `PluginWindowBase`
2. **Implementation:** Override `RenderWindow()`
3. **Registration:** Create in `onLoad()`, store as `std::shared_ptr`
4. **Access:** Provide button or command to open
5. **Build:** Normal SuiteSpot build
6. **Test:** Window appears in BakkesMod menu

---

## Key Differences from Regular ImGui

| Aspect | ImGui Standalone | BakkesMod Window |
|--------|-----------------|-----------------|
| Context Management | You manage | BakkesMod manages |
| Rendering | You call | BakkesMod calls |
| Window Begin/End | You call | PluginWindowBase handles |
| Input Handling | You handle | BakkesMod handles |
| ImGui State | You manage | Preserved by BakkesMod |
| Drawing Backend | You provide | BakkesMod provides |

---

## Advantages of BakkesMod Backing

✅ **Consistent Styling** - Matches BakkesMod UI theme  
✅ **Consistent Input** - Works with BakkesMod input system  
✅ **Performance** - Optimized rendering pipeline  
✅ **Integration** - Seamless with plugin system  
✅ **Persistence** - Window state managed automatically  
✅ **Accessibility** - Window appears in BakkesMod menu  

---

## Troubleshooting

### Window doesn't appear in menu
```cpp
// Make sure you created it in onLoad()
myWindow = std::make_shared<MyWindow>();

// And set the title
myWindow->menuTitle_ = "My Window";
```

### ImGui context errors
```cpp
// Don't call ImGui::SetCurrentContext() yourself
// PluginWindowBase::Render() handles it

// If you need context elsewhere:
void MyWindow::RenderWindow() {
    // ImGui context is already set here
    ImGui::TextUnformatted("Safe to use ImGui");
}
```

### Window closes unexpectedly
```cpp
// Check that close button sets isWindowOpen_ = false
if (ImGui::Button("Close")) {
    isWindowOpen_ = false;  // This closes the window
    // But doesn't destroy the window object
}

// Window can be reopened
myWindow->isWindowOpen_ = true;
```

### Data loss between frames
```cpp
// Store persistent data as member variables
class MyWindow : public PluginWindowBase {
private:
    std::string persistentData;  // Stays between frames
    std::vector<int> history;    // Accumulated data
};
```

---

## Summary

To create an ImGui window in BakkesMod:

1. **Inherit** from `PluginWindowBase`
2. **Override** `RenderWindow()`
3. **Set** `menuTitle_` in constructor
4. **Create** with `std::make_shared` in `onLoad()`
5. **Use** ImGui functions in `RenderWindow()`
6. **BakkesMod** handles the rest

The BakkesMod ImGui context is the backing system that provides all the infrastructure.
