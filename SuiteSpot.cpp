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

/*
 * ======================================================================================
 * SUITESPOT: IMPLEMENTATION DETAILS
 * ======================================================================================
 * 
 * This file contains the actual code for the plugin's lifecycle.
 * 
 * KEY SECTIONS:
 * 1. PERSISTENCE HELPERS: Tiny functions that ask the Managers for data paths (like where 
 *    to find Workshop maps).
 * 2. EVENT HOOKS: The `GameEndedEvent` function is the heartbeat of the automation. 
 *    It waits for the match to finish, then triggers the `AutoLoadFeature`.
 * 3. LOADING (onLoad):
 *    - Creates all the "Managers" (tools for Maps, Settings, Packs).
 *    - Registers the "Browser Window" logic (via togglemenu).
 *    - Hooks into the game events.
 * 4. RENDERING:
 *    - Handled natively by BakkesMod's PluginWindow interface.
 *    - Uses a custom OnClose override to ensure standalone windows stay persistent
 *      when the main settings menu is closed.
 */

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
        // Load workshop maps without passing an index - the path-based selection persists automatically
        int unused = 0;
        mapManager->LoadWorkshopMaps(RLWorkshop, unused);
    }
}

// ===== TRAINING PACK UPDATE INTEGRATION =====
bool SuiteSpot::IsEnabled() const {
    return settingsSync ? settingsSync->IsEnabled() : false;
}

bool SuiteSpot::IsAutoQueueEnabled() const {
    return settingsSync ? settingsSync->IsAutoQueue() : false;
}

bool SuiteSpot::IsBagRotationEnabled() const {
    return settingsSync ? settingsSync->IsBagRotationEnabled() : true;
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

std::string SuiteSpot::GetCurrentFreeplayCode() const {
    return settingsSync ? settingsSync->GetCurrentFreeplayCode() : "";
}

std::string SuiteSpot::GetCurrentTrainingCode() const {
    return settingsSync ? settingsSync->GetCurrentTrainingCode() : "";
}

std::string SuiteSpot::GetCurrentWorkshopPath() const {
    return settingsSync ? settingsSync->GetCurrentWorkshopPath() : "";
}

std::pair<TrainingEntry, std::string> SuiteSpot::AdvanceAndGetNextBagPack() {
    if (!trainingPackMgr || !settingsSync) return {{}, ""};

    // Get current bag and index from CVars
    auto currentBagCvar = cvarManager->getCvar("suitespot_current_bag");
    auto currentIdxCvar = cvarManager->getCvar("suitespot_current_bag_pack_index");

    std::string currentBag = currentBagCvar ? currentBagCvar.getStringValue() : "";
    int currentIdx = currentIdxCvar ? currentIdxCvar.getIntValue() : 0;

    // Get available bags
    const auto& bags = trainingPackMgr->GetAvailableBags();

    // Get packs in current bag (if any)
    std::vector<TrainingEntry> packsInBag;
    if (!currentBag.empty()) {
        packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
    }

    // Advance index
    currentIdx++;

    // If past end of current bag (or no current bag), move to next enabled bag
    if (currentIdx >= (int)packsInBag.size() || currentBag.empty()) {
        // Find current bag position and get next enabled bag
        bool foundCurrent = currentBag.empty();
        bool foundNext = false;

        for (const auto& bag : bags) {
            if (!bag.enabled) continue;
            int bagPackCount = trainingPackMgr->GetBagPackCount(bag.name);
            if (bagPackCount == 0) continue;

            if (foundCurrent) {
                // This is the next valid bag
                currentBag = bag.name;
                currentIdx = 0;
                packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
                foundNext = true;
                break;
            }

            if (bag.name == currentBag) {
                foundCurrent = true;
            }
        }

        // If we didn't find a next bag, wrap around to first enabled bag with packs
        if (!foundNext) {
            for (const auto& bag : bags) {
                if (bag.enabled && trainingPackMgr->GetBagPackCount(bag.name) > 0) {
                    currentBag = bag.name;
                    currentIdx = 0;
                    packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
                    break;
                }
            }
        }
    }

    // Update CVars
    if (currentBagCvar) currentBagCvar.setValue(currentBag);
    if (currentIdxCvar) currentIdxCvar.setValue(currentIdx);

    // Return the pack
    if (!packsInBag.empty() && currentIdx < (int)packsInBag.size()) {
        return { packsInBag[currentIdx], currentBag };
    }
    
    return { {}, "" };
}

std::pair<TrainingEntry, std::string> SuiteSpot::PeekNextBagPack() const {
    if (!trainingPackMgr || !settingsSync) return { {}, "" };

    // Get current bag and index from CVars
    std::string currentBag = settingsSync->GetCurrentBag();
    int currentIdx = settingsSync->GetCurrentBagPackIndex();

    // Get available bags
    const auto& bags = trainingPackMgr->GetAvailableBags();

    // Get packs in current bag (if any)
    std::vector<TrainingEntry> packsInBag;
    if (!currentBag.empty()) {
        packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
    }

    // Advance index (conceptually)
    currentIdx++;

    // If past end of current bag (or no current bag), move to next enabled bag
    if (currentIdx >= (int)packsInBag.size() || currentBag.empty()) {
        // Find current bag position and get next enabled bag
        bool foundCurrent = currentBag.empty();
        bool foundNext = false;

        for (const auto& bag : bags) {
            if (!bag.enabled) continue;
            int bagPackCount = trainingPackMgr->GetBagPackCount(bag.name);
            if (bagPackCount == 0) continue;

            if (foundCurrent) {
                // This is the next valid bag
                currentBag = bag.name;
                currentIdx = 0;
                packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
                foundNext = true;
                break;
            }

            if (bag.name == currentBag) {
                foundCurrent = true;
            }
        }

        // If we didn't find a next bag, wrap around to first enabled bag with packs
        if (!foundNext) {
            for (const auto& bag : bags) {
                if (bag.enabled && trainingPackMgr->GetBagPackCount(bag.name) > 0) {
                    currentBag = bag.name;
                    currentIdx = 0;
                    packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
                    break;
                }
            }
        }
    }

    // Return the pack
    if (!packsInBag.empty() && currentIdx < (int)packsInBag.size()) {
        return { packsInBag[currentIdx], currentBag };
    }

    return { {}, "" };
}

void SuiteSpot::AdvanceToNextBagPack() {
    auto [pack, bagName] = AdvanceAndGetNextBagPack();
    
    if (!pack.code.empty()) {
        std::string code = pack.code;
        std::string name = pack.name;

        gameWrapper->SetTimeout([this, code, name, bagName](GameWrapper* gw) {
            std::string cmd = "load_training " + code;
            cvarManager->executeCommand(cmd);
            LOG("SuiteSpot: Next bag pack: {} from {} ({})", name, bagName, code);
        }, 0.0f);
    } else {
        LOG("SuiteSpot: No packs available in any enabled bag");
    }
}

void SuiteSpot::RetreatToPreviousBagPack() {
    if (!trainingPackMgr || !settingsSync) return;

    // Get current bag and index from CVars
    auto currentBagCvar = cvarManager->getCvar("suitespot_current_bag");
    auto currentIdxCvar = cvarManager->getCvar("suitespot_current_bag_pack_index");

    std::string currentBag = currentBagCvar ? currentBagCvar.getStringValue() : "";
    int currentIdx = currentIdxCvar ? currentIdxCvar.getIntValue() : 0;

    // Get available bags
    const auto& bags = trainingPackMgr->GetAvailableBags();

    // Get packs in current bag (if any)
    std::vector<TrainingEntry> packsInBag;
    if (!currentBag.empty()) {
        packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
    }

    // Retreat index
    currentIdx--;

    // If before start of current bag (or no current bag), move to previous enabled bag
    if (currentIdx < 0 || currentBag.empty()) {
        // Find current bag position and get previous enabled bag
        std::string prevBag;
        int prevBagLastIdx = -1;

        for (const auto& bag : bags) {
            if (!bag.enabled) continue;
            int bagPackCount = trainingPackMgr->GetBagPackCount(bag.name);
            if (bagPackCount == 0) continue;

            if (bag.name == currentBag) {
                // Found current bag - use the previous valid bag we found
                if (!prevBag.empty()) {
                    currentBag = prevBag;
                    currentIdx = prevBagLastIdx;
                    packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
                    break;
                }
            }

            // Track this as a potential previous bag
            prevBag = bag.name;
            prevBagLastIdx = bagPackCount - 1;
        }

        // If we didn't find a previous bag (current was first or empty), wrap to last enabled bag
        if (currentIdx < 0 || currentBag.empty()) {
            for (const auto& bag : bags) {
                if (!bag.enabled) continue;
                int bagPackCount = trainingPackMgr->GetBagPackCount(bag.name);
                if (bagPackCount == 0) continue;

                // Keep updating to get the last enabled bag with packs
                prevBag = bag.name;
                prevBagLastIdx = bagPackCount - 1;
            }

            if (!prevBag.empty()) {
                currentBag = prevBag;
                currentIdx = prevBagLastIdx;
                packsInBag = trainingPackMgr->GetPacksInBag(currentBag);
            }
        }
    }

    // Update CVars
    if (currentBagCvar) currentBagCvar.setValue(currentBag);
    if (currentIdxCvar) currentIdxCvar.setValue(currentIdx);

    // Load the pack
    if (!packsInBag.empty() && currentIdx >= 0 && currentIdx < (int)packsInBag.size()) {
        std::string code = packsInBag[currentIdx].code;
        std::string name = packsInBag[currentIdx].name;
        std::string bagName = currentBag;

        gameWrapper->SetTimeout([this, code, name, bagName](GameWrapper* gw) {
            std::string cmd = "load_training " + code;
            cvarManager->executeCommand(cmd);
            LOG("SuiteSpot: Previous bag pack: {} from {} ({})", name, bagName, code);
        }, 0.0f);
    } else {
        LOG("SuiteSpot: No packs available in any enabled bag");
    }
}

std::filesystem::path SuiteSpot::GetTrainingPacksPath() const {
    return GetSuiteTrainingDir() / "training_packs.json";
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

// #detailed comments: UpdateTrainingPackList
// Purpose: Launches an external PowerShell script to download the latest
// training pack data and write a JSON cache to disk. This is intentionally
// performed in a background task to avoid any blocking on the UI/game thread.
//
// Safety and behavior notes:
//  - scrapingInProgress is a guard flag ensuring only one update
//    runs at a time. It is set before launching and cleared when the
//    background process finishes.
//  - The implementation uses system() and relies on the platform's
//    default process creation semantics; this must remain as-is for
//    portability with existing deployments. If this is changed to a
//    more advanced process API, ensure identical detach/exit semantics.
//  - The script path is hard-coded to the repo dev path; callers should
//    ensure that the script is present when invoking this routine.
//
// DO NOT CHANGE: Modifying the background thread logic or the way the
// result is checked could resurface race conditions that previously
// required this exact coordination.
void SuiteSpot::UpdateTrainingPackList() {
    if (trainingPackMgr) {
        trainingPackMgr->UpdateTrainingPackList(GetTrainingPacksPath(), gameWrapper);
    }
}


using namespace std;
using namespace std::chrono_literals;

BAKKESMOD_PLUGIN(SuiteSpot, "SuiteSpot", plugin_version, PLUGINTYPE_FREEPLAY)

shared_ptr<CVarManagerWrapper> _globalCvarManager;

void SuiteSpot::LoadHooks() {
    // ===== MATCH EVENT HOOKS =====
    // Re-queue/transition at match end. We use HookEventPost to ensure the game has finished
    // its internal match-end logic before we attempt to load a new map.
    gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", 
        [this](std::string eventName) { 
            GameEndedEvent(eventName); 
        });

    // Metadata Healing Hook
    gameWrapper->HookEvent("Function TAGame.GameInfo_TrainingEditor_TA.PostBeginPlay", [this](std::string eventName) {
        if (!trainingPackMgr) return;
        if (gameWrapper->IsInCustomTraining()) {
            auto server = gameWrapper->GetGameEventAsServer();
            if (!server) return;
            TrainingEditorWrapper editor(server.memory_address);
            if (!editor) return;
            auto saveData = editor.GetTrainingData().GetTrainingData();
            if (!saveData) return;
            std::string code = saveData.GetCode().ToString();
            int realShots = saveData.GetNumRounds();
            trainingPackMgr->HealPack(code, realShots);
        }
    });}

// #detailed comments: GameEndedEvent
// Purpose: Called by hooked game events when a match ends. The function
// runs the auto-load logic if enabled.
//
// Timing and ordering notes:
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

        // Determine if bag rotation should be used
        int trainingMode = settingsSync->GetTrainingMode();
        bool useBagRotation = (trainingMode == 1) && trainingPackMgr != nullptr;
        TrainingEntry selectedBagPack;

        if (useBagRotation) {
            // Get next pack from bag rotation system (advances sequentially)
            auto [pack, bagName] = AdvanceAndGetNextBagPack();
            selectedBagPack = pack;
            useBagRotation = !selectedBagPack.code.empty();  // Only use if we got a valid pack
        }

        autoLoadFeature->OnMatchEnded(gameWrapper, cvarManager, RLMaps, RLTraining, RLWorkshop,
            useBagRotation, selectedBagPack, *settingsSync, usageTracker.get());

        // Increment usage count for training packs
        if (settingsSync->GetMapType() == 1) {
            std::string codeToLoad;
            if (useBagRotation) {
                codeToLoad = selectedBagPack.code;
            } else {
                codeToLoad = settingsSync->GetQuickPicksSelectedCode();
                if (codeToLoad.empty()) codeToLoad = settingsSync->GetCurrentTrainingCode();
            }

            if (!codeToLoad.empty() && usageTracker) {
                usageTracker->IncrementLoadCount(codeToLoad);
            }
        }
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
    trainingPackUI = std::make_shared<TrainingPackUI>(this);
    loadoutUI = new LoadoutUI(this);

    EnsureDataDirectories();
    LoadWorkshopMaps();
    
    // Initialize LoadoutManager
    loadoutManager = std::make_unique<LoadoutManager>(gameWrapper);
    LOG("SuiteSpot: LoadoutManager initialized");

    // Initialize PackUsageTracker
    usageTracker = std::make_unique<PackUsageTracker>(GetSuiteTrainingDir() / "pack_usage_stats.json");
    LOG("SuiteSpot: PackUsageTracker initialized");

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

    if (settingsSync) {
        settingsSync->RegisterAllCVars(cvarManager);
    }
    
        cvarManager->registerNotifier("healer_test_fetch", [this](std::vector<std::string> args) {
        if (args.size() < 2) {
            LOG("Usage: healer_test_fetch <Code>");
            return;
        }
        if (trainingPackMgr) {
            trainingPackMgr->TestHealerFetch(gameWrapper, args[1]);
        }
    }, "Test the training pack metadata healer", PERMISSION_ALL);

    // Next pack command - advances to next pack in current bag (wraps to next bag)
    cvarManager->registerNotifier("suitespot_next_bag_pack", [this](std::vector<std::string> args) {
        AdvanceToNextBagPack();
    }, "Load next pack in current bag (wraps to next bag)", PERMISSION_ALL);

    // Previous pack command - retreats to previous pack in current bag (wraps to previous bag)
    cvarManager->registerNotifier("suitespot_previous_bag_pack", [this](std::vector<std::string> args) {
        RetreatToPreviousBagPack();
    }, "Load previous pack in current bag (wraps to previous bag)", PERMISSION_ALL);

    LOG("SuiteSpot: Plugin initialization complete");
}

void SuiteSpot::onUnload() {
    if (usageTracker) {
        usageTracker->SaveStats();
    }
    delete settingsUI;
    settingsUI = nullptr;
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

void SuiteSpot::Render() {
    if (!imgui_ctx) return;
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(imgui_ctx));

    // Note: TrainingPackUI is a PluginWindow registered with BakkesMod,
    // so it's rendered automatically by the framework. No need to call it here.
}



std::string SuiteSpot::GetMenuName() {

    return "suitespot_browser";

}



std::string SuiteSpot::GetMenuTitle() {

    return "SuiteSpot Training Browser";

}



void SuiteSpot::SetImGuiContext(uintptr_t ctx) {



    if (ctx) {



        imgui_ctx = ctx;



        ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));



    }



}







bool SuiteSpot::ShouldBlockInput() {
    if (!isBrowserOpen) {
        return false;  // Browser closed → no blocking
    }

    // Selective input blocking - consistent with TrainingPackUI
    ImGuiIO& io = ImGui::GetIO();

    // Block when actively typing in text fields (settings UI)
    if (io.WantTextInput && ImGui::IsAnyItemActive()) {
        return true;
    }

    // Allow normal mouse interaction without blocking game input
    return false;
}



bool SuiteSpot::IsActiveOverlay() {

    return isBrowserOpen;

}



void SuiteSpot::OnOpen() {

    LOG("SuiteSpot: OnOpen called");

    isBrowserOpen = true;

    if (trainingPackUI) {

        trainingPackUI->SetOpen(true);

    }

}



void SuiteSpot::OnClose() {



    LOG("SuiteSpot: OnClose called (Ignoring state change to keep browser open)");



    // isBrowserOpen = false; // Disabled to prevent F2 from closing browser



    // if (trainingPackUI) {



    //    trainingPackUI->SetOpen(false);



    // }



}









