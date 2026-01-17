#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <memory>

/*
 * ======================================================================================
 * SETTINGS SYNC: THE CONFIGURATION MANAGER
 * ======================================================================================
 * 
 * WHAT IS THIS?
 * This class manages all the "CVars" (Console Variables). CVars are how BakkesMod stores
 * settings so they are remembered when you restart the game.
 * 
 * WHY IS IT HERE?
 * We need a single, safe place to read and write settings. If we scattered this logic
 * everywhere, we might misspell a setting name or use the wrong default value.
 * 
 * HOW DOES IT WORK?
 * 1. `RegisterAllCVars()`: Called once at startup. It tells BakkesMod: "Hey, I have a setting called 
 *    'suitespot_enabled', please remember it for me."
 * 2. It keeps a local copy of every setting (e.g., `bool enabled`) for fast access.
 * 3. When BakkesMod says "The user changed this setting in the console," this class 
 *    automatically updates its local copy.
 */

class SettingsSync
{
public:
    // Tells BakkesMod about all our settings
    void RegisterAllCVars(const std::shared_ptr<CVarManagerWrapper>& cvarManager);
    
    // Updates the "Bag Size" setting (special case because it changes dynamically)
    void UpdateTrainingBagSize(int bagSize, const std::shared_ptr<CVarManagerWrapper>& cvarManager);

    // Getters: Fast, safe ways to ask "Is this feature on?"
    bool IsEnabled() const { return enabled; }
    int GetMapType() const { return mapType; }
    bool IsAutoQueue() const { return autoQueue; }
    bool IsTrainingShuffleEnabled() const { return trainingShuffleEnabled; }
    int GetTrainingBagSize() const { return trainingBagSize; }

    // Delay getters (How long to wait?)
    int GetDelayQueueSec() const { return delayQueueSec; }
    int GetDelayFreeplaySec() const { return delayFreeplaySec; }
    int GetDelayTrainingSec() const { return delayTrainingSec; }
    int GetDelayWorkshopSec() const { return delayWorkshopSec; }

    // Selection getters (Which map/pack is selected?)
    int GetCurrentIndex() const { return currentIndex; }
    int GetCurrentTrainingIndex() const { return currentTrainingIndex; }
    int GetCurrentWorkshopIndex() const { return currentWorkshopIndex; }

    // Setters: Update the local value (used when loading data)
    void SetCurrentIndex(int value);
    void SetCurrentTrainingIndex(int value);
    void SetCurrentWorkshopIndex(int value);

private:
    // Local copies of settings for fast access
    bool enabled = false;
    int mapType = 0; // 0=Freeplay, 1=Training, 2=Workshop
    bool autoQueue = false;
    bool trainingShuffleEnabled = false;
    int trainingBagSize = 0;

    int delayQueueSec = 0;
    int delayFreeplaySec = 0;
    int delayTrainingSec = 0;
    int delayWorkshopSec = 0;

    int currentIndex = 0;        // Freeplay
    int currentTrainingIndex = 0; // Training
    int currentWorkshopIndex = 0; // Workshop
};
