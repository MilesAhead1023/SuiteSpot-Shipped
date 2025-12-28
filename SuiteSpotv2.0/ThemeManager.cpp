#include "pch.h"
#include "ThemeManager.h"
#include "SuiteSpot.h"
#include "OverlayUtils.h"
#include <fstream>

namespace fs = std::filesystem;

ThemeManager::ThemeManager(SuiteSpot* plugin) : plugin_(plugin) {}

void ThemeManager::LoadThemes() {
    themes.clear();
    fs::path themeDir = plugin_->GetDataRoot() / "SuiteSpot" / "themes";
    
    if (!fs::exists(themeDir)) {
        fs::create_directories(themeDir);
    }

    // Always create a default theme if none exist
    bool defaultFound = false;
    for (const auto& entry : fs::directory_iterator(themeDir)) {
        if (entry.is_directory() && fs::exists(entry.path() / "config.json")) {
            Theme theme;
            theme.name = entry.path().filename().string();
            
            // Read config.json
            try {
                std::ifstream i(entry.path() / "config.json");
                i >> theme.config;
                
                if (theme.config.contains("author")) theme.author = theme.config["author"];
                if (theme.config.contains("version")) theme.version = theme.config["version"];
                
                themes.push_back(theme);
                if (theme.name == "Default") defaultFound = true;
            } catch (const std::exception& e) {
                LOG("SuiteSpot: Error loading theme {}: {}", theme.name, e.what());
            }
        }
    }

    if (!defaultFound) {
        // TODO: Create default theme file
    }
    
    if (!themes.empty()) {
        ChangeTheme(themes[0].name);
    }
}

bool ThemeManager::ChangeTheme(const std::string& themeName) {
    for (const auto& t : themes) {
        if (t.name == themeName) {
            activeTheme = t;
            // TODO: Load fonts
            return true;
        }
    }
    return false;
}

void ThemeManager::UpdateThemeElements(const RSOptions& options, std::map<std::string, std::string>& vars, const PostMatchInfo& pm) {
    activeTheme.elements.clear();
    
    if (activeTheme.config.contains("elements")) {
        for (const auto& elConfig : activeTheme.config["elements"]) {
            // Check for repeat mode
            std::string repeat = elConfig.value("repeat", "");
            
            if (repeat == "players") {
                std::string teamFilter = elConfig.value("team", "");
                float offsetY = 0.0f;
                float spacing = elConfig.value("spacing", 24.0f) * options.scale; // Row height

                // Identify My Team Index (0 or 1) based on PostMatchInfo
                // We need to know which team index corresponds to "My Team"
                int myTeamIdx = -1;
                for(const auto& p : pm.players) { if(p.isLocal) { myTeamIdx = p.teamIndex; break; } }

                for (const auto& player : pm.players) {
                    // Filter Logic
                    if (teamFilter == "my_team" && player.teamIndex != myTeamIdx) continue;
                    if (teamFilter == "opp_team" && player.teamIndex == myTeamIdx) continue;

                    // Prepare Player Vars
                    std::map<std::string, std::string> pVars = vars; // Copy globals
                    pVars["Name"] = player.name;
                    pVars["Score"] = std::to_string(player.score);
                    pVars["Goals"] = std::to_string(player.goals);
                    pVars["Assists"] = std::to_string(player.assists);
                    pVars["Saves"] = std::to_string(player.saves);
                    pVars["Shots"] = std::to_string(player.shots);
                    pVars["Ping"] = std::to_string(player.ping);
                    
                    // Create Element for this player
                    RSElement el = CalculateElement(elConfig, options, pVars);
                    
                    // Apply Offset
                    for(auto& pos : el.positions) {
                        pos.y += offsetY;
                    }
                    
                    activeTheme.elements.push_back(el);
                    offsetY += spacing;
                }
            } else {
                // Standard single element
                RSElement el = CalculateElement(elConfig, options, vars);
                activeTheme.elements.push_back(el);
            }
        }
    }
}

RSElement ThemeManager::CalculateElement(const json& element, const RSOptions& options, std::map<std::string, std::string>& vars) {
    RSElement calculated;
    
    // Visibility check
    if (element.contains("visible") && !element["visible"].get<bool>()) {
        return calculated; // Return empty/invalid element
    }

    // Geometry Calculation
    float x = 0, y = 0, w = 0, h = 0;
    
    // Helper lambda for expression parsing
    auto eval = [&](const json& val, float size) -> float {
        if (val.is_string()) return OverlayUtils::EvaluateExpression(val.get<std::string>(), size);
        if (val.is_number()) return val.get<float>();
        return 0.0f;
    };

    if (element.contains("x")) x = eval(element["x"], options.width) * options.scale;
    if (element.contains("y")) y = eval(element["y"], options.height) * options.scale;
    if (element.contains("width")) w = eval(element["width"], options.width) * options.scale;
    if (element.contains("height")) h = eval(element["height"], options.height) * options.scale;

    ImVec2 pos = ImVec2(options.x + x, options.y + y);
    ImVec2 size = ImVec2(w, h);

    // Common Properties
    float elScale = element.value("scale", 1.0f) * options.scale;
    float elRotate = element.value("rotate", 0.0f);
    float elOpacity = options.opacity * element.value("opacity", 1.0f);

    // Color Helper
    auto getColor = [&](const json& j, const char* key) -> RSColor {
        if (j.contains(key) && j[key].is_array()) {
            auto arr = j[key];
            if (arr.size() >= 3) {
                float r = arr[0].get<float>() / 255.0f;
                float g = arr[1].get<float>() / 255.0f;
                float b = arr[2].get<float>() / 255.0f;
                float a = (arr.size() > 3 ? arr[3].get<float>() : 1.0f) * elOpacity;
                return { true, ImGui::GetColorU32(ImVec4(r, g, b, a)) };
            }
        }
        return { false, 0 };
    };

    calculated.color = getColor(element, "color");
    calculated.fill = getColor(element, "fill");
    calculated.stroke = getColor(element, "stroke");
    calculated.scale = elScale;
    
    if (elRotate != 0.0f) {
        calculated.rotate_enable = true;
        calculated.rotate = (90.0f - elRotate) * (3.14159f / 180.0f); // Degrees to Radians
    }

    // Type Specifics
    calculated.type = element.value("type", "unknown");
    
    if (calculated.type == "text") {
        std::string val = element.value("value", "");
        OverlayUtils::ReplaceVars(val, vars);
        calculated.value = val;
        
        // Alignment Logic (simplified)
        ImVec2 textSize = ImGui::CalcTextSize(val.c_str());
        textSize.x *= elScale;
        textSize.y *= elScale;

        std::string align = element.value("align", "left");
        if (align == "center") pos.x -= textSize.x * 0.5f;
        if (align == "right") pos.x -= textSize.x;
    } 
    else if (calculated.type == "rectangle") {
        // Rectangles need 2 points (Min, Max)
        // Store rounding in size.x for the renderer
        float rounding = element.value("rounding", 0.0f);
        calculated.size.x = rounding; 
        
        // Add positions: [TopLeft, BottomRight]
        calculated.positions.push_back(pos);
        calculated.positions.push_back(ImVec2(pos.x + w, pos.y + h));
        return calculated; // Early return as pos is already pushed
    }

    // List Properties
    calculated.repeat_mode = element.value("repeat", "");
    calculated.team_filter = element.value("team", "");

    // Default push for single-point elements (text, image)
    calculated.positions.push_back(pos);
    calculated.size = size;

    return calculated;
}

std::vector<std::string> ThemeManager::GetThemeNames() const {
    std::vector<std::string> names;
    for (const auto& t : themes) names.push_back(t.name);
    return names;
}
