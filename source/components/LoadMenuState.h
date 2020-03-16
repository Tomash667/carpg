#pragma once

//-----------------------------------------------------------------------------
#include "BaseGameState.h"

//-----------------------------------------------------------------------------
class LoadMenuState : public BaseGameState
{
public:
	bool IsSuccess() const { return ok; }

private:
	void OnEnter() override;
	void OnLeave() override;
	void PreconfigureGame();
	void PreloadLanguage();
	void PreloadData();
	void LoadSystem();
	void AddFilesystem();
	void LoadDatafiles();
	void LoadLanguageFiles();
	void ConfigureGame();
	void LoadData();
	void CreateRenderTargets();
	void PostconfigureGame();
	uint ValidateGameData(bool major);
	uint TestGameData(bool major);

	LoadScreen* load_screen;
	uint load_errors, load_warnings;
	bool ok;
	cstring txPreloadAssets, txCreatingListOfFiles, txConfiguringGame, txLoadingAbilities, txLoadingBuildings, txLoadingClasses, txLoadingDialogs,
		txLoadingItems, txLoadingLocations, txLoadingMusics, txLoadingObjects, txLoadingPerks, txLoadingQuests, txLoadingRequired, txLoadingUnits,
		txLoadingShaders, txLoadingLanguageFiles;
};
