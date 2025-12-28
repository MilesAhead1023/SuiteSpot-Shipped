#pragma once
#include "IMGUI/imgui.h"
#include "MapList.h"
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
	void RenderOverlayTab();

	// Helper function to safely set CVar values with null checks
	template<typename T>
	void SetCVarSafely(const std::string& cvarName, const T& value);

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
