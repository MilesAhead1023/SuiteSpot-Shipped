#include "pch.h"
#include "SuiteSpot.h"
#include "MapList.h"
#include "MapManager.h"
#include "SettingsSync.h"
#include "AutoLoadFeature.h"
#include "TrainingPackManager.h"
#include "SettingsUI.h"
#include "TrainingPackUI.h"
#include "LoadoutUI.h"
#include "OverlayRenderer.h"
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>

// ===== SuiteSpot persistence helpers =====
std::filesystem::path SuiteSpot::GetDataRoot() const {
    return mapManager ? mapManager->GetDataRoot() : std::filesystem::path();
}

std::filesystem::path SuiteSpot::GetSuiteTrainingDir() const {
    return mapManager ? mapManager->GetSuiteTrainingDir() : std::filesystem::path();
}

void SuiteSpot::EnsureDataDirectories() const {
    if (mapManager) {
        mapManager->EnsureDataDirectories();
    }
}

int SuiteSpot::GetRandomTrainingIndex() const {
    if (!trainingPackMgr || !mapManager) return -1;
    auto shuffleBag = trainingPackMgr->GetShuffleBagPacks();
    return mapManager->GetRandomTrainingMapIndex(shuffleBag);
}

std::filesystem::path SuiteSpot::GetWorkshopLoaderConfigPath() const {
    return mapManager ? mapManager->GetWorkshopLoaderConfigPath() : std::filesystem::path();
}

std::filesystem::path SuiteSpot::ResolveConfiguredWorkshopRoot() const {
    return mapManager ? mapManager->ResolveConfiguredWorkshopRoot() : std::filesystem::path();
}

void SuiteSpot::DiscoverWorkshopInDir(const std::filesystem::path& dir) {
    if (mapManager) {
        mapManager->DiscoverWorkshopInDir(dir, RLWorkshop);
    }
}

void SuiteSpot::LoadWorkshopMaps() {
    if (mapManager) {
        int index = settingsSync ? settingsSync->GetCurrentWorkshopIndex() : 0;
        mapManager->LoadWorkshopMaps(RLWorkshop, index);
        if (settingsSync) {
            settingsSync->SetCurrentWorkshopIndex(index);
        }
    }
}

void SuiteSpot::SaveWorkshopMaps() const {
    // No-op
}

// ===== TRAINING PACK SCRAPER INTEGRATION =====
bool SuiteSpot::IsEnabled() const {
    return settingsSync ? settingsSync->IsEnabled() : false;
}

bool SuiteSpot::IsAutoQueueEnabled() const {
    return settingsSync ? settingsSync->IsAutoQueue() : false;
}

bool SuiteSpot::IsTrainingShuffleEnabled() const {
    return settingsSync ? settingsSync->IsTrainingShuffleEnabled() : false;
}

int SuiteSpot::GetMapType() const {
    return settingsSync ? settingsSync->GetMapType() : 0;
}

int SuiteSpot::GetDelayQueueSec() const {
    return settingsSync ? settingsSync->GetDelayQueueSec() : 0;
}

int SuiteSpot::GetDelayFreeplaySec() const {
    return settingsSync ? settingsSync->GetDelayFreeplaySec() : 0;
}

int SuiteSpot::GetDelayTrainingSec() const {
    return settingsSync ? settingsSync->GetDelayTrainingSec() : 0;
}

int SuiteSpot::GetDelayWorkshopSec() const {
    return settingsSync ? settingsSync->GetDelayWorkshopSec() : 0;
}

int SuiteSpot::GetCurrentIndex() const {
    return settingsSync ? settingsSync->GetCurrentIndex() : 0;
}

int SuiteSpot::GetCurrentTrainingIndex() const {
    return settingsSync ? settingsSync->GetCurrentTrainingIndex() : 0;
}

int SuiteSpot::GetCurrentWorkshopIndex() const {
    return settingsSync ? settingsSync->GetCurrentWorkshopIndex() : 0;
}

int SuiteSpot::GetTrainingBagSize() const {
    return settingsSync ? settingsSync->GetTrainingBagSize() : 0;
}

float SuiteSpot::GetPostMatchDurationSec() const {
    return settingsSync ? settingsSync->GetPostMatchDurationSec() : 15.0f;
}

float SuiteSpot::GetOverlayWidth() const {
    return settingsSync ? settingsSync->GetOverlayWidth() : 880.0f;
}

float SuiteSpot::GetOverlayHeight() const {
    return settingsSync ? settingsSync->GetOverlayHeight() : 400.0f;
}

float SuiteSpot::GetOverlayAlpha() const {
    return settingsSync ? settingsSync->GetOverlayAlpha() : 0.85f;
}

float SuiteSpot::GetBlueTeamHue() const {
    return settingsSync ? settingsSync->GetBlueTeamHue() : 240.0f;
}

float SuiteSpot::GetOrangeTeamHue() const {
    return settingsSync ? settingsSync->GetOrangeTeamHue() : 25.0f;
}

float SuiteSpot::GetOverlayOffsetX() const {
    return settingsSync ? settingsSync->GetOverlayOffsetX() : 0.0f;
}

float SuiteSpot::GetOverlayOffsetY() const {
    return settingsSync ? settingsSync->GetOverlayOffsetY() : 0.0f;
}

int SuiteSpot::GetOverlayMode() const {
    return settingsSync ? settingsSync->GetOverlayMode() : 0;
}

std::filesystem::path SuiteSpot::GetTrainingPacksPath() const {
    return GetSuiteTrainingDir() / "prejump_packs.json";
}

bool SuiteSpot::IsTrainingPackCacheStale() const {
    return trainingPackMgr ? trainingPackMgr->IsCacheStale(GetTrainingPacksPath()) : true;
}

std::string SuiteSpot::FormatLastUpdatedTime() const {
    return trainingPackMgr ? trainingPackMgr->GetLastUpdatedTime(GetTrainingPacksPath()) : "Unknown";
}

void SuiteSpot::LoadTrainingPacksFromFile(const std::filesystem::path& filePath) {
    if (trainingPackMgr) {
        trainingPackMgr->LoadPacksFromFile(filePath);
    }
}

// #detailed comments: ScrapeAndLoadTrainingPacks
// Purpose: Launches an external PowerShell script to scrape online source
// and write a JSON cache to disk. This is intentionally performed in a
// background task (scheduled via gameWrapper->SetTimeout) to avoid any
// blocking on the UI/game thread.
//
// Safety and behavior notes:
//  - scrapingInProgress is a guard flag ensuring only one scrape
//    runs at a time. It is set before scheduling and cleared when the
//    background process finishes.
//  - The implementation uses system() and relies on the platform's
//    default process creation semantics; this must remain as-is for
//    portability with existing deployments. If this is changed to a
//    more advanced process API, ensure identical detach/exit semantics.
//  - The script path is hard-coded to the repo dev path; callers should
//    ensure that the script is present when invoking this routine.
//
// DO NOT CHANGE: Modifying timing (the 0.1f scheduling) or the way the
// result is checked could resurface race conditions that previously
// required this exact coordination.
void SuiteSpot::ScrapeAndLoadTrainingPacks() {
    if (trainingPackMgr) {
        trainingPackMgr->ScrapeAndLoadTrainingPacks(GetTrainingPacksPath(), gameWrapper);
    }
}


using namespace std;
using namespace std::chrono_literals;

BAKKESMOD_PLUGIN(SuiteSpot, "SuiteSpot", plugin_version, PLUGINTYPE_FREEPLAY)

shared_ptr<CVarManagerWrapper> _globalCvarManager;

void SuiteSpot::LoadHooks() {
    // ===== MATCH EVENT HOOKS =====
    // Re-queue/transition at match end or when main menu appears after a match
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", bind(&SuiteSpot::GameEndedEvent, this, placeholders::_1));
    gameWrapper->HookEvent("Function TAGame.AchievementManager_TA.HandleMatchEnded", bind(&SuiteSpot::GameEndedEvent, this, placeholders::_1));

    // ===== MENU STATE HOOKS (RocketStats pattern) =====
    // Main menu detection - fires when player enters main menu
    gameWrapper->HookEvent("Function TAGame.GFxData_MainMenu_TA.OnEnteredMainMenu", [this](std::string) {
        OnEnteredMainMenu();
    });

    // Menu stack tracking - tracks menu depth for overlay visibility
    gameWrapper->HookEvent("Function TAGame.GFxData_MenuStack_TA.PushMenu", [this](std::string) {
        OnPushMenu();
    });

    gameWrapper->HookEvent("Function TAGame.GFxData_MenuStack_TA.PopMenu", [this](std::string) {
        OnPopMenu();
    });

    // Game start detection - countdown begins (note: no TAGame. prefix)
    gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState", [this](std::string) {
        OnGameStart();
    });

    // Scoreboard hooks
    gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", [this](std::string) {
        OnOpenScoreboard();
    });

    gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", [this](std::string) {
        OnCloseScoreboard();
    });

}

// ===== MENU STATE HANDLERS (RocketStats pattern) =====
void SuiteSpot::OnEnteredMainMenu() {
    menuState.menu_stack = 0;
    menuState.is_in_menu = true;
    menuState.is_in_MainMenu = true;
    menuState.is_in_game = false;
    menuState.is_in_freeplay = false;
    menuState.is_in_scoreboard = false;

    // Switch to menu theme if dual-theme enabled (RocketStats pattern)
    if (themeManager) {
        themeManager->SwitchToMenuTheme();
    }

    LOG("SuiteSpot: Entered main menu");
}

void SuiteSpot::OnPushMenu() {
    menuState.menu_stack++;
    menuState.is_in_menu = true;
}

void SuiteSpot::OnPopMenu() {
    menuState.menu_stack--;
    if (menuState.menu_stack <= 0) {
        menuState.menu_stack = 0;
        menuState.is_in_menu = false;
    }
}

void SuiteSpot::OnGameStart() {
    menuState.is_in_MainMenu = false;
    menuState.is_in_menu = false;
    menuState.is_in_game = true;
    menuState.is_in_freeplay = gameWrapper->IsInFreeplay();
    menuState.is_online_game = gameWrapper->IsInOnlineGame();
    menuState.is_offline_game = gameWrapper->IsInGame() && !menuState.is_online_game;

    // Switch to game theme if dual-theme enabled (RocketStats pattern)
    if (themeManager) {
        themeManager->SwitchToGameTheme();
    }

    LOG("SuiteSpot: Game started (online={}, freeplay={})", menuState.is_online_game, menuState.is_in_freeplay);
}

void SuiteSpot::OnOpenScoreboard() {
    menuState.is_in_scoreboard = true;
}

void SuiteSpot::OnCloseScoreboard() {
    menuState.is_in_scoreboard = false;
}

// ===== OVERLAY VISIBILITY LOGIC (RocketStats pattern) =====
bool SuiteSpot::ShouldShowOverlay() const {
    // Post-match overlay has its own active flag
    if (!postMatch.active) return false;

    // Test mode bypasses menu state checks
    if (testOverlayActive) return true;

    // Check visibility settings based on current state
    bool showInMenu = IsOverlayEnabledInMenu() && menuState.is_in_menu;
    bool showInGame = IsOverlayEnabledInGame() && menuState.is_in_game &&
                      (!menuState.is_in_scoreboard || menuState.is_in_freeplay);
    bool showInScoreboard = IsOverlayEnabledInScoreboard() && menuState.is_in_scoreboard &&
                            !menuState.is_in_freeplay;

    return showInMenu || showInGame || showInScoreboard;
}

bool SuiteSpot::IsOverlayEnabledInMenu() const {
    return settingsSync ? settingsSync->IsOverlayEnabledInMenu() : true;
}

bool SuiteSpot::IsOverlayEnabledInGame() const {
    return settingsSync ? settingsSync->IsOverlayEnabledInGame() : true;
}

bool SuiteSpot::IsOverlayEnabledInScoreboard() const {
    return settingsSync ? settingsSync->IsOverlayEnabledInScoreboard() : false;
}

// #detailed comments: GameEndedEvent
// Purpose: Called by hooked game events when a match ends. The function
// must capture final match state (team scores, PRIs) as quickly as
// possible and prepare the post-match overlay and optional subsequent
// automation (map load, queueing). This must be robust: exceptions
// are caught and logged to avoid destabilizing the host.
//
// Timing and ordering notes:
//  - Capture scores and PRIs immediately; the game may transition state
//    between frames so delaying capture risks losing final values.
//  - The postMatch.start timestamp is recorded with steady_clock so
//    overlay lifetime calculations are not affected by system clock
//    adjustments.
//
// DO NOT CHANGE: The safeExecute lambda intentionally accepts a delay
// (in seconds) and either executes immediately or schedules via
// gameWrapper->SetTimeout. Changing its semantics will alter when
// external commands (load_freeplay, queue, etc.) are run relative to
// overlay presentation.
void SuiteSpot::GameEndedEvent(std::string name) {
    LOG("SuiteSpot: GameEndedEvent triggered by hook: {}", name);
    
    // 1. Run Auto-Load/Queue Logic first (Independent of overlay)
    if (autoLoadFeature && settingsSync) {
        LOG("SuiteSpot: Triggering AutoLoadFeature::OnMatchEnded");
        // Get shuffle bag from TrainingPackManager
        std::vector<TrainingEntry> shuffleBag = trainingPackMgr ? trainingPackMgr->GetShuffleBagPacks() : std::vector<TrainingEntry>();
        autoLoadFeature->OnMatchEnded(gameWrapper, cvarManager, RLMaps, RLTraining, RLWorkshop,
            shuffleBag, *settingsSync);
    }

    // 2. Prepare Overlay Data
    if (!IsEnabled()) {
        LOG("SuiteSpot: Skipping overlay data capture because plugin is DISABLED");
        return;
    }

    // Capture final scores before any transitions
    ServerWrapper server = gameWrapper->GetGameEventAsServer();
    if (server.IsNull()) {
        LOG("SuiteSpot: ServerWrapper is null, cannot capture match stats");
        return;
    }

    try {
        TeamWrapper myTeam(static_cast<uintptr_t>(0));
        TeamWrapper oppTeam(static_cast<uintptr_t>(0));
        int myTeamIndex = -1;

        // Identify local player's team
        try {
            auto pc = gameWrapper->GetPlayerController();
            if (!pc.IsNull()) {
                auto pri = pc.GetPRI();
                if (!pri.IsNull()) {
                    auto team = pri.GetTeam();
                    if (!team.IsNull()) {
                        myTeamIndex = team.GetTeamIndex();
                    }
                }
            }
        } catch (...) {
            LOG("SuiteSpot: Error identifying local team index");
        }

        // Capture Team Wrappers
        try {
            auto teams = server.GetTeams();
            for (int i = 0; i < teams.Count(); ++i) {
                TeamWrapper tw = teams.Get(i);
                if (tw.IsNull()) continue;
                if (tw.GetTeamIndex() == myTeamIndex) {
                    myTeam = tw;
                } else if (oppTeam.IsNull()) {
                    oppTeam = tw;
                }
            }
            
            // Fallback: If we couldn't match myTeamIndex, just take first two
            if (myTeam.IsNull() && teams.Count() > 0) myTeam = teams.Get(0);
            if (oppTeam.IsNull() && teams.Count() > 1) oppTeam = teams.Get(1);
        } catch (...) {
            LOG("SuiteSpot: Error capturing team wrappers");
        }

        if (myTeam.IsNull() || oppTeam.IsNull()) {
             LOG("SuiteSpot: Could not find valid teams, skipping overlay data capture");
             return;
        }

        auto nameFromTeam = [](TeamWrapper& t, const std::string& fallback) {
            if (t.IsNull()) return fallback;
            std::string result = fallback;
            try {
                auto custom = t.GetCustomTeamName();
                if (!custom.IsNull()) {
                    auto str = custom.ToString();
                    if (!str.empty()) result = str;
                } else {
                    auto baseName = t.GetTeamName();
                    if (!baseName.IsNull()) {
                        auto str = baseName.ToString();
                        if (!str.empty()) result = str;
                    }
                }
            } catch (...) {}
            return result;
        };

        // Populate PostMatchInfo
        postMatch.players.clear();
        try {
            auto pris = server.GetPRIs();
            for (int i = 0; i < pris.Count(); ++i) {
                PriWrapper pri = pris.Get(i);
                if (pri.IsNull()) continue;

                PostMatchPlayerRow row;
                try {
                    auto team = pri.GetTeam();
                    row.teamIndex = team.IsNull() ? -1 : team.GetTeamIndex();
                    row.isLocal = pri.IsLocalPlayerPRI();

                    auto pName = pri.GetPlayerName();
                    row.name = pName.IsNull() ? "Unknown" : pName.ToString();
                    
                    row.score = pri.GetMatchScore();
                    row.goals = pri.GetMatchGoals();
                    row.assists = pri.GetMatchAssists();
                    row.saves = pri.GetMatchSaves();
                    row.shots = pri.GetMatchShots();

                    // Network quality
                    unsigned char netQuality = pri.GetReplicatedWorstNetQualityBeyondLatency();
                    row.ping = static_cast<int>(netQuality * 2); 
                    row.isMVP = false; // Calculated later

                    postMatch.players.push_back(row);
                } catch (...) {
                    continue; // Skip bad PRI
                }
            }
        } catch (...) {
            LOG("SuiteSpot: Error capturing PRIs");
        }

        // Sort players
        std::sort(postMatch.players.begin(), postMatch.players.end(), [](const PostMatchPlayerRow& a, const PostMatchPlayerRow& b) {
            if (a.teamIndex != b.teamIndex) return a.teamIndex < b.teamIndex;
            if (a.score != b.score) return a.score > b.score;
            return a.name < b.name;
        });

        // Calculate MVP (Highest score per team)
        std::map<int, int> teamHighScores;
        for (auto& row : postMatch.players) {
            if (teamHighScores.find(row.teamIndex) == teamHighScores.end() || row.score > teamHighScores[row.teamIndex]) {
                teamHighScores[row.teamIndex] = row.score;
            }
        }
        for (auto& row : postMatch.players) {
            row.isMVP = (row.score == teamHighScores[row.teamIndex] && row.score > 0);
        }

        // Final Match Details
        postMatch.myScore = !myTeam.IsNull() ? myTeam.GetScore() : 0;
        postMatch.oppScore = !oppTeam.IsNull() ? oppTeam.GetScore() : 0;
        postMatch.myTeamName = nameFromTeam(myTeam, "My Team");
        postMatch.oppTeamName = nameFromTeam(oppTeam, "Opponents");
        postMatch.playlist = server.GetMatchTypeName();
        postMatch.overtime = !!server.GetbOverTime();
        
        if (!myTeam.IsNull()) postMatch.myColor = myTeam.GetFontColor();
        if (!oppTeam.IsNull()) postMatch.oppColor = oppTeam.GetFontColor();

        // Update Session Stats
        sessionStats.matchesPlayed++;
        if (postMatch.myScore > postMatch.oppScore) sessionStats.wins++;
        else sessionStats.losses++;

        // Find local player stats
        for (const auto& p : postMatch.players) {
            if (p.isLocal) {
                sessionStats.goals += p.goals;
                sessionStats.assists += p.assists;
                sessionStats.saves += p.saves;
                sessionStats.shots += p.shots;
                if (p.isMVP) sessionStats.mvps++;
                break;
            }
        }

        postMatch.start = std::chrono::steady_clock::now();
        postMatch.active = true;
        
        LOG("SuiteSpot: Post-match overlay activated - {} ({}) vs {} ({})",
            postMatch.myTeamName, postMatch.myScore, postMatch.oppTeamName, postMatch.oppScore);

    } catch (const std::exception& e) {
        LOG("SuiteSpot: Critical error in GameEndedEvent: {}", e.what());
    } catch (...) {
        LOG("SuiteSpot: Unknown error in GameEndedEvent");
    }
}

void SuiteSpot::onLoad() {
    _globalCvarManager = cvarManager;
    LOG("SuiteSpot loaded");
    mapManager = new MapManager();
    settingsSync = new SettingsSync();
    autoLoadFeature = new AutoLoadFeature();
    trainingPackMgr = new TrainingPackManager();
    settingsUI = new SettingsUI(this);
    trainingPackUI = new TrainingPackUI(this);
    loadoutUI = new LoadoutUI(this);
    overlayRenderer = new OverlayRenderer(this);
    themeManager = new ThemeManager(this);
    EnsureDataDirectories();
    LoadWorkshopMaps();
    themeManager->LoadThemes();
    
    // Initialize LoadoutManager
    loadoutManager = std::make_unique<LoadoutManager>(gameWrapper);
    LOG("SuiteSpot: LoadoutManager initialized");
    
    // Check Pack cache and load if available
    if (trainingPackMgr) {
        if (!std::filesystem::exists(GetTrainingPacksPath())) {
            LOG("SuiteSpot: No Pack cache found. Schedule scraping on next opportunity.");
            // Will be scraped on first Settings render or user request
        } else {
            // Load existing Pack cache
            trainingPackMgr->LoadPacksFromFile(GetTrainingPacksPath());
            LOG("SuiteSpot: Pack cache loaded");
        }
    }
    
    LoadHooks();

    // NOTE: Overlay rendering is handled through PluginWindow::Render()
    // Do NOT use RegisterDrawable for ImGui overlays (causes crashes)

    // Register test overlay toggle
    cvarManager->registerNotifier("ss_testoverlay", [this](std::vector<std::string> args) {
        ToggleTestOverlay();
    }, "Toggle the SuiteSpot test overlay", PERMISSION_ALL);

    if (settingsSync) {
        settingsSync->RegisterAllCVars(cvarManager);
        // Shuffle bag count is now managed by TrainingPackManager
        int shuffleBagSize = trainingPackMgr ? trainingPackMgr->GetShuffleBagCount() : 0;
        settingsSync->UpdateTrainingBagSize(shuffleBagSize, cvarManager);
    }
    
    LOG("SuiteSpot: Plugin initialization complete");
}

void SuiteSpot::ToggleTestOverlay() {
    // 1. Mock Match Data if empty
    if (postMatch.players.empty()) {
        postMatch.myTeamName = "Blue Team";
        postMatch.oppTeamName = "Orange Team";
        postMatch.myScore = 3;
        postMatch.oppScore = 2;
        postMatch.playlist = "Competitive Doubles";
        postMatch.overtime = false;
        
        PostMatchPlayerRow p1; p1.name = "LocalPlayer"; p1.score = 650; p1.goals = 2; p1.isLocal = true; p1.teamIndex = 0; p1.isMVP = true;
        PostMatchPlayerRow p2; p2.name = "Teammate"; p2.score = 400; p2.goals = 1; p2.isLocal = false; p2.teamIndex = 0;
        PostMatchPlayerRow p3; p3.name = "Opponent 1"; p3.score = 500; p3.goals = 1; p3.isLocal = false; p3.teamIndex = 1; p3.isMVP = true;
        PostMatchPlayerRow p4; p4.name = "Opponent 2"; p4.score = 300; p4.goals = 1; p4.isLocal = false; p4.teamIndex = 1;
        
        postMatch.players = { p1, p2, p3, p4 };
    }

    // 2. Mock Session Data if empty (and mode is Session Stats)
    if (GetOverlayMode() == 1 && sessionStats.matchesPlayed == 0) {
        sessionStats.matchesPlayed = 5;
        sessionStats.wins = 3;
        sessionStats.losses = 2;
        sessionStats.goals = 12;
        sessionStats.assists = 8;
        sessionStats.saves = 15;
        sessionStats.shots = 24;
        sessionStats.mvps = 2;
    }
    
    // 3. Toggle Visibility state (Actual ImGui calls happen in Render loop)
    if (!postMatch.active) {
        postMatch.start = std::chrono::steady_clock::now();
        postMatch.active = true;
        testOverlayActive = true;  // Bypass menu state checks
        LOG("SuiteSpot: Test overlay state -> ACTIVE");
    } else {
        postMatch.active = false;
        testOverlayActive = false;
        LOG("SuiteSpot: Test overlay state -> INACTIVE");
    }
}

// PluginWindow::Render() - Called every frame by BakkesMod (RocketStats pattern)
void SuiteSpot::Render() {
    // 1. Update display size FIRST (RocketStats line 6)
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    // 2. Update game state flags (RocketStats lines 8-11)
    menuState.is_online_game = gameWrapper->IsInOnlineGame();
    menuState.is_offline_game = gameWrapper->IsInGame();
    menuState.is_in_freeplay = gameWrapper->IsInFreeplay();
    menuState.is_in_game = (menuState.is_online_game || menuState.is_offline_game);

    // 3. ALWAYS call overlay renderer (visibility check inside)
    if (overlayRenderer) {
        overlayRenderer->Render(displaySize);  // Pass display size
    }
}

void SuiteSpot::onUnload() {
    // Match events
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
    gameWrapper->UnhookEvent("Function TAGame.AchievementManager_TA.HandleMatchEnded");

    // Menu state events (RocketStats pattern)
    gameWrapper->UnhookEvent("Function TAGame.GFxData_MainMenu_TA.OnEnteredMainMenu");
    gameWrapper->UnhookEvent("Function TAGame.GFxData_MenuStack_TA.PushMenu");
    gameWrapper->UnhookEvent("Function TAGame.GFxData_MenuStack_TA.PopMenu");
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.Countdown.BeginState");
    gameWrapper->UnhookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard");
    gameWrapper->UnhookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard");
    delete overlayRenderer;
    overlayRenderer = nullptr;
    delete themeManager;
    themeManager = nullptr;
    delete settingsUI;
    settingsUI = nullptr;
    delete trainingPackUI;
    trainingPackUI = nullptr;
    delete loadoutUI;
    loadoutUI = nullptr;
    delete trainingPackMgr;
    trainingPackMgr = nullptr;
    delete autoLoadFeature;
    autoLoadFeature = nullptr;
    delete settingsSync;
    settingsSync = nullptr;
    delete mapManager;
    mapManager = nullptr;
    LOG("SuiteSpot unloaded");
}
