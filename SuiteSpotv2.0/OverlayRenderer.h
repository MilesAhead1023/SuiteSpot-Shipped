#pragma once
#include "OverlayTypes.h"
#include "OverlayUtils.h"
#include <chrono>

class SuiteSpot;

class OverlayRenderer {
public:
    explicit OverlayRenderer(SuiteSpot* plugin);

    // Main render entry point (RocketStats pattern)
    void Render(const ImVec2& displaySize);

    // Specific render functions
    void RenderOverlayContent(ImDrawList* dl, ImVec2 pos, ImVec2 size, float opacity);
    void RenderMoveHandle(ImDrawList* dl, const ImVec2& pos, const ImVec2& overlaySize);

    // Overlay state
    bool IsOverlayMoving() const { return overlayMoving; }
    void SetOverlayMoving(bool value) { overlayMoving = value; }

    float GetTeamHeaderHeight() const { return teamHeaderHeight; }
    void SetTeamHeaderHeight(float value) { teamHeaderHeight = value; }

    float GetPlayerRowHeight() const { return playerRowHeight; }
    void SetPlayerRowHeight(float value) { playerRowHeight = value; }

    float GetTeamSectionSpacing() const { return teamSectionSpacing; }
    void SetTeamSectionSpacing(float value) { teamSectionSpacing = value; }

    float GetSectionPadding() const { return sectionPadding; }
    void SetSectionPadding(float value) { sectionPadding = value; }

    float GetNameColumnX() const { return nameColumnX; }
    void SetNameColumnX(float value) { nameColumnX = value; }

    float GetScoreColumnX() const { return scoreColumnX; }
    void SetScoreColumnX(float value) { scoreColumnX = value; }

    float GetGoalsColumnX() const { return goalsColumnX; }
    void SetGoalsColumnX(float value) { goalsColumnX = value; }

    float GetAssistsColumnX() const { return assistsColumnX; }
    void SetAssistsColumnX(float value) { assistsColumnX = value; }

    float GetSavesColumnX() const { return savesColumnX; }
    void SetSavesColumnX(float value) { savesColumnX = value; }

    float GetShotsColumnX() const { return shotsColumnX; }
    void SetShotsColumnX(float value) { shotsColumnX = value; }

    float GetPingColumnX() const { return pingColumnX; }
    void SetPingColumnX(float value) { pingColumnX = value; }

    float GetMainFontSize() const { return mainFontSize; }
    void SetMainFontSize(float value) { mainFontSize = value; }

    float GetHeaderFontSize() const { return headerFontSize; }
    void SetHeaderFontSize(float value) { headerFontSize = value; }

    float GetTeamHeaderFontSize() const { return teamHeaderFontSize; }
    void SetTeamHeaderFontSize(float value) { teamHeaderFontSize = value; }

    float GetBlueTeamSat() const { return blueTeamSat; }
    void SetBlueTeamSat(float value) { blueTeamSat = value; }

    float GetBlueTeamVal() const { return blueTeamVal; }
    void SetBlueTeamVal(float value) { blueTeamVal = value; }

    float GetOrangeTeamSat() const { return orangeTeamSat; }
    void SetOrangeTeamSat(float value) { orangeTeamSat = value; }

    float GetOrangeTeamVal() const { return orangeTeamVal; }
    void SetOrangeTeamVal(float value) { orangeTeamVal = value; }

    float GetBackgroundAlpha() const { return backgroundAlpha; }
    void SetBackgroundAlpha(float value) { backgroundAlpha = value; }

    float GetHeaderAlpha() const { return headerAlpha; }
    void SetHeaderAlpha(float value) { headerAlpha = value; }

    float GetMvpCheckmarkSize() const { return mvpCheckmarkSize; }
    void SetMvpCheckmarkSize(float value) { mvpCheckmarkSize = value; }

    bool GetShowMvpGlow() const { return showMvpGlow; }
    void SetShowMvpGlow(bool value) { showMvpGlow = value; }

    bool GetShowTeamScores() const { return showTeamScores; }
    void SetShowTeamScores(bool value) { showTeamScores = value; }

    bool GetShowColumnHeaders() const { return showColumnHeaders; }
    void SetShowColumnHeaders(bool value) { showColumnHeaders = value; }

    float GetFadeInDuration() const { return fadeInDuration; }
    void SetFadeInDuration(float value) { fadeInDuration = value; }

    float GetFadeOutDuration() const { return fadeOutDuration; }
    void SetFadeOutDuration(float value) { fadeOutDuration = value; }

    bool GetEnableFadeEffects() const { return enableFadeEffects; }
    void SetEnableFadeEffects(bool value) { enableFadeEffects = value; }

    void ResetDefaults();

private:
    void RenderElement(ImDrawList* dl, const RSElement& element);
    RSElement CreateTextElement(std::string text, float x, float y, float scale, ImU32 color);
    RSElement CreateRectElement(float x, float y, float w, float h, ImU32 color, float rounding, bool filled);

    SuiteSpot* plugin_;

    float spawnOpacity = 0.0f;  // RocketStats' rs_launch equivalent

    // Overlay move state (RocketStats pattern)
    bool overlayMoving = false;
    ImVec2 moveOrigin = {0, 0};
    ImVec2 moveCursor = {0, 0};

    // Display state cache
    ImVec2 displaySize = {0, 0};

    // Frame rate limiter (RocketStats pattern)
    std::chrono::steady_clock::time_point lastFrameTime;
    int frameCounter = 0;

    float teamHeaderHeight = 28.0f;
    float playerRowHeight = 24.0f;
    float teamSectionSpacing = 12.0f;
    float sectionPadding = 8.0f;

    float nameColumnX = 50.0f;
    float scoreColumnX = 230.0f;
    float goalsColumnX = 290.0f;
    float assistsColumnX = 350.0f;
    float savesColumnX = 410.0f;
    float shotsColumnX = 470.0f;
    float pingColumnX = 530.0f;

    float mainFontSize = 14.0f;
    float headerFontSize = 12.0f;
    float teamHeaderFontSize = 16.0f;

    float blueTeamSat = 0.8f;
    float blueTeamVal = 0.6f;
    float orangeTeamSat = 0.9f;
    float orangeTeamVal = 0.7f;
    float backgroundAlpha = 0.4f;
    float headerAlpha = 0.8f;

    float mvpCheckmarkSize = 1.2f;
    bool showMvpGlow = true;
    bool showTeamScores = true;
    bool showColumnHeaders = true;

    float fadeInDuration = 0.5f;
    float fadeOutDuration = 2.0f;
    bool enableFadeEffects = true;
};
