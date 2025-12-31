#pragma once
#include "IMGUI/imgui.h"
#include "MapList.h"
#include "UIHelpers.h"
#include <string>
#include <vector>

class SuiteSpot;

class SettingsUI {
public:
	explicit SettingsUI(SuiteSpot* plugin);
	void RenderMainSettingsWindow();

private:
	void RenderGeneralTab(bool& enabledValue, int& mapTypeValue);
	void RenderAutoQueueTab(bool& autoQueueValue, int& delayQueueSecValue);
	void RenderMapSelectionTab(int mapTypeValue,
		bool trainingShuffleEnabledValue,
		int& currentIndexValue,
		int& currentTrainingIndexValue,
		int& currentWorkshopIndexValue,
		int& delayFreeplaySecValue,
		int& delayTrainingSecValue,
		int& delayWorkshopSecValue);

	// Mode-specific rendering (extracted from RenderMapSelectionTab)
	void RenderFreeplayMode(int& currentIndexValue, int& delayFreeplaySecValue);
	void RenderTrainingMode(bool trainingShuffleEnabledValue, int& currentTrainingIndexValue, int& delayTrainingSecValue);
	void RenderWorkshopMode(int& currentWorkshopIndexValue, int& delayWorkshopSecValue);

	SuiteSpot* plugin_;

	// Main settings UI state
	bool showAddTrainingForm = false;
	std::string trainingLabelBuf;
	char newMapCode[64] = {0};
	char newMapName[64] = {0};
	bool addSuccess = false;
	float addSuccessTimer = 0.0f;

	bool workshopPathInit = false;
	std::string workshopPathCache;
	char workshopPathBuf[512] = {0};

};
