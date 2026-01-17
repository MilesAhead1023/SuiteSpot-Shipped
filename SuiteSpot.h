#pragma once

/*
 * ======================================================================================
 * SUITESPOT: PLUGIN ARCHITECTURE & MAIN HUB
 * ======================================================================================
 * 
 * WHAT IS THIS?
 * This is the "Brain" of the SuiteSpot plugin. The `SuiteSpot` class is the central
 * hub that connects BakkesMod (the game) to all our custom features.
 * 
 * WHY IS IT HERE?
 * BakkesMod requires one main class that inherits from `BakkesModPlugin` to start
 * everything up. This file tells the game "I am a plugin, here is how to load me."
 * 
 * HOW DOES IT WORK?
 * 1. LIFECYCLE:
 *    - `onLoad()`: Called when Rocket League starts. We create all our tools (Managers, UI) here.
 *    - `onUnload()`: Called when the game closes. We clean up memory here.
 * 
 * 2. THE BRIDGE:
 *    - It holds pointers to "Managers" (MapManager, TrainingPackManager) which handle data.
 *    - It holds pointers to "UI" (SettingsUI, TrainingPackUI) which draw the menus.
 *    - It listens for game events (like "Match Ended") and tells the `AutoLoadFeature` to react.
 * 
 * 3. WINDOW MANAGEMENT (Advanced):
 *    - This class is special because it acts as BOTH the "Settings Tab" (inside F2 menu)
 *      AND the "Window Manager" for our pop-up browser.
 *    - It uses a "Hybrid Rendering" trick to keep the browser window open even when F2 is closed.
 *      (See `docs/development/thread-safe-imgui.md` for the technical details).
 */

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "MapList.h"
#include "LoadoutManager.h"
#include "version.h"
#include <filesystem>
#include <set>
#include <memory>

#include <atomic>

class SettingsWindowBase : public BakkesMod::Plugin::PluginSettingsWindow
{
public:
    virtual ~SettingsWindowBase() = default;
    std::string GetPluginName() override { return "SuiteSpot"; }
    void SetImGuiContext(uintptr_t ctx) override {
        ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
    }
};

// Forward declarations
class MapManager;
class SettingsSync;
class AutoLoadFeature;
class TrainingPackManager;
class SettingsUI;
class TrainingPackUI;
class LoadoutUI;

// Version macro carried over from the master template
constexpr auto plugin_version =
    stringify(VERSION_MAJOR) "."
    stringify(VERSION_MINOR) "."
    stringify(VERSION_PATCH) "."
    stringify(VERSION_BUILD);

class SuiteSpot final : public BakkesMod::Plugin::BakkesModPlugin,
                        public SettingsWindowBase,
                        public BakkesMod::Plugin::PluginWindow
{
    friend class SettingsUI;
    friend class TrainingPackUI;
    friend class LoadoutUI;
public:
    // PluginWindow implementation
    void Render() override;
    std::string GetMenuName() override;
    std::string GetMenuTitle() override;
    void SetImGuiContext(uintptr_t ctx) override;
    bool ShouldBlockInput() override;
    bool IsActiveOverlay() override;
    void OnOpen() override;
    void OnClose() override;
    // Persistence folders and files under %APPDATA%\bakkesmod\bakkesmod\data
    void EnsureDataDirectories() const;
    std::filesystem::path GetDataRoot() const;
    std::filesystem::path GetSuiteTrainingDir() const;

    // Workshop persistence API
    void LoadWorkshopMaps();
    void DiscoverWorkshopInDir(const std::filesystem::path& dir);
    std::filesystem::path GetWorkshopLoaderConfigPath() const;
    std::filesystem::path ResolveConfiguredWorkshopRoot() const;

    // lifecycle
    void onLoad() override;
    void onUnload() override;

    // settings UI (PluginSettingsWindow)
    void RenderSettings() override;

    // hooks
    void LoadHooks();
    void GameEndedEvent(std::string name);

    // Training Pack update integration
    std::filesystem::path GetTrainingPacksPath() const;
    void UpdateTrainingPackList();
    void LoadTrainingPacksFromFile(const std::filesystem::path& filePath);
    bool IsTrainingPackCacheStale() const;
    std::string FormatLastUpdatedTime() const;
    
    bool IsEnabled() const;
    bool IsAutoQueueEnabled() const;
    bool IsTrainingShuffleEnabled() const;
    int GetMapType() const;
    int GetDelayQueueSec() const;
    int GetDelayFreeplaySec() const;
    int GetDelayTrainingSec() const;
    int GetDelayWorkshopSec() const;
    int GetCurrentIndex() const;
    int GetCurrentTrainingIndex() const;
    int GetCurrentWorkshopIndex() const;
    int GetTrainingBagSize() const;

private:
    // Loadout management
    std::unique_ptr<LoadoutManager> loadoutManager;

    MapManager* mapManager = nullptr;
    SettingsSync* settingsSync = nullptr;
    AutoLoadFeature* autoLoadFeature = nullptr;
    TrainingPackManager* trainingPackMgr = nullptr;
    SettingsUI* settingsUI = nullptr;
    std::shared_ptr<TrainingPackUI> trainingPackUI = nullptr;
    LoadoutUI* loadoutUI = nullptr;

    bool isBrowserOpen = false;
    uintptr_t imgui_ctx = 0;
    std::atomic<bool> isRenderingSettings{false};
};