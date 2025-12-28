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
    
    // Called every frame or when data changes to update element positions/values
    void UpdateThemeElements(const RSOptions& options, std::map<std::string, std::string>& vars, const struct PostMatchInfo& pm);

    Theme& GetActiveTheme() { return activeTheme; }
    std::vector<std::string> GetThemeNames() const;

private:
    SuiteSpot* plugin_;
    std::vector<Theme> themes;
    Theme activeTheme;
    
    // Caches loaded images to prevent disk spam
    std::map<std::string, std::shared_ptr<ImageWrapper>> imageCache;

    // Helper to calculate a single element from JSON
    RSElement CalculateElement(const json& config, const RSOptions& options, std::map<std::string, std::string>& vars);
    
    // Helper to load a specific font
    void LoadFont(const std::string& themeName, const std::string& fontFile, int size);
};
