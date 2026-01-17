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
                                   const std::vector<TrainingEntry>& shuffleBag,
                                   SettingsSync& settings)
{
    if (!gameWrapper || !cvarManager) return;
    if (!settings.IsEnabled()) return;

    const int mapType = settings.GetMapType();
    const int delayQueueSec = settings.GetDelayQueueSec();
    const int delayFreeplaySec = settings.GetDelayFreeplaySec();
    const int delayTrainingSec = settings.GetDelayTrainingSec();
    const int delayWorkshopSec = settings.GetDelayWorkshopSec();
    const bool trainingShuffleEnabled = settings.IsTrainingShuffleEnabled();

    int currentIndex = settings.GetCurrentIndex();
    int currentTrainingIndex = settings.GetCurrentTrainingIndex();
    int currentWorkshopIndex = settings.GetCurrentWorkshopIndex();

    auto safeExecute = [&](int delaySec, const std::string& cmd) {
        if (delaySec <= 0) {
            cvarManager->executeCommand(cmd);
        } else {
            gameWrapper->SetTimeout([cvarManager, cmd](GameWrapper* gw) {
                cvarManager->executeCommand(cmd);
            }, static_cast<float>(delaySec));
        }
    };

    int mapLoadDelay = 0;

    if (mapType == 0) { // Freeplay
        if (currentIndex < 0 || currentIndex >= static_cast<int>(maps.size())) {
            LOG("SuiteSpot: Freeplay index out of range; skipping load.");
        } else {
            safeExecute(delayFreeplaySec, "load_freeplay " + maps[currentIndex].code);
            mapLoadDelay = delayFreeplaySec;
            LOG("SuiteSpot: Loading freeplay map: " + maps[currentIndex].name);
        }
    } else if (mapType == 1) { // Training
        if (training.empty() && shuffleBag.empty()) {
            LOG("SuiteSpot: No training maps configured.");
        } else {
            std::string codeToLoad;
            std::string nameToLoad;

            if (trainingShuffleEnabled && !shuffleBag.empty()) {
                static std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, static_cast<int>(shuffleBag.size()) - 1);
                int bagIndex = dist(rng);
                codeToLoad = shuffleBag[bagIndex].code;
                nameToLoad = shuffleBag[bagIndex].name;
            } else if (!training.empty()) {
                currentTrainingIndex = std::clamp(currentTrainingIndex, 0, static_cast<int>(training.size() - 1));
                settings.SetCurrentTrainingIndex(currentTrainingIndex);
                codeToLoad = training[currentTrainingIndex].code;
                nameToLoad = training[currentTrainingIndex].name;
            }

            if (!codeToLoad.empty()) {
                safeExecute(delayTrainingSec, "load_training " + codeToLoad);
                mapLoadDelay = delayTrainingSec;
                LOG("SuiteSpot: Loading training map: " + nameToLoad);
            }
        }
    } else if (mapType == 2) { // Workshop
        if (workshop.empty()) {
            LOG("SuiteSpot: No workshop maps configured.");
        } else {
            currentWorkshopIndex = std::clamp(currentWorkshopIndex, 0, static_cast<int>(workshop.size() - 1));
            settings.SetCurrentWorkshopIndex(currentWorkshopIndex);
            safeExecute(delayWorkshopSec, "load_workshop \"" + workshop[currentWorkshopIndex].filePath + "\"");
            mapLoadDelay = delayWorkshopSec;
            LOG("SuiteSpot: Loading workshop map: " + workshop[currentWorkshopIndex].name);
        }
    }

    if (settings.IsAutoQueue()) {
        safeExecute(delayQueueSec, "queue");
        LOG("SuiteSpot: Auto-Queuing scheduled with delay: " + std::to_string(delayQueueSec) + "s.");
    }
}
