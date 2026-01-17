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

    cvarManager->registerCvar("suitespot_training_shuffle", "0", "Enable shuffle for training maps", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            trainingShuffleEnabled = cvar.getBoolValue();
        });

    cvarManager->registerCvar("suitespot_training_bag_size", "0", "Shuffle bag size (legacy, reflects selected count)", true, true, 0, true, 1000)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            trainingBagSize = cvar.getIntValue();
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

    cvarManager->registerCvar("suitespot_current_freeplay_index", "0", "Currently selected freeplay map index", true, true, 0, true, 1000)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentIndex = std::max(0, cvar.getIntValue());
        });

    cvarManager->registerCvar("suitespot_current_training_index", "0", "Currently selected training map index", true, true, 0, true, 1000)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentTrainingIndex = std::max(0, cvar.getIntValue());
        });

    cvarManager->registerCvar("suitespot_current_workshop_index", "0", "Currently selected workshop map index", true, true, 0, true, 1000)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentWorkshopIndex = std::max(0, cvar.getIntValue());
        });

    cvarManager->registerCvar("ss_training_maps", "", "Stored training maps", true, false, 0, false, 0);

    cvarManager->getCvar("suitespot_enabled").setValue(enabled ? 1 : 0);
    cvarManager->getCvar("suitespot_map_type").setValue(mapType);
    cvarManager->getCvar("suitespot_auto_queue").setValue(autoQueue ? 1 : 0);
    cvarManager->getCvar("suitespot_training_shuffle").setValue(trainingShuffleEnabled ? 1 : 0);
    cvarManager->getCvar("suitespot_training_bag_size").setValue(trainingBagSize);
    cvarManager->getCvar("suitespot_delay_queue_sec").setValue(delayQueueSec);
    cvarManager->getCvar("suitespot_delay_freeplay_sec").setValue(delayFreeplaySec);
    cvarManager->getCvar("suitespot_delay_training_sec").setValue(delayTrainingSec);
    cvarManager->getCvar("suitespot_delay_workshop_sec").setValue(delayWorkshopSec);
    cvarManager->getCvar("suitespot_current_freeplay_index").setValue(currentIndex);
    cvarManager->getCvar("suitespot_current_training_index").setValue(currentTrainingIndex);
    cvarManager->getCvar("suitespot_current_workshop_index").setValue(currentWorkshopIndex);
}

void SettingsSync::UpdateTrainingBagSize(int bagSize, const std::shared_ptr<CVarManagerWrapper>& cvarManager)
{
    trainingBagSize = std::max(0, bagSize);
    if (cvarManager)
    {
        cvarManager->getCvar("suitespot_training_bag_size").setValue(trainingBagSize);
    }
}

void SettingsSync::SetCurrentIndex(int value)
{
    currentIndex = std::max(0, value);
}

void SettingsSync::SetCurrentTrainingIndex(int value)
{
    currentTrainingIndex = std::max(0, value);
}

void SettingsSync::SetCurrentWorkshopIndex(int value)
{
    currentWorkshopIndex = std::max(0, value);
}
