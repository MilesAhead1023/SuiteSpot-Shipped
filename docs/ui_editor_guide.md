# SuiteSpot UI Editor Guide (ImGui 1.75)

This guide is for building SuiteSpot UI without writing code from scratch.
It combines a visual editor (ImStudio) and in-game ImGui tools.

## In-game tools (dev-only)

These tools are hidden unless you enable them with CVars:

```
// Enable dev tools window
suitespot_ui_editor 1

// Optional toggles (can also be switched in the UI tools window)
suitespot_ui_show_demo 1
suitespot_ui_show_metrics 1
suitespot_ui_show_style 1
```

When enabled, a "SuiteSpot UI Tools" window appears in the settings UI.
Use it to open:
- ImGui Demo Window (widget catalog)
- ImGui Metrics Window (inspect layout and draw calls)
- ImGui Style Editor (theme/colors/spacing)

The window also includes:
- Widget Palette: click a button to copy a ready-made snippet to clipboard.
- ImGui 1.75 Validator: paste generated code and scan for known incompatibilities.
- Export Snippets: saves a bundle of snippets to a file under SuiteSpot's data folder.
- Widget Search: filter the palette by name.
- Widget Preview: click Preview to see a live widget example.
- Categories: filter snippets by Buttons, Inputs, Layout, or Advanced.
- Copy+Preview: clicking a snippet copies it and opens its live preview.
- JSON UI (Experimental): render a UI from `ui_layout.json` without recompiling.
- Lua UI (Experimental): render UI + advanced logic from `ui_layout.lua` without recompiling.
- Lua Snippet Builder: generate Lua code blocks with buttons, copy/save them.

## JSON UI (Experimental)

You can define simple UI in a JSON file and render it live in-game.
This is a no-code way to build menus using safe ImGui 1.75 widgets.

Default path:

```
%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\Dev\ui_layout.json
```

How to use:
1. In the UI Tools window, click "Write Sample JSON".
2. Click "Load/Reload JSON".
3. Enable "Render JSON UI".
4. Edit the JSON file and reload to see changes.

Supported widget types (initial set):
- text, text_wrapped, text_colored
- separator, spacing, same_line, new_line, indent, unindent
- button (with `action.command`)
- checkbox, slider_int, slider_float
- input_text, input_text_multiline
- combo, listbox

Binding to CVars:
- Use `bind_cvar` to connect widgets to plugin settings.
- Example: `"bind_cvar": "suitespot_enabled"`

Conditional visibility:
- Use `visible_if_cvar` with `name` + `equals` or `is_true`.

## Lua scripting (experimental)

Lua scripting is now available using vcpkg's Lua package.

Prerequisite:
- Install Lua via vcpkg: `vcpkg install lua:x64-windows`

Default path:

```
%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\Dev\ui_layout.lua
```

How to use:
1. In the UI Tools window, click "Write Sample Lua".
2. Click "Load/Reload Lua".
3. Enable "Render Lua UI".
4. Edit the Lua file and it will auto-reload (if enabled).

Lua API (initial set):
- ui.text, ui.text_wrapped, ui.text_colored
- ui.button, ui.checkbox, ui.slider_int, ui.slider_float
- ui.input_text, ui.input_text_multiline
- ui.combo, ui.listbox
- ui.separator, ui.spacing, ui.same_line, ui.new_line
- ui.indent, ui.unindent
- ui.begin_window / ui.end_window
- ui.begin_child / ui.end_child
- ui.begin_tab_bar / ui.end_tab_bar
- ui.begin_tab_item / ui.end_tab_item
- ui.tooltip
- ui.image (ImTextureID as integer), ui.draw_line, ui.draw_rect, ui.draw_rect_filled
- ui.begin_drag_drop_source / ui.set_drag_drop_payload / ui.end_drag_drop_source
- ui.begin_drag_drop_target / ui.accept_drag_drop_payload / ui.end_drag_drop_target

Commands and CVars:
- cmd.exec("togglemenu suitespot_browser")
- cvar.get / cvar.get_int / cvar.get_float / cvar.get_bool
- cvar.set("suitespot_enabled", true)

## Visual editor: ImStudio (recommended)

ImStudio is a live ImGui editor that generates C++ ImGui code.
It does not need to match your exact ImGui version to be useful, but
you must keep the output compatible with ImGui 1.75.

Workflow:
1. Design a window in ImStudio using only "safe" widgets (see below).
2. Copy the generated ImGui code.
3. Paste into the appropriate UI file:
   - `SettingsUI.cpp` (main settings window)
   - `TrainingPackUI.cpp` (browser window)
   - `LoadoutUI.cpp` (loadout controls)
4. Build and test in-game.

If the generated code uses APIs newer than 1.75, replace them or remove
the unsupported parts.

## Safe widget checklist (ImGui 1.75)

These are confirmed available in `IMGUI/imgui.h` for 1.75:
- Text: `Text`, `TextWrapped`, `TextColored`, `TextDisabled`
- Buttons: `Button`, `SmallButton`, `ArrowButton`
- Checkboxes / radios: `Checkbox`, `RadioButton`
- Inputs: `InputText`, `InputTextMultiline`, `InputInt`, `InputFloat`
- Drags: `DragInt`, `DragFloat`
- Sliders: `SliderInt`, `SliderFloat`
- Combos / list: `BeginCombo`/`EndCombo`, `ListBox`
- Color: `ColorEdit3/4`, `ColorPicker3/4`
- Layout: `Spacing`, `SameLine`, `Separator`, `Indent`/`Unindent`
- Groups: `BeginGroup`/`EndGroup`
- Child windows: `BeginChild`/`EndChild`
- Collapsing: `CollapsingHeader`, `TreeNode`/`TreePop`
- Tabs: `BeginTabBar`/`EndTabBar`, `BeginTabItem`/`EndTabItem`
- Popups: `OpenPopup`, `BeginPopup`, `BeginPopupModal`, `EndPopup`
- Menus: `BeginMenuBar`, `BeginMenu`, `MenuItem`, `EndMenuBar`
- Tooltips: `BeginTooltip`/`EndTooltip`, `SetTooltip`
- Images: `Image`, `ImageButton` (if you have textures set up)

## Not available in ImGui 1.75 (avoid or replace)

- Tables: `BeginTable`, `TableSetupColumn`, `TableNextRow`, etc.
  Use `Columns()` (legacy) instead.
- Docking / DockSpace / Multi-Viewport: not in 1.75.
- New input helpers such as `ImGui::Shortcut`.

If ImStudio outputs anything above, remove it or convert it to a supported
pattern before compiling.

## Tip: use the built-in Demo code

The full demo implementation lives in `IMGUI/imgui_demo.cpp`.
When you see a widget you like in the Demo Window, open that file and
copy the exact pattern from there into your plugin UI.
