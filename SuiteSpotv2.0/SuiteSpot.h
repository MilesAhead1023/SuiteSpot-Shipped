#pragma once
#include "GuiBase.h" // defines SettingsWindowBase
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "MapList.h"
#include "LoadoutManager.h"
#include "version.h"
#include "ThemeManager.h"
#include <filesystem>
#include <set>
#include <memory>

// Forward declarations for additional windows
class SuiteSpotSettingsWindow2;
class SuiteSpotTestWindow;
class PostMatchOverlayWindow;
class MapManager;
class SettingsSync;
class AutoLoadFeature;
class TrainingPackManager;
class SettingsUI;
class TrainingPackUI;
class LoadoutUI;
class OverlayRenderer;
// ThemeManager is now included directly

// Version macro carried over from the master template
constexpr auto plugin_version =
    stringify(VERSION_MAJOR) "."
    stringify(VERSION_MINOR) "."
    stringify(VERSION_PATCH) "."
    stringify(VERSION_BUILD);

struct PostMatchPlayerRow {
    int teamIndex = -1;
    bool isLocal = false;
    std::string name;
    int score = 0;
    int goals = 0;
    int assists = 0;
    int saves = 0;
    int shots = 0;
    int ping = 0;
    bool isMVP = false;
};

struct PostMatchInfo {
    bool active = false;
    std::chrono::steady_clock::time_point start;
    int myScore = 0;
    int oppScore = 0;
    std::string myTeamName;
    std::string oppTeamName;
    std::string playlist;
    bool overtime = false;
    LinearColor myColor{};
    LinearColor oppColor{};
    std::vector<PostMatchPlayerRow> players;
};

struct SessionStats {
    int matchesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int goals = 0;
    int assists = 0;
    int saves = 0;
    int shots = 0;
    int mvps = 0;
};

// NOTE: inherit from SettingsWindowBase (not “GuiBase”)
class SuiteSpot final : public BakkesMod::Plugin::BakkesModPlugin,
                        public SettingsWindowBase
{
    friend class SettingsUI;
    friend class TrainingPackUI;
    friend class LoadoutUI;
    friend class OverlayRenderer;
public:
    // Persistence folders and files under %APPDATA%\bakkesmod\bakkesmod\data
    void EnsureDataDirectories() const;
    std::filesystem::path GetDataRoot() const;
    std::filesystem::path GetSuiteTrainingDir() const;

    // Workshop persistence API
    void LoadWorkshopMaps();
    void SaveWorkshopMaps() const; // no-op (legacy)
    void DiscoverWorkshopInDir(const std::filesystem::path& dir);
    std::filesystem::path GetWorkshopLoaderConfigPath() const;
    std::filesystem::path ResolveConfiguredWorkshopRoot() const;

    // lifecycle
    void onLoad() override;
    void onUnload() override;

    // settings UI
    void RenderSettings() override;
    void SetImGuiContext(uintptr_t ctx) override;

    // hooks
    void LoadHooks();
    void GameEndedEvent(std::string name);

    // Training Pack scraper integration
    std::filesystem::path GetTrainingPacksPath() const;
    void ScrapeAndLoadTrainingPacks();
    void LoadTrainingPacksFromFile(const std::filesystem::path& filePath);
    bool IsTrainingPackCacheStale() const;
    std::string FormatLastUpdatedTime() const;
    
    // Post-match overlay rendering
    void RenderPostMatchOverlay();
    void ToggleTestOverlay();
    
    PostMatchInfo& GetPostMatchInfo() { return postMatch; }
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
    float GetPostMatchDurationSec() const;
    float GetOverlayWidth() const;
    float GetOverlayHeight() const;
    float GetOverlayAlpha() const;
    float GetBlueTeamHue() const;
    float GetOrangeTeamHue() const;
    float GetOverlayOffsetX() const;
    float GetOverlayOffsetY() const;
    int GetOverlayMode() const;
    OverlayRenderer* GetOverlayRenderer() const { return overlayRenderer; }
    ThemeManager* GetThemeManager() const { return themeManager; }

    PostMatchInfo postMatch;
    SessionStats sessionStats;

private:
    // Windows
    std::shared_ptr<PostMatchOverlayWindow> postMatchOverlayWindow;

    std::string lastGameMode = "";

    // Shuffle helpers (delegates to TrainingPackManager)
    int GetRandomTrainingIndex() const;

    ImGuiContext* imguiCtx = nullptr;
    
    // Loadout management
    std::unique_ptr<LoadoutManager> loadoutManager;

    MapManager* mapManager = nullptr;
    SettingsSync* settingsSync = nullptr;
    AutoLoadFeature* autoLoadFeature = nullptr;
    TrainingPackManager* trainingPackMgr = nullptr;
    SettingsUI* settingsUI = nullptr;
    TrainingPackUI* trainingPackUI = nullptr;
    LoadoutUI* loadoutUI = nullptr;
    OverlayRenderer* overlayRenderer = nullptr;
    ThemeManager* themeManager = nullptr;
};

class PostMatchOverlayWindow : public PluginWindowBase {
public:
    PostMatchOverlayWindow(SuiteSpot* plugin);
    void Render() override;
    void RenderWindow() override;
    void Open();
    void Close();
    
    std::string GetMenuName() override { return "SuiteSpotPostMatchOverlay"; }
    std::string GetMenuTitle() override { return "SuiteSpot Post-Match Overlay"; }
    bool IsActiveOverlay() override { return true; }
    bool ShouldBlockInput() override { return false; }

private:
    SuiteSpot* plugin_;
};