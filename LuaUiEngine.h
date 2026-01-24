#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct lua_State;
class SuiteSpot;

class LuaUiEngine {
public:
    explicit LuaUiEngine(SuiteSpot* plugin);
    ~LuaUiEngine();

    bool LoadFromFile(const std::filesystem::path& path, std::string* error_out);
    void Render();
    void SetAutoReload(bool enabled);
    void TickAutoReload();

    bool HasScript() const;
    const std::string& GetLastError() const;
    const std::filesystem::path& GetLastPath() const;

private:
    void ResetState();
    void RegisterApi();
    bool CallRender();
    std::vector<char>& GetTextBuffer(const std::string& key, size_t max_len, const std::string& value);

    static LuaUiEngine* GetEngine(lua_State* L);

    static int L_Text(lua_State* L);
    static int L_TextWrapped(lua_State* L);
    static int L_TextColored(lua_State* L);
    static int L_Button(lua_State* L);
    static int L_Checkbox(lua_State* L);
    static int L_SliderInt(lua_State* L);
    static int L_SliderFloat(lua_State* L);
    static int L_InputText(lua_State* L);
    static int L_InputTextMultiline(lua_State* L);
    static int L_Combo(lua_State* L);
    static int L_ListBox(lua_State* L);
    static int L_Separator(lua_State* L);
    static int L_Spacing(lua_State* L);
    static int L_SameLine(lua_State* L);
    static int L_NewLine(lua_State* L);
    static int L_Indent(lua_State* L);
    static int L_Unindent(lua_State* L);
    static int L_BeginWindow(lua_State* L);
    static int L_EndWindow(lua_State* L);
    static int L_BeginChild(lua_State* L);
    static int L_EndChild(lua_State* L);
    static int L_BeginTabBar(lua_State* L);
    static int L_EndTabBar(lua_State* L);
    static int L_BeginTabItem(lua_State* L);
    static int L_EndTabItem(lua_State* L);
    static int L_SetTooltip(lua_State* L);
    static int L_Image(lua_State* L);
    static int L_DrawLine(lua_State* L);
    static int L_DrawRect(lua_State* L);
    static int L_DrawRectFilled(lua_State* L);
    static int L_BeginDragDropSource(lua_State* L);
    static int L_SetDragDropPayload(lua_State* L);
    static int L_EndDragDropSource(lua_State* L);
    static int L_BeginDragDropTarget(lua_State* L);
    static int L_AcceptDragDropPayload(lua_State* L);
    static int L_EndDragDropTarget(lua_State* L);

    static int L_CvarGet(lua_State* L);
    static int L_CvarGetInt(lua_State* L);
    static int L_CvarGetFloat(lua_State* L);
    static int L_CvarGetBool(lua_State* L);
    static int L_CvarSet(lua_State* L);
    static int L_CmdExec(lua_State* L);

    SuiteSpot* plugin_;
    lua_State* state_ = nullptr;
    std::filesystem::path last_path_;
    std::filesystem::file_time_type last_write_time_{};
    std::string last_error_;
    bool auto_reload_ = false;
    bool has_script_ = false;
    std::unordered_map<std::string, std::vector<char>> text_state_;
};
