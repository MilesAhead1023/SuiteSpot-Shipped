#pragma once
#include "OverlayTypes.h"
#include "OverlayUtils.h"
#include "IMGUI/json.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <map>

using json = nlohmann::json;

struct ThemeFont {
    int size = 0;
    std::string name = "";
    bool isDefault = false;
};

struct Theme {
    std::string name = "Default";
    std::string author = "Unknown";
    std::string version = "1.0";
    std::vector<ThemeFont> fonts;
    std::vector<RSElement> elements; // Pre-calculated elements for rendering
    json config; // Raw config for dynamic recalculation
};

class SuiteSpot;

class ThemeManager {
public:
    explicit ThemeManager(SuiteSpot* plugin);

    void LoadThemes();
    bool ChangeTheme(const std::string& themeName);
    bool ChangeTheme(int themeIndex);

    // Dual-theme support (RocketStats pattern)
    void SwitchToMenuTheme();
    void SwitchToGameTheme();
    void SetMenuThemeIndex(int index) { menuThemeIndex = index; }
    void SetGameThemeIndex(int index) { gameThemeIndex = index; }
    int GetMenuThemeIndex() const { return menuThemeIndex; }
    int GetGameThemeIndex() const { return gameThemeIndex; }
    bool IsDualThemeEnabled() const { return dualThemeEnabled; }
    void SetDualThemeEnabled(bool enabled) { dualThemeEnabled = enabled; }

    // Called every frame or when data changes to update element positions/values
    void UpdateThemeElements(const RSOptions& options, std::map<std::string, std::string>& vars, const struct PostMatchInfo& pm);

    Theme& GetActiveTheme() { return activeTheme; }
    const std::vector<Theme>& GetThemes() const { return themes; }
    std::vector<std::string> GetThemeNames() const;
    int GetActiveThemeIndex() const { return activeThemeIndex; }

    // Refresh trigger (RocketStats pattern)
    void SetRefresh(bool full = false) { needsRefresh = true; needsFullRefresh = full; }
    bool NeedsRefresh() const { return needsRefresh; }

private:
    SuiteSpot* plugin_;
    std::vector<Theme> themes;
    Theme activeTheme;
    int activeThemeIndex = 0;

    // Dual-theme state (RocketStats pattern)
    int menuThemeIndex = 0;
    int gameThemeIndex = 0;
    bool dualThemeEnabled = false;

    // Refresh state
    bool needsRefresh = false;
    bool needsFullRefresh = false;
    
    // Caches loaded images to prevent disk spam
    std::map<std::string, std::shared_ptr<ImageWrapper>> imageCache;

    // Helper to calculate a single element from JSON
    RSElement CalculateElement(const json& config, const RSOptions& options, std::map<std::string, std::string>& vars);
    
    // Helper to load a specific font
    void LoadFont(const std::string& themeName, const std::string& fontFile, int size);
};
