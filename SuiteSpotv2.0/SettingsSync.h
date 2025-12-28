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

    float GetPostMatchDurationSec() const { return postMatchDurationSec; }
    float GetOverlayWidth() const { return overlayWidth; }
    float GetOverlayHeight() const { return overlayHeight; }
    float GetOverlayAlpha() const { return overlayAlpha; }
    float GetBlueTeamHue() const { return blueTeamHue; }
    float GetOrangeTeamHue() const { return orangeTeamHue; }
    float GetOverlayOffsetX() const { return overlayOffsetX; }
    float GetOverlayOffsetY() const { return overlayOffsetY; }
    int GetOverlayMode() const { return overlayMode; }

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

    float postMatchDurationSec = 15.0f;
    float overlayWidth = 880.0f;
    float overlayHeight = 400.0f;
    float overlayAlpha = 0.85f;
    float blueTeamHue = 240.0f;
    float orangeTeamHue = 25.0f;
    float overlayOffsetX = 0.0f;
    float overlayOffsetY = 0.0f;
    int overlayMode = 0; // 0=LastMatch, 1=SessionStats
};
