#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "MapList.h"
#include "LoadoutManager.h"
#include "version.h"
#include <filesystem>
#include <set>
#include <memory>

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
                        public SettingsWindowBase
{
    friend class SettingsUI;
    friend class TrainingPackUI;
    friend class LoadoutUI;
public:
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

    // Training Pack scraper integration
    std::filesystem::path GetTrainingPacksPath() const;
    void ScrapeAndLoadTrainingPacks();
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
    TrainingPackUI* trainingPackUI = nullptr;
    LoadoutUI* loadoutUI = nullptr;
};