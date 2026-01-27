#include "pch.h"
#include "SettingsSync.h"

#include <algorithm>

void SettingsSync::RegisterAllCVars(const std::shared_ptr<CVarManagerWrapper>& cvarManager)
{
    if (!cvarManager) return;

    cvarManager->registerCvar("suitespot_enabled", "0", "Enable SuiteSpot", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            enabled = cvar.getBoolValue();
        });

    cvarManager->registerCvar("suitespot_map_type", "0", "Map type: 0=Freeplay, 1=Training, 2=Workshop", true, true, 0, true, 2)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            mapType = cvar.getIntValue();
        });

    cvarManager->registerCvar("suitespot_auto_queue", "0", "Enable auto-queuing after map load", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            autoQueue = cvar.getBoolValue();
        });

    cvarManager->registerCvar("suitespot_training_mode", "0", "Training mode: 0=Single Pack, 1=Bag Rotation", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            trainingMode = cvar.getIntValue();
        });

    cvarManager->registerCvar("suitespot_quickpicks_list_type", "0", "List type: 0=Flicks Picks, 1=Your Favorites", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            quickPicksListType = cvar.getIntValue();
        });

    cvarManager->registerCvar("suitespot_quickpicks_count", "10", "Number of quick picks to show", true, true, 5, true, 15)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            quickPicksCount = cvar.getIntValue();
        });

    cvarManager->registerCvar("suitespot_quickpicks_selected", "", "Selected quick pick pack code", true)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            quickPicksSelected = cvar.getStringValue();
        });

    cvarManager->registerCvar("suitespot_delay_queue_sec", "0", "Delay before queuing (seconds)", true, true, 0, true, 300)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            delayQueueSec = std::max(0, cvar.getIntValue());
        });

    cvarManager->registerCvar("suitespot_delay_freeplay_sec", "0", "Delay before loading freeplay map (seconds)", true, true, 0, true, 300)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            delayFreeplaySec = std::max(0, cvar.getIntValue());
        });

    cvarManager->registerCvar("suitespot_delay_training_sec", "0", "Delay before loading training map (seconds)", true, true, 0, true, 300)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            delayTrainingSec = std::max(0, cvar.getIntValue());
        });

    cvarManager->registerCvar("suitespot_delay_workshop_sec", "0", "Delay before loading workshop map (seconds)", true, true, 0, true, 300)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            delayWorkshopSec = std::max(0, cvar.getIntValue());
        });

    cvarManager->registerCvar("suitespot_current_freeplay_code", "", "Currently selected freeplay map code", true)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentFreeplayCode = cvar.getStringValue();
        });

    cvarManager->registerCvar("suitespot_current_training_code", "", "Currently selected training pack code", true)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentTrainingCode = cvar.getStringValue();
        });

    cvarManager->registerCvar("suitespot_current_workshop_path", "", "Currently selected workshop map path", true)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentWorkshopPath = cvar.getStringValue();
        });

    cvarManager->registerCvar("ss_training_maps", "", "Stored training maps", true, false, 0, false, 0);

    cvarManager->getCvar("suitespot_enabled").setValue(enabled ? 1 : 0);
    cvarManager->getCvar("suitespot_map_type").setValue(mapType);
    cvarManager->getCvar("suitespot_auto_queue").setValue(autoQueue ? 1 : 0);
    cvarManager->getCvar("suitespot_training_mode").setValue(trainingMode);
    cvarManager->getCvar("suitespot_quickpicks_count").setValue(quickPicksCount);
    cvarManager->getCvar("suitespot_quickpicks_selected").setValue(quickPicksSelected);
    cvarManager->getCvar("suitespot_delay_queue_sec").setValue(delayQueueSec);
    cvarManager->getCvar("suitespot_delay_freeplay_sec").setValue(delayFreeplaySec);
    cvarManager->getCvar("suitespot_delay_training_sec").setValue(delayTrainingSec);
    cvarManager->getCvar("suitespot_delay_workshop_sec").setValue(delayWorkshopSec);
    cvarManager->getCvar("suitespot_current_freeplay_code").setValue(currentFreeplayCode);
    cvarManager->getCvar("suitespot_current_training_code").setValue(currentTrainingCode);
    cvarManager->getCvar("suitespot_current_workshop_path").setValue(currentWorkshopPath);
}

void SettingsSync::SetCurrentFreeplayCode(const std::string& code)
{
    currentFreeplayCode = code;
}

void SettingsSync::SetCurrentTrainingCode(const std::string& code)
{
    currentTrainingCode = code;
}

void SettingsSync::SetQuickPicksSelected(const std::string& code)
{
    quickPicksSelected = code;
}

void SettingsSync::SetCurrentWorkshopPath(const std::string& path)
{
    currentWorkshopPath = path;
}

void SettingsSync::SetTrainingMode(int mode)
{
    trainingMode = mode;
}
