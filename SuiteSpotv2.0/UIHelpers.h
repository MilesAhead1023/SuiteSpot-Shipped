#pragma once

// UIHelpers.h
//
// This file contains "helper functions" that make the UI code cleaner.
// Instead of writing the same 5-10 lines of code over and over for every button or input box,
// we can call one of these helpers that does all the work in one line.
//
// Think of it like having pre-made "templates" for common UI elements:
// - Input boxes that automatically validate numbers and save settings
// - Buttons that show helpful tooltips when you hover
// - Dropdown menus with tooltips
// - Status messages that fade out automatically
//
// This makes the code easier to read, maintain, and less error-prone.

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "IMGUI/imgui.h"
#include <string>

namespace UI {
namespace Helpers {

	//
	// SetCVarSafely - Safely saves a setting value to the plugin
	//
	// This is a utility function that saves values to BakkesMod's settings system (CVars).
	// It includes safety checks to prevent crashes if the settings system isn't available.
	//
	// Think of CVars like a settings file - when you change a value in the UI, this saves it
	// so it persists when you restart Rocket League.
	//
	// Parameters:
	//   cvarName - The internal name of the setting (e.g., "suitespot_delay_queue_sec")
	//   value - The value to save (can be int, bool, float, string, etc.)
	//   cvarManager - The plugin's settings manager (handles the actual saving)
	//
	template<typename T>
	void SetCVarSafely(const char* cvarName, const T& value, std::shared_ptr<CVarManagerWrapper> cvarManager) {
		// Safety check: Make sure the settings manager exists before trying to use it
		if (!cvarManager) return;

		// Get the specific setting we want to update
		auto cvar = cvarManager->getCvar(cvarName);

		// Safety check: Make sure this setting actually exists
		if (cvar) {
			// Save the new value to the setting
			cvar.setValue(value);
		}
	}

	//
	// InputIntWithRange - Creates a "smart" number input box
	//
	// This creates an input box that:
	// 1. Only allows numbers within a specific range (e.g., 0-300)
	// 2. Automatically saves the value to your plugin settings
	// 3. Shows a helpful tooltip when you hover over it
	// 4. Displays the valid range next to the input (e.g., "0-300s")
	//
	// Example: The "Delay Queue (sec)" input that only allows 0-300 seconds
	//
	// Parameters:
	//   label - What to display next to the input box (e.g., "Delay Queue (sec)")
	//   value - The number being edited (this gets updated when user types)
	//   minValue - Smallest allowed number (e.g., 0 - won't let you type -50)
	//   maxValue - Largest allowed number (e.g., 300 - won't let you type 9999)
	//   width - How wide the input box should be in pixels (0 = use default width)
	//   cvarName - Internal setting name to save to (e.g., "suitespot_delay_queue_sec")
	//   cvarManager - Plugin's settings manager (handles the actual saving)
	//   tooltip - Help text when hovering (optional - use nullptr for no tooltip)
	//   rangeHint - Range display like "0-300s" shown next to input (optional)
	//
	// Returns: true if the input box was displayed
	//
	bool InputIntWithRange(
		const char* label,
		int& value,
		int minValue,
		int maxValue,
		float width,
		const char* cvarName,
		std::shared_ptr<CVarManagerWrapper> cvarManager,
		const char* tooltip = nullptr,
		const char* rangeHint = nullptr
	);

	//
	// ComboWithTooltip - Creates a dropdown menu with automatic tooltip
	//
	// This creates a dropdown (combo box) that automatically shows a helpful tooltip
	// when you hover over it. Saves you from having to write the tooltip code separately.
	//
	// Example: The "Freeplay Maps" dropdown that shows "Select which stadium to load after matches"
	//
	// Parameters:
	//   label - Dropdown label (what's shown on the UI)
	//   previewValue - Text shown when dropdown is closed (the currently selected item)
	//   tooltip - Help text displayed when hovering over the dropdown
	//   width - How wide the dropdown should be in pixels (0 = use default width)
	//
	// Returns: true if dropdown is open (caller must still call ImGui::EndCombo() after adding items)
	//
	// Usage:
	//   if (ComboWithTooltip("Maps", selectedMapName, "Choose a map", 260.0f)) {
	//       // Add your dropdown items here using ImGui::Selectable()
	//       ImGui::EndCombo();
	//   }
	//
	bool ComboWithTooltip(
		const char* label,
		const char* previewValue,
		const char* tooltip,
		float width = 0.0f
	);

	//
	// ButtonWithTooltip - Creates a button with automatic tooltip
	//
	// Creates a button that automatically shows a helpful tooltip when you hover over it.
	// Much cleaner than writing the button and tooltip code separately every time.
	//
	// Example: The "Load Now" button with tooltip "Load this pack immediately"
	//
	// Parameters:
	//   label - Text displayed on the button
	//   tooltip - Help text shown when hovering over the button
	//   size - Button size in pixels (0,0 = auto-size to fit the text)
	//
	// Returns: true if the button was clicked
	//
	// Usage:
	//   if (ButtonWithTooltip("Load Now", "Load this pack immediately")) {
	//       // Do something when button is clicked
	//   }
	//
	bool ButtonWithTooltip(
		const char* label,
		const char* tooltip,
		const ImVec2& size = ImVec2(0, 0)
	);

	//
	// ShowStatusMessage - Displays a message that automatically fades out
	//
	// Shows a colored status message that automatically disappears after a few seconds.
	// Like when you add a training pack and see "Pack added!" that fades away.
	//
	// This handles all the timing logic - you just set the message, color, and duration,
	// then call this every frame and it handles the countdown and hiding automatically.
	//
	// Example: "Loadouts refreshed" message that shows for 2.5 seconds then disappears
	//
	// Parameters:
	//   text - The message to display
	//   color - Text color (e.g., green for success, red for error)
	//   timer - Reference to a timer variable (decrements each frame until 0)
	//   deltaTime - Time since last frame (usually ImGui::GetIO().DeltaTime)
	//
	// Usage:
	//   // When something happens, set the message:
	//   statusMessage = "Pack added!";
	//   statusColor = green;
	//   statusTimer = 3.0f;  // Show for 3 seconds
	//
	//   // Every frame in your render loop:
	//   ShowStatusMessage(statusMessage, statusColor, statusTimer, ImGui::GetIO().DeltaTime);
	//
	void ShowStatusMessage(
		const std::string& text,
		const ImVec4& color,
		float& timer,
		float deltaTime
	);

	//
	// ShowStatusMessageWithFade - Status message with smooth alpha fade-out
	//
	// Like ShowStatusMessage, but the text smoothly fades to transparent as the timer runs out.
	// Creates a nicer visual effect than just disappearing.
	//
	// Example: "Pack added!" starts fully visible, then gradually becomes transparent
	//
	// Parameters:
	//   text - The message to display
	//   baseColor - Base text color (alpha will be calculated based on remaining time)
	//   timer - Reference to a timer variable (decrements each frame until 0)
	//   maxDuration - Total duration in seconds (used to calculate fade - timer/maxDuration = alpha)
	//   deltaTime - Time since last frame (usually ImGui::GetIO().DeltaTime)
	//
	// Usage:
	//   // When something happens:
	//   statusMessage = "Pack added!";
	//   statusTimer = 3.0f;
	//
	//   // Every frame:
	//   ShowStatusMessageWithFade(statusMessage, greenColor, statusTimer, 3.0f, ImGui::GetIO().DeltaTime);
	//
	void ShowStatusMessageWithFade(
		const std::string& text,
		const ImVec4& baseColor,
		float& timer,
		float maxDuration,
		float deltaTime
	);

	//
	// CheckboxWithCVar - Creates a checkbox that automatically saves to settings
	//
	// Creates a checkbox that automatically saves its value to the plugin settings when toggled.
	// Combines the checkbox with the save logic so you don't have to write it separately.
	//
	// Example: "Enable SuiteSpot" checkbox that saves to "suitespot_enabled" setting
	//
	// Parameters:
	//   label - Text displayed next to the checkbox
	//   value - The bool variable being toggled (updated when checkbox is clicked)
	//   cvarName - Internal setting name to save to (e.g., "suitespot_enabled")
	//   cvarManager - Plugin's settings manager (handles the actual saving)
	//   tooltip - Help text when hovering (optional - use nullptr for no tooltip)
	//
	// Returns: true if the checkbox was toggled
	//
	// Usage:
	//   if (CheckboxWithCVar("Enable SuiteSpot", enabledValue, "suitespot_enabled",
	//                        cvarManager, "Enable/disable all features")) {
	//       // Checkbox was toggled, value and setting are already updated
	//   }
	//
	bool CheckboxWithCVar(
		const char* label,
		bool& value,
		const char* cvarName,
		std::shared_ptr<CVarManagerWrapper> cvarManager,
		const char* tooltip = nullptr
	);

	//
	// InputTextWithTooltip - Creates a text input box with automatic tooltip
	//
	// Creates a text input field that automatically shows a tooltip when you hover over it.
	// Cleaner than writing the input and tooltip code separately.
	//
	// Example: "Training Map Code" input with tooltip "Enter the code (e.g., 555F-7503-BBB9-E1E3)"
	//
	// Parameters:
	//   label - What to display next to the input box
	//   buf - Character buffer for the text being edited
	//   bufSize - Size of the buffer (how many characters it can hold)
	//   tooltip - Help text displayed when hovering
	//   width - How wide the input box should be in pixels (0 = use default width)
	//   flags - Special input options (optional - see ImGui docs)
	//
	// Returns: true if the text was modified
	//
	// Usage:
	//   char mapCode[64] = "";
	//   if (InputTextWithTooltip("Code", mapCode, sizeof(mapCode),
	//                            "Enter pack code", 220.0f)) {
	//       // Text was changed
	//   }
	//
	bool InputTextWithTooltip(
		const char* label,
		char* buf,
		size_t bufSize,
		const char* tooltip,
		float width = 0.0f,
		ImGuiInputTextFlags flags = 0
	);

} // namespace Helpers
} // namespace UI
