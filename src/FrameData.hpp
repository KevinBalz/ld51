#pragma once
#include <string>

struct FrameData
{
	bool triggeredCheckpoint = false;
	int collectedCount;
	bool showDialog;
	bool tutorialDialogOpen;
};

struct SharedData
{
	std::string targetText = "";
	int textDisplayed = 0;
	float textPassed = 0;
	float textBreakpoint = 0;
	float textTutorial = 0;

	void ShowText(std::string str, bool tutorial = false)
	{
		targetText = str;
		textDisplayed = 0;
		textPassed = 0;
		textBreakpoint = 0;
		textTutorial = tutorial;
	}
};
