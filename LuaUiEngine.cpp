#include "pch.h"
#include "LuaUiEngine.h"
#include "SuiteSpot.h"

#include <lua.hpp>
#include <fstream>
#include <algorithm>

LuaUiEngine::LuaUiEngine(SuiteSpot* plugin)
    : plugin_(plugin) {}

LuaUiEngine::~LuaUiEngine() {
    ResetState();
}

void LuaUiEngine::ResetState() {
    if (state_) {
        lua_close(state_);
        state_ = nullptr;
    }
    has_script_ = false;
}

bool LuaUiEngine::LoadFromFile(const std::filesystem::path& path, std::string* error_out) {
    last_error_.clear();
    last_path_.clear();
    has_script_ = false;
    text_state_.clear();

    if (!std::filesystem::exists(path)) {
        last_error_ = "Lua file not found.";
        if (error_out) *error_out = last_error_;
        return false;
    }

    ResetState();
    state_ = luaL_newstate();
    luaL_openlibs(state_);
    RegisterApi();

    if (luaL_dofile(state_, path.string().c_str()) != LUA_OK) {
        last_error_ = lua_tostring(state_, -1);
        lua_pop(state_, 1);
        if (error_out) *error_out = last_error_;
        ResetState();
        return false;
    }

    last_path_ = path;
    last_write_time_ = std::filesystem::last_write_time(path);
    has_script_ = true;
    if (error_out) *error_out = "";
    return true;
}

bool LuaUiEngine::HasScript() const {
    return has_script_;
}

const std::string& LuaUiEngine::GetLastError() const {
    return last_error_;
}

const std::filesystem::path& LuaUiEngine::GetLastPath() const {
    return last_path_;
}

void LuaUiEngine::SetAutoReload(bool enabled) {
    auto_reload_ = enabled;
}

void LuaUiEngine::TickAutoReload() {
    if (!auto_reload_ || !has_script_ || last_path_.empty()) return;
    std::error_code ec;
    auto ts = std::filesystem::last_write_time(last_path_, ec);
    if (ec) return;
    if (ts != last_write_time_) {
        std::string err;
        LoadFromFile(last_path_, &err);
    }
}

void LuaUiEngine::Render() {
    if (!has_script_ || !state_) return;
    TickAutoReload();
    CallRender();
}

bool LuaUiEngine::CallRender() {
    lua_getglobal(state_, "render");
    if (!lua_isfunction(state_, -1)) {
        lua_pop(state_, 1);
        return false;
    }
    if (lua_pcall(state_, 0, 0, 0) != LUA_OK) {
        last_error_ = lua_tostring(state_, -1);
        lua_pop(state_, 1);
        return false;
    }
    return true;
}

std::vector<char>& LuaUiEngine::GetTextBuffer(const std::string& key, size_t max_len, const std::string& value) {
    auto it = text_state_.find(key);
    if (it == text_state_.end()) {
        std::vector<char> buf(max_len, '\0');
        it = text_state_.emplace(key, std::move(buf)).first;
    }
    auto& buf = it->second;
    if (buf.size() != max_len) {
        buf.assign(max_len, '\0');
    }
    if (!value.empty()) {
        const size_t copy_len = std::min(value.size(), max_len - 1);
        std::copy(value.begin(), value.begin() + copy_len, buf.begin());
        buf[copy_len] = '\0';
    }
    return buf;
}

LuaUiEngine* LuaUiEngine::GetEngine(lua_State* L) {
    return static_cast<LuaUiEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
}

static void PushUiFunc(lua_State* L, const char* name, lua_CFunction fn, LuaUiEngine* engine) {
    lua_pushstring(L, name);
    lua_pushlightuserdata(L, engine);
    lua_pushcclosure(L, fn, 1);
    lua_settable(L, -3);
}

void LuaUiEngine::RegisterApi() {
    lua_newtable(state_); // ui
    PushUiFunc(state_, "text", L_Text, this);
    PushUiFunc(state_, "text_wrapped", L_TextWrapped, this);
    PushUiFunc(state_, "text_colored", L_TextColored, this);
    PushUiFunc(state_, "button", L_Button, this);
    PushUiFunc(state_, "checkbox", L_Checkbox, this);
    PushUiFunc(state_, "slider_int", L_SliderInt, this);
    PushUiFunc(state_, "slider_float", L_SliderFloat, this);
    PushUiFunc(state_, "input_text", L_InputText, this);
    PushUiFunc(state_, "input_text_multiline", L_InputTextMultiline, this);
    PushUiFunc(state_, "combo", L_Combo, this);
    PushUiFunc(state_, "listbox", L_ListBox, this);
    PushUiFunc(state_, "separator", L_Separator, this);
    PushUiFunc(state_, "spacing", L_Spacing, this);
    PushUiFunc(state_, "same_line", L_SameLine, this);
    PushUiFunc(state_, "new_line", L_NewLine, this);
    PushUiFunc(state_, "indent", L_Indent, this);
    PushUiFunc(state_, "unindent", L_Unindent, this);
    PushUiFunc(state_, "begin_window", L_BeginWindow, this);
    PushUiFunc(state_, "end_window", L_EndWindow, this);
    PushUiFunc(state_, "begin_child", L_BeginChild, this);
    PushUiFunc(state_, "end_child", L_EndChild, this);
    PushUiFunc(state_, "begin_tab_bar", L_BeginTabBar, this);
    PushUiFunc(state_, "end_tab_bar", L_EndTabBar, this);
    PushUiFunc(state_, "begin_tab_item", L_BeginTabItem, this);
    PushUiFunc(state_, "end_tab_item", L_EndTabItem, this);
    PushUiFunc(state_, "tooltip", L_SetTooltip, this);
    lua_setglobal(state_, "ui");

    lua_newtable(state_); // cvar
    PushUiFunc(state_, "get", L_CvarGet, this);
    PushUiFunc(state_, "get_int", L_CvarGetInt, this);
    PushUiFunc(state_, "get_float", L_CvarGetFloat, this);
    PushUiFunc(state_, "get_bool", L_CvarGetBool, this);
    PushUiFunc(state_, "set", L_CvarSet, this);
    lua_setglobal(state_, "cvar");

    lua_newtable(state_); // cmd
    PushUiFunc(state_, "exec", L_CmdExec, this);
    lua_setglobal(state_, "cmd");
}

int LuaUiEngine::L_Text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    ImGui::TextUnformatted(text);
    return 0;
}

int LuaUiEngine::L_TextWrapped(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    ImGui::TextWrapped("%s", text);
    return 0;
}

int LuaUiEngine::L_TextColored(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    if (!lua_istable(L, 2)) {
        return 0;
    }
    lua_rawgeti(L, 2, 1);
    lua_rawgeti(L, 2, 2);
    lua_rawgeti(L, 2, 3);
    lua_rawgeti(L, 2, 4);
    ImVec4 color(
        (float)luaL_optnumber(L, -4, 1.0),
        (float)luaL_optnumber(L, -3, 1.0),
        (float)luaL_optnumber(L, -2, 1.0),
        (float)luaL_optnumber(L, -1, 1.0));
    lua_pop(L, 4);
    ImGui::TextColored(color, "%s", text);
    return 0;
}

int LuaUiEngine::L_Button(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool clicked = ImGui::Button(label);
    lua_pushboolean(L, clicked);
    return 1;
}

int LuaUiEngine::L_Checkbox(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool value = lua_toboolean(L, 2) != 0;
    bool changed = ImGui::Checkbox(label, &value);
    lua_pushboolean(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

int LuaUiEngine::L_SliderInt(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int value = (int)luaL_checkinteger(L, 2);
    int min_v = (int)luaL_checkinteger(L, 3);
    int max_v = (int)luaL_checkinteger(L, 4);
    bool changed = ImGui::SliderInt(label, &value, min_v, max_v);
    lua_pushinteger(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

int LuaUiEngine::L_SliderFloat(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float value = (float)luaL_checknumber(L, 2);
    float min_v = (float)luaL_checknumber(L, 3);
    float max_v = (float)luaL_checknumber(L, 4);
    bool changed = ImGui::SliderFloat(label, &value, min_v, max_v);
    lua_pushnumber(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

int LuaUiEngine::L_InputText(lua_State* L) {
    auto* engine = GetEngine(L);
    const char* label = luaL_checkstring(L, 1);
    const char* value = luaL_optstring(L, 2, "");
    size_t max_len = (size_t)luaL_optinteger(L, 3, 256);
    auto& buf = engine->GetTextBuffer(label, max_len, value);
    bool changed = ImGui::InputText(label, buf.data(), buf.size());
    lua_pushstring(L, buf.data());
    lua_pushboolean(L, changed);
    return 2;
}

int LuaUiEngine::L_InputTextMultiline(lua_State* L) {
    auto* engine = GetEngine(L);
    const char* label = luaL_checkstring(L, 1);
    const char* value = luaL_optstring(L, 2, "");
    size_t max_len = (size_t)luaL_optinteger(L, 3, 512);
    float height = (float)luaL_optnumber(L, 4, 80.0);
    auto& buf = engine->GetTextBuffer(label, max_len, value);
    bool changed = ImGui::InputTextMultiline(label, buf.data(), buf.size(), ImVec2(0, height));
    lua_pushstring(L, buf.data());
    lua_pushboolean(L, changed);
    return 2;
}

static int ReadStringTable(lua_State* L, int idx, std::vector<std::string>& out) {
    if (!lua_istable(L, idx)) return 0;
    int n = (int)lua_rawlen(L, idx);
    out.reserve(n);
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, idx, i);
        if (lua_isstring(L, -1)) {
            out.emplace_back(lua_tostring(L, -1));
        }
        lua_pop(L, 1);
    }
    return (int)out.size();
}

int LuaUiEngine::L_Combo(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    std::vector<std::string> items;
    ReadStringTable(L, 2, items);
    int current = (int)luaL_checkinteger(L, 3);
    bool changed = false;
    if (items.empty()) {
        lua_pushinteger(L, current);
        lua_pushboolean(L, changed);
        return 2;
    }
    current = std::max(1, std::min(current, (int)items.size()));
    if (ImGui::BeginCombo(label, items[current - 1].c_str())) {
        for (int i = 0; i < (int)items.size(); i++) {
            bool selected = (i == current - 1);
            if (ImGui::Selectable(items[i].c_str(), selected)) {
                current = i + 1;
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    lua_pushinteger(L, current);
    lua_pushboolean(L, changed);
    return 2;
}

int LuaUiEngine::L_ListBox(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    std::vector<std::string> items;
    ReadStringTable(L, 2, items);
    int current = (int)luaL_checkinteger(L, 3);
    int height_items = (int)luaL_optinteger(L, 4, 4);
    if (items.empty()) {
        lua_pushinteger(L, current);
        lua_pushboolean(L, false);
        return 2;
    }
    current = std::max(1, std::min(current, (int)items.size()));
    std::vector<const char*> citems;
    citems.reserve(items.size());
    for (auto& s : items) citems.push_back(s.c_str());
    bool changed = ImGui::ListBox(label, &current, citems.data(), (int)citems.size(), height_items);
    lua_pushinteger(L, current);
    lua_pushboolean(L, changed);
    return 2;
}

int LuaUiEngine::L_Separator(lua_State* L) {
    IM_UNUSED(L);
    ImGui::Separator();
    return 0;
}

int LuaUiEngine::L_Spacing(lua_State* L) {
    IM_UNUSED(L);
    ImGui::Spacing();
    return 0;
}

int LuaUiEngine::L_SameLine(lua_State* L) {
    float offset = (float)luaL_optnumber(L, 1, 0.0);
    float spacing = (float)luaL_optnumber(L, 2, -1.0);
    ImGui::SameLine(offset, spacing);
    return 0;
}

int LuaUiEngine::L_NewLine(lua_State* L) {
    IM_UNUSED(L);
    ImGui::NewLine();
    return 0;
}

int LuaUiEngine::L_Indent(lua_State* L) {
    float amount = (float)luaL_optnumber(L, 1, 0.0);
    ImGui::Indent(amount);
    return 0;
}

int LuaUiEngine::L_Unindent(lua_State* L) {
    float amount = (float)luaL_optnumber(L, 1, 0.0);
    ImGui::Unindent(amount);
    return 0;
}

int LuaUiEngine::L_BeginWindow(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    bool open = lua_isboolean(L, 2) ? (lua_toboolean(L, 2) != 0) : true;
    bool visible = ImGui::Begin(title, &open);
    lua_pushboolean(L, open);
    lua_pushboolean(L, visible);
    return 2;
}

int LuaUiEngine::L_EndWindow(lua_State* L) {
    IM_UNUSED(L);
    ImGui::End();
    return 0;
}

int LuaUiEngine::L_BeginChild(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    float w = (float)luaL_optnumber(L, 2, 0.0);
    float h = (float)luaL_optnumber(L, 3, 0.0);
    bool border = lua_isboolean(L, 4) ? (lua_toboolean(L, 4) != 0) : false;
    bool visible = ImGui::BeginChild(name, ImVec2(w, h), border);
    lua_pushboolean(L, visible);
    return 1;
}

int LuaUiEngine::L_EndChild(lua_State* L) {
    IM_UNUSED(L);
    ImGui::EndChild();
    return 0;
}

int LuaUiEngine::L_BeginTabBar(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    bool ok = ImGui::BeginTabBar(name);
    lua_pushboolean(L, ok);
    return 1;
}

int LuaUiEngine::L_EndTabBar(lua_State* L) {
    IM_UNUSED(L);
    ImGui::EndTabBar();
    return 0;
}

int LuaUiEngine::L_BeginTabItem(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool open = lua_isboolean(L, 2) ? (lua_toboolean(L, 2) != 0) : true;
    bool ok = ImGui::BeginTabItem(label, &open);
    lua_pushboolean(L, open);
    lua_pushboolean(L, ok);
    return 2;
}

int LuaUiEngine::L_EndTabItem(lua_State* L) {
    IM_UNUSED(L);
    ImGui::EndTabItem();
    return 0;
}

int LuaUiEngine::L_SetTooltip(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    ImGui::SetTooltip("%s", text);
    return 0;
}

int LuaUiEngine::L_CvarGet(lua_State* L) {
    auto* engine = GetEngine(L);
    if (!engine->plugin_ || !engine->plugin_->cvarManager) {
        lua_pushnil(L);
        return 1;
    }
    const char* name = luaL_checkstring(L, 1);
    auto cvar = engine->plugin_->cvarManager->getCvar(name);
    if (cvar.IsNull()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, cvar.getStringValue().c_str());
    return 1;
}

int LuaUiEngine::L_CvarGetInt(lua_State* L) {
    auto* engine = GetEngine(L);
    if (!engine->plugin_ || !engine->plugin_->cvarManager) {
        lua_pushnil(L);
        return 1;
    }
    const char* name = luaL_checkstring(L, 1);
    auto cvar = engine->plugin_->cvarManager->getCvar(name);
    if (cvar.IsNull()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, cvar.getIntValue());
    return 1;
}

int LuaUiEngine::L_CvarGetFloat(lua_State* L) {
    auto* engine = GetEngine(L);
    if (!engine->plugin_ || !engine->plugin_->cvarManager) {
        lua_pushnil(L);
        return 1;
    }
    const char* name = luaL_checkstring(L, 1);
    auto cvar = engine->plugin_->cvarManager->getCvar(name);
    if (cvar.IsNull()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, cvar.getFloatValue());
    return 1;
}

int LuaUiEngine::L_CvarGetBool(lua_State* L) {
    auto* engine = GetEngine(L);
    if (!engine->plugin_ || !engine->plugin_->cvarManager) {
        lua_pushnil(L);
        return 1;
    }
    const char* name = luaL_checkstring(L, 1);
    auto cvar = engine->plugin_->cvarManager->getCvar(name);
    if (cvar.IsNull()) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushboolean(L, cvar.getBoolValue());
    return 1;
}

int LuaUiEngine::L_CvarSet(lua_State* L) {
    auto* engine = GetEngine(L);
    if (!engine->plugin_ || !engine->plugin_->cvarManager) {
        return 0;
    }
    const char* name = luaL_checkstring(L, 1);
    auto cvar = engine->plugin_->cvarManager->getCvar(name);
    if (cvar.IsNull()) {
        return 0;
    }
    if (lua_isboolean(L, 2)) {
        cvar.setValue(lua_toboolean(L, 2) ? 1 : 0);
    } else if (lua_isnumber(L, 2)) {
        cvar.setValue((float)lua_tonumber(L, 2));
    } else if (lua_isstring(L, 2)) {
        cvar.setValue(std::string(lua_tostring(L, 2)));
    }
    return 0;
}

int LuaUiEngine::L_CmdExec(lua_State* L) {
    auto* engine = GetEngine(L);
    if (!engine->plugin_ || !engine->plugin_->cvarManager) {
        return 0;
    }
    const char* cmd = luaL_checkstring(L, 1);
    engine->plugin_->cvarManager->executeCommand(cmd);
    return 0;
}
