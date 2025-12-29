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

// Menu state tracking (RocketStats pattern)
struct MenuState {
    bool is_in_menu = false;       // Player is in any menu
    bool is_in_MainMenu = true;    // Player is at main menu
    bool is_in_game = false;       // Player is in a match
    bool is_in_freeplay = false;   // Player is in freeplay
    bool is_in_scoreboard = false; // Scoreboard is open
    bool is_online_game = false;   // Online match
    bool is_offline_game = false;  // Offline match (bots/freeplay)
    int menu_stack = 0;            // Menu depth counter
};

// NOTE: inherit from SettingsWindowBase AND PluginWindow for overlay rendering
class SuiteSpot final : public BakkesMod::Plugin::BakkesModPlugin,
                        public SettingsWindowBase,
                        public BakkesMod::Plugin::PluginWindow
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

    // settings UI (PluginSettingsWindow)
    void RenderSettings() override;

    // PluginWindow interface for overlay rendering
    void Render() override;
    std::string GetMenuName() override { return "suitespot_overlay"; }
    std::string GetMenuTitle() override { return "SuiteSpot Overlay"; }
    void SetImGuiContext(uintptr_t ctx) override;
    bool ShouldBlockInput() override {
        return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    }
    bool IsActiveOverlay() override {
        return testOverlayActive || postMatch.active;
    }
    void OnOpen() override {}
    void OnClose() override {}

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
    MenuState menuState;
    bool testOverlayActive = false;  // Bypass menu state checks for testing

    // Menu state accessors (RocketStats pattern)
    bool IsInMenu() const { return menuState.is_in_menu; }
    bool IsInMainMenu() const { return menuState.is_in_MainMenu; }
    bool IsInGame() const { return menuState.is_in_game; }
    bool IsInFreeplay() const { return menuState.is_in_freeplay; }
    bool IsInScoreboard() const { return menuState.is_in_scoreboard; }

    // Overlay visibility settings
    bool ShouldShowOverlay() const;
    bool IsOverlayEnabledInMenu() const;
    bool IsOverlayEnabledInGame() const;
    bool IsOverlayEnabledInScoreboard() const;

private:
    // Menu state event handlers (RocketStats pattern)
    void OnEnteredMainMenu();
    void OnPushMenu();
    void OnPopMenu();
    void OnGameStart();
    void OnOpenScoreboard();
    void OnCloseScoreboard();
    // Windows

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