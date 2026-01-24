#pragma once

#include "IMGUI/imgui.h"
#include "IMGUI/json.hpp"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class SuiteSpot;

class UiJsonRenderer {
public:
    explicit UiJsonRenderer(SuiteSpot* plugin);
    bool LoadFromFile(const std::filesystem::path& path, std::string* error_out);
    void Render();
    bool HasDocument() const;
    const std::string& GetLastError() const;
    const std::filesystem::path& GetLastPath() const;

private:
    bool ShouldShowWidget(const nlohmann::json& widget) const;
    void RenderWindow(const nlohmann::json& window);
    void RenderWidget(const nlohmann::json& widget);
    ImGuiWindowFlags ParseWindowFlags(const nlohmann::json& flags_json) const;
    const char* GetWidgetId(const nlohmann::json& widget) const;
    std::vector<char>& GetTextBuffer(const std::string& key, size_t max_len, const std::string& initial);

    SuiteSpot* plugin_;
    nlohmann::json doc_;
    std::filesystem::path last_path_;
    std::string last_error_;
    std::unordered_map<std::string, bool> bool_state_;
    std::unordered_map<std::string, int> int_state_;
    std::unordered_map<std::string, float> float_state_;
    std::unordered_map<std::string, std::vector<char>> text_state_;
    std::unordered_map<std::string, bool> window_open_state_;
};
