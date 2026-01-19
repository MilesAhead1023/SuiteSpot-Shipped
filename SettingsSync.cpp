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

    cvarManager->registerCvar("suitespot_bag_rotation", "1", "Enable categorized bag rotation for training", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            bagRotationEnabled = cvar.getBoolValue();
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

    // Bag navigation CVars (for Next Pack feature)
    cvarManager->registerCvar("suitespot_current_bag", "", "Current bag for pack navigation", true, false, 0, false, 0)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentBag = cvar.getStringValue();
        });

    cvarManager->registerCvar("suitespot_current_bag_pack_index", "0", "Current pack index within bag", true, true, 0, true, 1000)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
            currentBagPackIndex = std::max(0, cvar.getIntValue());
        });

    cvarManager->getCvar("suitespot_enabled").setValue(enabled ? 1 : 0);
    cvarManager->getCvar("suitespot_map_type").setValue(mapType);
    cvarManager->getCvar("suitespot_auto_queue").setValue(autoQueue ? 1 : 0);
    cvarManager->getCvar("suitespot_bag_rotation").setValue(bagRotationEnabled ? 1 : 0);
    cvarManager->getCvar("suitespot_delay_queue_sec").setValue(delayQueueSec);
    cvarManager->getCvar("suitespot_delay_freeplay_sec").setValue(delayFreeplaySec);
    cvarManager->getCvar("suitespot_delay_training_sec").setValue(delayTrainingSec);
    cvarManager->getCvar("suitespot_delay_workshop_sec").setValue(delayWorkshopSec);
    cvarManager->getCvar("suitespot_current_freeplay_index").setValue(currentIndex);
    cvarManager->getCvar("suitespot_current_training_index").setValue(currentTrainingIndex);
    cvarManager->getCvar("suitespot_current_workshop_index").setValue(currentWorkshopIndex);
    cvarManager->getCvar("suitespot_current_bag").setValue(currentBag);
    cvarManager->getCvar("suitespot_current_bag_pack_index").setValue(currentBagPackIndex);
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

void SettingsSync::SetCurrentBag(const std::string& bagName)
{
    currentBag = bagName;
}

void SettingsSync::SetCurrentBagPackIndex(int value)
{
    currentBagPackIndex = std::max(0, value);
}
