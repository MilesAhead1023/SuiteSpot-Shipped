#pragma once
#include <memory>
#include <vector>

class CVarManagerWrapper;
class GameWrapper;
class SettingsSync;

struct MapEntry;
struct TrainingEntry;
struct WorkshopEntry;

// AutoLoadFeature: Match-end auto-load/auto-queue logic.
// Depends on SettingsSync state and map lists; no ImGui usage.
class AutoLoadFeature {
public:
    void OnMatchEnded(const std::shared_ptr<GameWrapper>& gameWrapper,
                      const std::shared_ptr<CVarManagerWrapper>& cvarManager,
                      const std::vector<MapEntry>& maps,
                      const std::vector<TrainingEntry>& training,
                      const std::vector<WorkshopEntry>& workshop,
                      const std::vector<TrainingEntry>& shuffleBag,
                      SettingsSync& settings);
};
