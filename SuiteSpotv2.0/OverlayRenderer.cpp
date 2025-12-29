#include "pch.h"

#include "OverlayRenderer.h"
#include "SuiteSpot.h"
#include "ThemeManager.h"

#include <algorithm>
#include <cmath>

OverlayRenderer::OverlayRenderer(SuiteSpot* plugin) : plugin_(plugin) {
    lastFrameTime = std::chrono::steady_clock::now();
}

// ===== MAIN RENDER ENTRY POINT (RocketStats pattern) =====
void OverlayRenderer::Render(const ImVec2& displaySize) {
    if (!plugin_) return;

    this->displaySize = displaySize;  // Update cache

    // 1. VISIBILITY CHECK (RocketStats pattern)
    auto& postMatch = plugin_->GetPostMatchInfo();
    bool show_overlay = postMatch.active || plugin_->testOverlayActive;

    if (!show_overlay) {
        spawnOpacity = 0.0f;  // Reset for next activation
        return;
    }

    // 2. CREATE INVISIBLE WINDOW (RocketStats pattern)
    ImGui::SetNextWindowPos({ 0.f, 0.f }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("##SuiteSpotOverlay", (bool*)1,
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoMouseInputs |
        ImGuiWindowFlags_NoFocusOnAppearing);

    // 3. CALCULATE OPTIONS (RocketStats pattern)
    const ImVec2 overlaySize = ImVec2(std::max(400.0f, plugin_->GetOverlayWidth()),
                                      std::max(180.0f, plugin_->GetOverlayHeight()));
    
    float overlayX = 0.5f;  // Center horizontally (50%)
    float overlayY = 0.08f; // 8% from top
    float opacity = spawnOpacity * plugin_->GetOverlayAlpha();

    // Use CVar-backed offsets if needed
    float offsetX = plugin_->GetOverlayOffsetX();
    float offsetY = plugin_->GetOverlayOffsetY();

    // Convert normalized coords to pixels (RocketStats pattern)
    int posX = int(overlayX * displaySize.x - overlaySize.x * 0.5f + offsetX);
    int posY = int(overlayY * displaySize.y + offsetY);

    // 4. GET DRAW LIST (RocketStats pattern)
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    if (!dl) {
        ImGui::End();
        return;
    }

    // 5. RENDER OVERLAY CONTENT
    RenderOverlayContent(dl, ImVec2(posX, posY), overlaySize, opacity);

    // 6. RENDER MOVE HANDLE (RocketStats pattern)
    if (plugin_->IsActiveOverlay()) {
        RenderMoveHandle(dl, ImVec2(posX, posY), overlaySize);
    }

    // 7. SPAWN ANIMATION (RocketStats pattern)
    if (spawnOpacity < 1.f) {
        spawnOpacity += 0.05f;
        if (spawnOpacity > 1.f) spawnOpacity = 1.f;
    }

    // 7. CLOSE WINDOW (RocketStats pattern)
    ImGui::End();
}

void OverlayRenderer::ResetDefaults() {
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

void OverlayRenderer::RenderOverlayContent(ImDrawList* dl, ImVec2 pos, ImVec2 size, float opacity) {
    if (!plugin_ || !dl) return;

    auto& postMatch = plugin_->GetPostMatchInfo();

    // --- NEW THEME SYSTEM INTEGRATION ---
    if (auto* tm = plugin_->GetThemeManager()) {

        // 1. Prepare Variables
        std::map<std::string, std::string> vars;
        vars["MyScore"] = std::to_string(postMatch.myScore);
        vars["OppScore"] = std::to_string(postMatch.oppScore);
        vars["MyTeam"] = postMatch.myTeamName;
        vars["OppTeam"] = postMatch.oppTeamName;
        vars["Playlist"] = postMatch.playlist;

        // 2. Prepare Options
        RSOptions options;
        options.x = pos.x;
        options.y = pos.y;
        options.width = size.x;
        options.height = size.y;
        options.scale = 1.0f; // Global scale
        options.opacity = opacity;

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
    ImU32 baseBg = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, backgroundAlpha * opacity));
    RSElement bgRect = CreateRectElement(pos.x, pos.y, size.x, size.y, baseBg, 8.0f, true);
    RenderElement(dl, bgRect);

    // Draw header
    ImU32 headerBg = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, headerAlpha * opacity));
    RSElement headerRect = CreateRectElement(pos.x, pos.y, size.x, 34.0f, headerBg, 8.0f, true);
    RenderElement(dl, headerRect);

    ImU32 titleColor = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, opacity));
    int mode = plugin_->GetOverlayMode();

    if (mode == 1) {
        // --- SESSION STATS VIEW ---
        RSElement sessionTitle = CreateTextElement("SESSION SUMMARY", pos.x + 12.0f, pos.y + 8.0f, teamHeaderFontSize / 13.0f, titleColor);
        RenderElement(dl, sessionTitle);
        
        auto& stats = plugin_->sessionStats;
        std::string subHeader = "Matches: " + std::to_string(stats.matchesPlayed) + 
                              " | Wins: " + std::to_string(stats.wins) + 
                              " | Losses: " + std::to_string(stats.losses);
        RSElement statsHeader = CreateTextElement(subHeader, pos.x + 12.0f, pos.y + 45.0f, headerFontSize / 13.0f, titleColor);
        RenderElement(dl, statsHeader);

        // Simple table for session stats
        float startY = pos.y + 80.0f;
        float labelX = pos.x + 50.0f;
        float valueX = pos.x + 250.0f;
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
        RSElement titleEl = CreateTextElement(title, pos.x + 12.0f, pos.y + 8.0f, teamHeaderFontSize / 13.0f, titleColor);
        RenderElement(dl, titleEl);

        // Match info line
        std::string matchInfo = postMatch.playlist + " | " + postMatch.myTeamName + " " +
                               std::to_string(postMatch.myScore) + " - " +
                               std::to_string(postMatch.oppScore) + " " + postMatch.oppTeamName;

        RSElement matchInfoEl = CreateTextElement(matchInfo, pos.x + 12.0f, pos.y + 45.0f, headerFontSize / 13.0f, titleColor);
        RenderElement(dl, matchInfoEl);

        // Column headers if enabled
        float contentY = pos.y + 70.0f;
        if (showColumnHeaders) {
            ImU32 headerTextColor = ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, opacity));
            RenderElement(dl, CreateTextElement("Player", pos.x + nameColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Score", pos.x + scoreColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Goals", pos.x + goalsColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Assists", pos.x + assistsColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Saves", pos.x + savesColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Shots", pos.x + shotsColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
            RenderElement(dl, CreateTextElement("Ping", pos.x + pingColumnX, contentY, headerFontSize / 13.0f, headerTextColor));
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
                teamColor = ImGui::GetColorU32(hsvToRgb(plugin_->GetBlueTeamHue(), blueTeamSat, blueTeamVal, opacity));
            } else {
                teamColor = ImGui::GetColorU32(hsvToRgb(plugin_->GetOrangeTeamHue(), orangeTeamSat, orangeTeamVal, opacity));
            }

            std::string teamHeader = teamName;
            if (showTeamScores) {
                teamHeader += " - " + std::to_string(teamScore);
            }

            RSElement teamBg = CreateRectElement(pos.x + sectionPadding, contentY, size.x - (sectionPadding * 2), teamHeaderHeight, teamColor, 4.0f, true);
            RenderElement(dl, teamBg);
            
            RSElement teamTitle = CreateTextElement(teamHeader, pos.x + nameColumnX, contentY + 4.0f, teamHeaderFontSize / 13.0f, titleColor);
            RenderElement(dl, teamTitle);
            contentY += teamHeaderHeight + 4.0f;

            // Players in this team
            for (const auto& player : postMatch.players) {
                if (player.teamIndex != teamIdx) continue;

                ImU32 playerColor = player.isMVP && showMvpGlow ?
                                  ImGui::GetColorU32(ImVec4(1.f, 0.84f, 0.f, opacity)) : titleColor;

                // MVP indicator
                if (player.isMVP) {
                    RSElement star = CreateTextElement("★", pos.x + nameColumnX - 20.0f, contentY, (mainFontSize * mvpCheckmarkSize) / 13.0f, ImGui::GetColorU32(ImVec4(1.f, 0.84f, 0.f, opacity)));
                    RenderElement(dl, star);
                }

                // Player stats
                std::map<std::string, std::string> vars;
                vars["Name"] = player.name;
                RSElement nameEl = CreateTextElement("{{Name}}", pos.x + nameColumnX, contentY, mainFontSize / 13.0f, playerColor);
                OverlayUtils::ReplaceVars(nameEl.value, vars);
                RenderElement(dl, nameEl);
                
                RenderElement(dl, CreateTextElement(std::to_string(player.score), pos.x + scoreColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.goals), pos.x + goalsColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.assists), pos.x + assistsColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.saves), pos.x + savesColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.shots), pos.x + shotsColumnX, contentY, mainFontSize / 13.0f, playerColor));
                RenderElement(dl, CreateTextElement(std::to_string(player.ping), pos.x + pingColumnX, contentY, mainFontSize / 13.0f, playerColor));

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

// ===== OVERLAY MOVE HANDLE (RocketStats pattern) =====
void OverlayRenderer::RenderMoveHandle(ImDrawList* dl, const ImVec2& pos, const ImVec2& overlaySize) {
    if (!plugin_ || !dl) return;

    float margin = 10.0f;
    float rectSize = 10.0f;
    bool mouseClick = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
    ImVec2 mousePos = ImGui::GetIO().MousePos;

    // Get current overlay position from CVars
    float overlayX = plugin_->GetOverlayOffsetX();
    float overlayY = plugin_->GetOverlayOffsetY();
    ImVec2 overlayPos = ImVec2(pos.x + overlayX, pos.y + overlayY);
    ImVec2 rectPos = overlayPos;

    // Check if mouse is hovering over the handle
    bool hover = (mousePos.x > (overlayPos.x - rectSize - margin * 2) &&
                  mousePos.x < (overlayPos.x + rectSize + margin * 2));
    hover = hover && (mousePos.y > (overlayPos.y - rectSize - margin * 2) &&
                      mousePos.y < (overlayPos.y + rectSize + margin * 2));

    // Handle mouse interaction
    if (hover || overlayMoving) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        if (mouseClick) {
            if (!overlayMoving) {
                // Start moving
                overlayMoving = true;
                moveOrigin = mousePos;
            }

            moveCursor = mousePos;
            rectPos = moveCursor;
        } else {
            // Mouse released - apply new position
            if (overlayMoving) {
                // Calculate delta and apply
                float deltaX = moveCursor.x - moveOrigin.x;
                float deltaY = moveCursor.y - moveOrigin.y;

                // Update the offset and sync to CVars (RocketStats pattern)
                float newX = overlayX + deltaX;
                float newY = overlayY + deltaY;
                
                if (plugin_->cvarManager) {
                    plugin_->cvarManager->getCvar("overlay_offset_x").setValue(newX);
                    plugin_->cvarManager->getCvar("overlay_offset_y").setValue(newY);
                }

                overlayMoving = false;
            } else {
                ImGui::SetTooltip("Drag to move overlay");
            }
        }
    }

    // Draw the move handle
    ImU32 fillColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
    ImU32 strokeColor = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

    dl->AddRectFilled(
        ImVec2(rectPos.x - rectSize, rectPos.y - rectSize),
        ImVec2(rectPos.x + rectSize, rectPos.y + rectSize),
        fillColor, 4.0f
    );
    dl->AddRect(
        ImVec2(rectPos.x - rectSize, rectPos.y - rectSize),
        ImVec2(rectPos.x + rectSize, rectPos.y + rectSize),
        strokeColor, 4.0f, 0, 2.0f
    );
}