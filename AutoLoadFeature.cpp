#include "pch.h"
#include "AutoLoadFeature.h"
#include "MapList.h"
#include "SettingsSync.h"

#include <algorithm>
#include <random>

void AutoLoadFeature::OnMatchEnded(std::shared_ptr<GameWrapper> gameWrapper,
                                   std::shared_ptr<CVarManagerWrapper> cvarManager,
                                   const std::vector<MapEntry>& maps,
                                   const std::vector<TrainingEntry>& training,
                                   const std::vector<WorkshopEntry>& workshop,
                                   bool useBagRotation,
                                   const TrainingEntry& selectedBagPack,
                                   SettingsSync& settings)
{
    if (!gameWrapper || !cvarManager) return;
    if (!settings.IsEnabled()) return;

    const int mapType = settings.GetMapType();
    const int delayQueueSec = settings.GetDelayQueueSec();
    const int delayFreeplaySec = settings.GetDelayFreeplaySec();
    const int delayTrainingSec = settings.GetDelayTrainingSec();
    const int delayWorkshopSec = settings.GetDelayWorkshopSec();

    std::string currentFreeplayCode = settings.GetCurrentFreeplayCode();
    std::string currentTrainingCode = settings.GetCurrentTrainingCode();
    std::string currentWorkshopPath = settings.GetCurrentWorkshopPath();

    auto safeExecute = [&](int delaySec, const std::string& cmd) {
        // Enforce a minimum delay of 0.1s to ensure the game state has settled after the match.
        // Even if the user sets 0s, we want to force a context switch out of the event stack.
        float actualDelay = (delaySec <= 0) ? 0.1f : static_cast<float>(delaySec);

        gameWrapper->SetTimeout([cvarManager, cmd](GameWrapper* gw) {
            cvarManager->executeCommand(cmd);
        }, actualDelay);
    };

    int mapLoadDelay = 0;

    if (mapType == 0) { // Freeplay
        if (currentFreeplayCode.empty()) {
            LOG("SuiteSpot: No freeplay map selected; skipping load.");
        } else {
            // Verify the map code exists in the list
            auto it = std::find_if(maps.begin(), maps.end(),
                [&](const MapEntry& e) { return e.code == currentFreeplayCode; });
            if (it != maps.end()) {
                safeExecute(delayFreeplaySec, "load_freeplay " + currentFreeplayCode);
                mapLoadDelay = delayFreeplaySec;
                LOG("SuiteSpot: Loading freeplay map: " + it->name);
            } else {
                LOG("SuiteSpot: Freeplay map code not found: " + currentFreeplayCode);
            }
        }
    } else if (mapType == 1) { // Training
        std::string codeToLoad;
        std::string nameToLoad;

        // Use bag rotation if enabled and a pack was selected
        if (useBagRotation && !selectedBagPack.code.empty()) {
            codeToLoad = selectedBagPack.code;
            nameToLoad = selectedBagPack.name;
        } else if (!currentTrainingCode.empty()) {
            // Fall back to specific training code (only if explicitly selected)
            auto it = std::find_if(training.begin(), training.end(),
                [&](const TrainingEntry& e) { return e.code == currentTrainingCode; });
            if (it != training.end()) {
                codeToLoad = currentTrainingCode;
                nameToLoad = it->name;
            } else {
                LOG("SuiteSpot: Training pack code not found: " + currentTrainingCode);
            }
        }

        if (!codeToLoad.empty()) {
            safeExecute(delayTrainingSec, "load_training " + codeToLoad);
            mapLoadDelay = delayTrainingSec;
            LOG("SuiteSpot: Loading training pack: " + nameToLoad);
        } else {
            LOG("SuiteSpot: No training pack to load.");
        }
    } else if (mapType == 2) { // Workshop
        if (currentWorkshopPath.empty()) {
            LOG("SuiteSpot: No workshop map selected; skipping load.");
        } else {
            // Verify the workshop map exists in the list
            auto it = std::find_if(workshop.begin(), workshop.end(),
                [&](const WorkshopEntry& e) { return e.filePath == currentWorkshopPath; });
            if (it != workshop.end()) {
                safeExecute(delayWorkshopSec, "load_workshop \"" + currentWorkshopPath + "\"");
                mapLoadDelay = delayWorkshopSec;
                LOG("SuiteSpot: Loading workshop map: " + it->name);
            } else {
                LOG("SuiteSpot: Workshop map path not found: " + currentWorkshopPath);
            }
        }
    }

    if (settings.IsAutoQueue()) {
        safeExecute(delayQueueSec, "queue");
        LOG("SuiteSpot: Auto-Queuing scheduled with delay: " + std::to_string(delayQueueSec) + "s.");
    }
}
