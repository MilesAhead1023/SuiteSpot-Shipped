#include "pch.h"

#include "OverlayRenderer.h"
#include "SuiteSpot.h"
#include "ThemeManager.h"

#include <algorithm>
#include <cmath>

OverlayRenderer::OverlayRenderer(SuiteSpot* plugin) : plugin_(plugin) {}

void OverlayRenderer::ResetDefaults() {
    overlayOffsetX = 0.0f;
    overlayOffsetY = 0.0f;

    teamHeaderHeight = 28.0f;
    playerRowHeight = 24.0f;
    teamSectionSpacing = 12.0f;
    sectionPadding = 8.0f;

    nameColumnX = 50.0f;
    scoreColumnX = 230.0f;
    goalsColumnX = 290.0f;
    assistsColumnX = 350.0f;
    savesColumnX = 410.0f;
    shotsColumnX = 470.0f;
    pingColumnX = 530.0f;

    mainFontSize = 14.0f;
    headerFontSize = 12.0f;
    teamHeaderFontSize = 16.0f;

    blueTeamSat = 0.8f;
    blueTeamVal = 0.6f;
    orangeTeamSat = 0.9f;
    orangeTeamVal = 0.7f;
    backgroundAlpha = 0.4f;
    headerAlpha = 0.8f;

    mvpCheckmarkSize = 1.2f;
    showMvpGlow = true;
    showTeamScores = true;
    showColumnHeaders = true;

    fadeInDuration = 0.5f;
    fadeOutDuration = 2.0f;
    enableFadeEffects = true;
}

void OverlayRenderer::RenderPostMatchOverlay() {
    if (!plugin_) {
        return;
    }

    if (!ImGui::GetCurrentContext()) {
        if (plugin_->imguiCtx) {
            ImGui::SetCurrentContext(plugin_->imguiCtx);
        } else {
            return;
        }
    }

    auto& postMatch = plugin_->GetPostMatchInfo();

    // Check if overlay should still be shown
    const auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - postMatch.start).count();
    const float postMatchDurationSec = plugin_->GetPostMatchDurationSec();

    // Auto-hide after duration
    if (postMatch.active && elapsed >= postMatchDurationSec) {
        postMatch.active = false;
        if (plugin_->postMatchOverlayWindow) {
            plugin_->postMatchOverlayWindow->Close();
        }
        return;
    }

    // Calculate fade
    float alpha = plugin_->GetOverlayAlpha();
    if (enableFadeEffects) {
        if (elapsed < fadeInDuration) {
            alpha *= (elapsed / fadeInDuration);
        } else if (elapsed > postMatchDurationSec - fadeOutDuration) {
            alpha *= std::max(0.0f, (postMatchDurationSec - elapsed) / fadeOutDuration);
        }
    }

    ImVec2 display = ImGui::GetIO().DisplaySize;
    if (display.x < 100.0f || display.y < 100.0f) {
        return;
    }
    const ImVec2 overlaySize = ImVec2(std::max(400.0f, plugin_->GetOverlayWidth()),
                                      std::max(180.0f, plugin_->GetOverlayHeight()));
    
    // Use CVar-backed offsets
    float offsetX = plugin_->GetOverlayOffsetX();
    float offsetY = plugin_->GetOverlayOffsetY();
    ImVec2 pos = ImVec2((display.x - overlaySize.x) * 0.5f + offsetX, display.y * 0.08f + offsetY);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl) {
        return;
    }
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // --- NEW THEME SYSTEM INTEGRATION ---
    if (auto* tm = plugin_->GetThemeManager()) {
        
        // 1. Prepare Variables
        std::map<std::string, std::string> vars;
        vars["MyScore"] = std::to_string(postMatch.myScore);
        vars["OppScore"] = std::to_string(postMatch.oppScore);
        vars["MyTeam"] = postMatch.myTeamName;
        vars["OppTeam"] = postMatch.oppTeamName;
        vars["Playlist"] = postMatch.playlist;
        
        // TODO: Add player stats loop for "list" type elements later
        // For now, we just pass global match stats

        // 2. Prepare Options
        RSOptions options;
        options.x = winPos.x;
        options.y = winPos.y;
        options.width = winSize.x;
        options.height = winSize.y;
        options.scale = 1.0f; // Global scale
        options.opacity = alpha;

        // 3. Update & Render
        tm->UpdateThemeElements(options, vars, postMatch);
        
        for (auto& el : tm->GetActiveTheme().elements) {
            RenderElement(dl, el);
        }
        
        // If theme has elements, we skip the hardcoded fallback
        if (!tm->GetActiveTheme().elements.empty()) {
            return;
        }
    }

    // --- FALLBACK HARDCODED RENDERER ---
    // (Only runs if no theme is active or theme is empty)
    
    // Draw background
    ImU32 baseBg = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, backgroundAlpha * alpha));
    RSElement bgRect = CreateRectElement(winPos.x, winPos.y, winSize.x, winSize.y, baseBg, 8.0f, true);
    RenderElement(dl, bgRect);

    // Draw header
    ImU32 headerBg = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, headerAlpha * alpha));
    RSElement headerRect = CreateRectElement(winPos.x, winPos.y, winSize.x, 34.0f, headerBg, 8.0f, true);
    RenderElement(dl, headerRect);

    ImU32 titleColor = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, alpha));
    int mode = plugin_->GetOverlayMode();

    if (mode == 1) {
        // --- SESSION STATS VIEW ---
        RSElement sessionTitle = CreateTextElement("SESSION SUMMARY", winPos.x + 12.0f, winPos.y + 8.0f, teamHeaderFontSize / 13.0f, titleColor);
        RenderElement(dl, sessionTitle);
        
        auto& stats = plugin_->sessionStats;
        std::string subHeader = "Matches: " + std::to_string(stats.matchesPlayed) + 
                              " | Wins: " + std::to_string(stats.wins) + 
                              " | Losses: " + std::to_string(stats.losses);
        RSElement statsHeader = CreateTextElement(subHeader, winPos.x + 12.0f, winPos.y + 45.0f, headerFontSize / 13.0f, titleColor);
        RenderElement(dl, statsHeader);

        // Simple table for session stats
        float startY = winPos.y + 80.0f;
        float labelX = winPos.x + 50.0f;
        float valueX = winPos.x + 250.0f;
        float rowH = 24.0f;

        auto DrawRow = [&](const char* label, int value, int rowIdx) {
            float y = startY + (rowIdx * rowH);
            RSElement labelEl = CreateTextElement(label, labelX, y, mainFontSize / 13.0f, titleColor);
            RenderElement(dl, labelEl);
            RSElement valueEl = CreateTextElement(std::to_string(value), valueX, y, mainFontSize / 13.0f, titleColor);
            RenderElement(dl, valueEl);
        };

        DrawRow("Goals", stats.goals, 0);
        DrawRow("Assists", stats.assists, 1);
        DrawRow("Saves", stats.saves, 2);
        DrawRow("Shots", stats.shots, 3);
        DrawRow("MVPs", stats.mvps, 4);

    } else {
        // --- LAST MATCH VIEW (Default) ---
        std::string title = "MATCH COMPLETE";
        if (postMatch.overtime) title += " - OVERTIME";
        RSElement titleEl = CreateTextElement(title, winPos.x + 12.0f, winPos.y + 8.0f, teamHeaderFontSize / 13.0f, titleColor);
        RenderElement(dl, titleEl);

        // Match info line
        std::string matchInfo = postMatch.playlist + " | " + postMatch.myTeamName + " " +
                               std::to_string(postMatch.myScore) + " - " +
                               std::to_string(postMatch.oppScore) + " " + postMatch.oppTeamName;

        RSElement matchInfoEl = CreateTextElement(matchInfo, winPos.x + 12.0f, winPos.y + 45.0f, headerFontSize / 13.0f, titleColor);
        RenderElement(dl, matchInfoEl);

        // Column headers if enabled
        float contentY = winPos.y + 70.0f;
        if (showColumnHeaders) {
            ImU32 headerTextColor = ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, alpha));
            RenderElement(dl, CreateTextElement("Player", winPos.x + nameColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Score", winPos.x + scoreColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Goals", winPos.x + goalsColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Assists", winPos.x + assistsColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Saves", winPos.x + savesColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Shots", winPos.x + shotsColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Ping", winPos.x + pingColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            contentY += playerRowHeight;
        }

        // Render players by team
        for (int teamIdx = 0; teamIdx <= 1; teamIdx++) {
            // Team header
            bool isMyTeam = false;
            for (const auto& p : postMatch.players) {
                if (p.isLocal && p.teamIndex == teamIdx) {
                    isMyTeam = true;
                    break;
                }
            }

            std::string teamName = isMyTeam ? postMatch.myTeamName : postMatch.oppTeamName;
            int teamScore = isMyTeam ? postMatch.myScore : postMatch.oppScore;

            // Team section
            ImU32 teamColor;
            auto hsvToRgb = [](float h, float s, float v, float a) -> ImVec4 {
                float c = v * s;
                float x = c * (1 - abs(fmod(h / 60.0f, 2) - 1));
                float m = v - c;
                float r = 0, g = 0, b = 0;
                if (h < 60) { r = c; g = x; b = 0; }
                else if (h < 120) { r = x; g = c; b = 0; }
                else if (h < 180) { r = 0; g = c; b = x; }
                else if (h < 240) { r = 0; g = x; b = c; }
                else if (h < 300) { r = x; g = 0; b = c; }
                else { r = c; g = 0; b = x; }
                return ImVec4(r + m, g + m, b + m, a);
            };

            if (teamIdx == 0) {
                teamColor = ImGui::GetColorU32(hsvToRgb(plugin_->GetBlueTeamHue(), blueTeamSat, blueTeamVal, alpha));
            } else {
                teamColor = ImGui::GetColorU32(hsvToRgb(plugin_->GetOrangeTeamHue(), orangeTeamSat, orangeTeamVal, alpha));
            }

            std::string teamHeader = teamName;
            if (showTeamScores) {
                teamHeader += " - " + std::to_string(teamScore);
            }

            RSElement teamBg = CreateRectElement(winPos.x + sectionPadding, contentY, winSize.x - (sectionPadding * 2), teamHeaderHeight, teamColor, 4.0f, true);
            RenderElement(dl, teamBg);
            
            RSElement teamTitle = CreateTextElement(teamHeader, winPos.x + nameColumnX, contentY + 4.0f, teamHeaderFontSize / 13.0f, titleColor);
            RenderElement(dl, teamTitle);
            contentY += teamHeaderHeight + 4.0f;

            // Players in this team
            for (const auto& player : postMatch.players) {
                if (player.teamIndex != teamIdx) continue;

                ImU32 playerColor = player.isMVP && showMvpGlow ?
                                  ImGui::GetColorU32(ImVec4(1.f, 0.84f, 0.f, alpha)) : titleColor;

                // MVP indicator
                if (player.isMVP) {
                    RSElement star = CreateTextElement("★", winPos.x + nameColumnX - 20.0f, contentY, (mainFontSize * mvpCheckmarkSize) / 13.0f, ImGui::GetColorU32(ImVec4(1.f, 0.84f, 0.f, alpha)));
                    RenderElement(dl, star);
                }

                // Player stats
                std::map<std::string, std::string> vars;
                vars["Name"] = player.name;
                RSElement nameEl = CreateTextElement("{{Name}}", winPos.x + nameColumnX, contentY, mainFontSize / 13.0f, playerColor);
                OverlayUtils::ReplaceVars(nameEl.value, vars);
                RenderElement(dl, nameEl);
                
                RenderElement(dl, CreateTextElement(std::to_string(player.score), winPos.x + scoreColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.goals), winPos.x + goalsColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.assists), winPos.x + assistsColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.saves), winPos.x + savesColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.shots), winPos.x + shotsColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.ping), winPos.x + pingColumnX, contentY, mainFontSize / 13.0f, playerColor));

                contentY += playerRowHeight;
            }

            contentY += teamSectionSpacing;
        }
    }
}

void OverlayRenderer::RenderElement(ImDrawList* dl, const RSElement& element) {
    int rotate_idx = 0;
    if (element.rotate_enable) OverlayUtils::ImRotateStart(dl, rotate_idx);

    if (element.fill.enable) {
        if (element.type == "rectangle") {
            dl->AddRectFilled(element.positions[0], element.positions[1], element.fill.color, element.size.x);
        }
    }

    if (element.stroke.enable) {
        if (element.type == "rectangle") {
            dl->AddRect(element.positions[0], element.positions[1], element.stroke.color, element.size.x, 0, element.scale);
        }
    }

    if (element.type == "text") {
        ImGui::SetWindowFontScale(element.scale);
        dl->AddText(element.positions[0], element.color.color, element.value.c_str());
        ImGui::SetWindowFontScale(1.0f);
    }

    if (element.rotate_enable) OverlayUtils::ImRotateEnd(dl, element.rotate, rotate_idx);
}

RSElement OverlayRenderer::CreateTextElement(std::string text, float x, float y, float scale, ImU32 color) {
    RSElement el;
    el.type = "text";
    el.value = text;
    el.positions.push_back(ImVec2(x, y));
    el.scale = scale;
    el.color = { true, color };
    return el;
}

RSElement OverlayRenderer::CreateRectElement(float x, float y, float w, float h, ImU32 color, float rounding, bool filled) {
    RSElement el;
    el.type = "rectangle";
    el.positions.push_back(ImVec2(x, y));
    el.positions.push_back(ImVec2(x + w, y + h));
    el.size = ImVec2(rounding, 0); // using x for rounding
    
    if (filled) {
        el.fill = { true, color };
    } else {
        el.stroke = { true, color };
        el.scale = 1.0f; // stroke thickness
    }
    return el;
}