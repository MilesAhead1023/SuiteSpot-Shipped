#include "pch.h"
#include "UiJsonRenderer.h"
#include "SuiteSpot.h"

#include <fstream>
#include <algorithm>

using json = nlohmann::json;

UiJsonRenderer::UiJsonRenderer(SuiteSpot* plugin)
    : plugin_(plugin) {}

bool UiJsonRenderer::LoadFromFile(const std::filesystem::path& path, std::string* error_out) {
    last_error_.clear();
    last_path_.clear();

    std::ifstream input(path);
    if (!input.is_open()) {
        last_error_ = "Failed to open JSON file.";
        if (error_out) *error_out = last_error_;
        return false;
    }

    try {
        input >> doc_;
    } catch (const std::exception& ex) {
        last_error_ = std::string("JSON parse error: ") + ex.what();
        if (error_out) *error_out = last_error_;
        return false;
    }

    if (!doc_.is_object() || !doc_.contains("windows") || !doc_["windows"].is_array()) {
        last_error_ = "Invalid JSON: expected { \"windows\": [ ... ] }.";
        if (error_out) *error_out = last_error_;
        return false;
    }

    last_path_ = path;
    if (error_out) *error_out = "";
    return true;
}

bool UiJsonRenderer::HasDocument() const {
    return doc_.is_object() && doc_.contains("windows");
}

const std::string& UiJsonRenderer::GetLastError() const {
    return last_error_;
}

const std::filesystem::path& UiJsonRenderer::GetLastPath() const {
    return last_path_;
}

void UiJsonRenderer::Render() {
    if (!HasDocument()) return;
    for (const auto& window : doc_["windows"]) {
        if (!window.is_object()) continue;
        RenderWindow(window);
    }
}

ImGuiWindowFlags UiJsonRenderer::ParseWindowFlags(const json& flags_json) const {
    ImGuiWindowFlags flags = 0;
    if (!flags_json.is_array()) return flags;
    for (const auto& f : flags_json) {
        if (!f.is_string()) continue;
        const std::string name = f.get<std::string>();
        if (name == "AlwaysAutoResize") flags |= ImGuiWindowFlags_AlwaysAutoResize;
        else if (name == "NoResize") flags |= ImGuiWindowFlags_NoResize;
        else if (name == "NoCollapse") flags |= ImGuiWindowFlags_NoCollapse;
        else if (name == "NoTitleBar") flags |= ImGuiWindowFlags_NoTitleBar;
        else if (name == "NoScrollbar") flags |= ImGuiWindowFlags_NoScrollbar;
        else if (name == "NoScrollWithMouse") flags |= ImGuiWindowFlags_NoScrollWithMouse;
        else if (name == "MenuBar") flags |= ImGuiWindowFlags_MenuBar;
    }
    return flags;
}

const char* UiJsonRenderer::GetWidgetId(const json& widget) const {
    if (widget.contains("id") && widget["id"].is_string()) {
        return widget["id"].get_ref<const std::string&>().c_str();
    }
    if (widget.contains("label") && widget["label"].is_string()) {
        return widget["label"].get_ref<const std::string&>().c_str();
    }
    return "";
}

std::vector<char>& UiJsonRenderer::GetTextBuffer(const std::string& key, size_t max_len, const std::string& initial) {
    auto it = text_state_.find(key);
    if (it == text_state_.end()) {
        std::vector<char> buf(max_len, '\0');
        if (!initial.empty()) {
            const size_t copy_len = std::min(initial.size(), max_len - 1);
            std::copy(initial.begin(), initial.begin() + copy_len, buf.begin());
            buf[copy_len] = '\0';
        }
        it = text_state_.emplace(key, std::move(buf)).first;
    }
    return it->second;
}

bool UiJsonRenderer::ShouldShowWidget(const json& widget) const {
    if (!plugin_ || !plugin_->cvarManager) return true;
    if (!widget.contains("visible_if_cvar")) return true;
    const auto& rule = widget["visible_if_cvar"];
    if (!rule.is_object()) return true;
    if (!rule.contains("name") || !rule["name"].is_string()) return true;
    const std::string name = rule["name"].get<std::string>();
    auto cvar = plugin_->cvarManager->getCvar(name);
    if (cvar.IsNull()) return true;
    if (rule.contains("equals")) {
        if (rule["equals"].is_boolean()) {
            return cvar.getBoolValue() == rule["equals"].get<bool>();
        }
        if (rule["equals"].is_number_integer()) {
            return cvar.getIntValue() == rule["equals"].get<int>();
        }
        if (rule["equals"].is_number_float()) {
            return cvar.getFloatValue() == rule["equals"].get<float>();
        }
        if (rule["equals"].is_string()) {
            return cvar.getStringValue() == rule["equals"].get<std::string>();
        }
    }
    if (rule.contains("is_true")) {
        return cvar.getBoolValue() == rule["is_true"].get<bool>();
    }
    return true;
}

void UiJsonRenderer::RenderWindow(const json& window) {
    if (!window.contains("title") || !window["title"].is_string()) return;
    const std::string title = window["title"].get<std::string>();
    const std::string id = window.value("id", title);

    bool open = true;
    auto it_open = window_open_state_.find(id);
    if (it_open != window_open_state_.end()) {
        open = it_open->second;
    }
    if (window.contains("open") && window["open"].is_boolean()) {
        open = window["open"].get<bool>();
    }

    ImGuiWindowFlags flags = 0;
    if (window.contains("flags")) {
        flags = ParseWindowFlags(window["flags"]);
    }

    if (!ImGui::Begin(title.c_str(), &open, flags)) {
        ImGui::End();
        window_open_state_[id] = open;
        return;
    }

    if (window.contains("widgets") && window["widgets"].is_array()) {
        for (const auto& widget : window["widgets"]) {
            if (!widget.is_object()) continue;
            if (!ShouldShowWidget(widget)) continue;
            RenderWidget(widget);
        }
    }

    ImGui::End();
    window_open_state_[id] = open;
}

void UiJsonRenderer::RenderWidget(const json& widget) {
    if (!widget.contains("type") || !widget["type"].is_string()) return;
    const std::string type = widget["type"].get<std::string>();
    const std::string label = widget.value("label", "");
    const std::string bind = widget.value("bind_cvar", "");
    const std::string tooltip = widget.value("tooltip", "");

    if (type == "text") {
        if (widget.contains("value") && widget["value"].is_string())
            ImGui::TextUnformatted(widget["value"].get_ref<const std::string&>().c_str());
    } else if (type == "text_wrapped") {
        if (widget.contains("value") && widget["value"].is_string())
            ImGui::TextWrapped("%s", widget["value"].get_ref<const std::string&>().c_str());
    } else if (type == "text_colored") {
        if (widget.contains("value") && widget["value"].is_string() && widget.contains("color") && widget["color"].is_array()) {
            auto c = widget["color"];
            if (c.size() >= 4) {
                ImVec4 color((float)c[0], (float)c[1], (float)c[2], (float)c[3]);
                ImGui::TextColored(color, "%s", widget["value"].get_ref<const std::string&>().c_str());
            }
        }
    } else if (type == "separator") {
        ImGui::Separator();
    } else if (type == "spacing") {
        ImGui::Spacing();
    } else if (type == "same_line") {
        ImGui::SameLine();
    } else if (type == "new_line") {
        ImGui::NewLine();
    } else if (type == "indent") {
        float amount = widget.value("amount", 0.0f);
        ImGui::Indent(amount);
    } else if (type == "unindent") {
        float amount = widget.value("amount", 0.0f);
        ImGui::Unindent(amount);
    } else if (type == "button") {
        if (ImGui::Button(label.c_str())) {
            if (plugin_ && plugin_->cvarManager && widget.contains("action") && widget["action"].is_object()) {
                const auto& action = widget["action"];
                if (action.contains("command") && action["command"].is_string()) {
                    plugin_->cvarManager->executeCommand(action["command"].get<std::string>());
                }
            }
        }
    } else if (type == "checkbox") {
        bool value = false;
        if (!bind.empty() && plugin_ && plugin_->cvarManager) {
            value = plugin_->cvarManager->getCvar(bind).getBoolValue();
            if (ImGui::Checkbox(label.c_str(), &value)) {
                plugin_->cvarManager->getCvar(bind).setValue(value ? 1 : 0);
            }
        } else {
            const std::string id = GetWidgetId(widget);
            value = bool_state_[id];
            ImGui::Checkbox(label.c_str(), &value);
            bool_state_[id] = value;
        }
    } else if (type == "slider_int") {
        int value = 0;
        int min_v = widget.value("min", 0);
        int max_v = widget.value("max", 100);
        if (!bind.empty() && plugin_ && plugin_->cvarManager) {
            value = plugin_->cvarManager->getCvar(bind).getIntValue();
            if (ImGui::SliderInt(label.c_str(), &value, min_v, max_v)) {
                plugin_->cvarManager->getCvar(bind).setValue(value);
            }
        } else {
            const std::string id = GetWidgetId(widget);
            value = int_state_[id];
            ImGui::SliderInt(label.c_str(), &value, min_v, max_v);
            int_state_[id] = value;
        }
    } else if (type == "slider_float") {
        float value = 0.0f;
        float min_v = widget.value("min", 0.0f);
        float max_v = widget.value("max", 1.0f);
        if (!bind.empty() && plugin_ && plugin_->cvarManager) {
            value = plugin_->cvarManager->getCvar(bind).getFloatValue();
            if (ImGui::SliderFloat(label.c_str(), &value, min_v, max_v)) {
                plugin_->cvarManager->getCvar(bind).setValue(value);
            }
        } else {
            const std::string id = GetWidgetId(widget);
            value = float_state_[id];
            ImGui::SliderFloat(label.c_str(), &value, min_v, max_v);
            float_state_[id] = value;
        }
    } else if (type == "input_text") {
        const std::string id = GetWidgetId(widget);
        size_t max_len = (size_t)widget.value("max_length", 256);
        std::string initial;
        if (!bind.empty() && plugin_ && plugin_->cvarManager) {
            initial = plugin_->cvarManager->getCvar(bind).getStringValue();
        }
        auto& buf = GetTextBuffer(id, max_len, initial);
        if (ImGui::InputText(label.c_str(), buf.data(), buf.size())) {
            if (!bind.empty() && plugin_ && plugin_->cvarManager) {
                plugin_->cvarManager->getCvar(bind).setValue(std::string(buf.data()));
            }
        }
    } else if (type == "input_text_multiline") {
        const std::string id = GetWidgetId(widget);
        size_t max_len = (size_t)widget.value("max_length", 512);
        std::string initial;
        if (!bind.empty() && plugin_ && plugin_->cvarManager) {
            initial = plugin_->cvarManager->getCvar(bind).getStringValue();
        }
        auto& buf = GetTextBuffer(id, max_len, initial);
        float height = widget.value("height", 80.0f);
        if (ImGui::InputTextMultiline(label.c_str(), buf.data(), buf.size(), ImVec2(0, height))) {
            if (!bind.empty() && plugin_ && plugin_->cvarManager) {
                plugin_->cvarManager->getCvar(bind).setValue(std::string(buf.data()));
            }
        }
    } else if (type == "combo") {
        if (widget.contains("items") && widget["items"].is_array()) {
            struct ComboItem { std::string label; bool is_number; int int_value; };
            std::vector<ComboItem> items;
            items.reserve(widget["items"].size());
            for (const auto& it : widget["items"]) {
                if (it.is_string()) {
                    items.push_back({ it.get<std::string>(), false, 0 });
                } else if (it.is_number_integer()) {
                    items.push_back({ std::to_string(it.get<int>()), true, it.get<int>() });
                }
            }
            if (!items.empty()) {
                int current = 0;
                if (!bind.empty() && plugin_ && plugin_->cvarManager) {
                    auto cvar = plugin_->cvarManager->getCvar(bind);
                    for (int i = 0; i < (int)items.size(); i++) {
                        if (items[i].is_number && cvar.getIntValue() == items[i].int_value) {
                            current = i;
                            break;
                        }
                        if (!items[i].is_number && cvar.getStringValue() == items[i].label) {
                            current = i;
                            break;
                        }
                    }
                }
                if (ImGui::BeginCombo(label.c_str(), items[current].label.c_str())) {
                    for (int i = 0; i < (int)items.size(); i++) {
                        bool selected = (i == current);
                        if (ImGui::Selectable(items[i].label.c_str(), selected)) {
                            current = i;
                            if (!bind.empty() && plugin_ && plugin_->cvarManager) {
                                if (items[i].is_number) {
                                    plugin_->cvarManager->getCvar(bind).setValue(items[i].int_value);
                                } else {
                                    plugin_->cvarManager->getCvar(bind).setValue(items[i].label);
                                }
                            }
                        }
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
        }
    } else if (type == "listbox") {
        if (widget.contains("items") && widget["items"].is_array()) {
            struct ComboItem { std::string label; bool is_number; int int_value; };
            std::vector<ComboItem> items;
            items.reserve(widget["items"].size());
            for (const auto& it : widget["items"]) {
                if (it.is_string()) {
                    items.push_back({ it.get<std::string>(), false, 0 });
                } else if (it.is_number_integer()) {
                    items.push_back({ std::to_string(it.get<int>()), true, it.get<int>() });
                }
            }
            if (!items.empty()) {
                int current = 0;
                const std::string id = GetWidgetId(widget);
                if (!bind.empty() && plugin_ && plugin_->cvarManager) {
                    auto cvar = plugin_->cvarManager->getCvar(bind);
                    for (int i = 0; i < (int)items.size(); i++) {
                        if (items[i].is_number && cvar.getIntValue() == items[i].int_value) {
                            current = i;
                            break;
                        }
                        if (!items[i].is_number && cvar.getStringValue() == items[i].label) {
                            current = i;
                            break;
                        }
                    }
                } else {
                    current = int_state_[id];
                }
                std::vector<const char*> citems;
                citems.reserve(items.size());
                for (const auto& s : items) citems.push_back(s.label.c_str());
                if (ImGui::ListBox(label.c_str(), &current, citems.data(), (int)citems.size(), widget.value("height_items", 4))) {
                    if (!bind.empty() && plugin_ && plugin_->cvarManager) {
                        if (items[current].is_number) {
                            plugin_->cvarManager->getCvar(bind).setValue(items[current].int_value);
                        } else {
                            plugin_->cvarManager->getCvar(bind).setValue(items[current].label);
                        }
                    } else {
                        int_state_[id] = current;
                    }
                }
            }
        }
    }

    if (!tooltip.empty() && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip.c_str());
    }
}
