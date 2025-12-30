#pragma once
#include <memory>

class CVarManagerWrapper;

// SettingsSync: Registers CVars and stores settings state for UI/features.
// Only module allowed to register CVars directly.
class SettingsSync {
public:
    void RegisterAllCVars(const std::shared_ptr<CVarManagerWrapper>& cvarManager);
    void UpdateTrainingBagSize(int bagSize, const std::shared_ptr<CVarManagerWrapper>& cvarManager);

    bool IsEnabled() const { return enabled; }
    int GetMapType() const { return mapType; }
    bool IsAutoQueue() const { return autoQueue; }
    bool IsTrainingShuffleEnabled() const { return trainingShuffleEnabled; }
    int GetTrainingBagSize() const { return trainingBagSize; }

    int GetDelayQueueSec() const { return delayQueueSec; }
    int GetDelayFreeplaySec() const { return delayFreeplaySec; }
    int GetDelayTrainingSec() const { return delayTrainingSec; }
    int GetDelayWorkshopSec() const { return delayWorkshopSec; }

    int GetCurrentIndex() const { return currentIndex; }
    int GetCurrentTrainingIndex() const { return currentTrainingIndex; }
    int GetCurrentWorkshopIndex() const { return currentWorkshopIndex; }

    void SetCurrentIndex(int value);
    void SetCurrentTrainingIndex(int value);
    void SetCurrentWorkshopIndex(int value);

private:
    bool enabled = false;
    bool autoQueue = false;
    int  mapType = 0;

    int  delayQueueSec = 0;
    int  delayFreeplaySec = 0;
    int  delayTrainingSec = 0;
    int  delayWorkshopSec = 0;

    int  currentIndex = 0;
    int  currentTrainingIndex = 0;
    int  currentWorkshopIndex = 0;

    bool trainingShuffleEnabled = false;
    int  trainingBagSize = 1;
};
