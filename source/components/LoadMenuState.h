#pragma once

//-----------------------------------------------------------------------------
#include "GameState.h"

//-----------------------------------------------------------------------------
class LoadMenuState : public GameState
{
private:
	bool OnEnter() override;
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
	cstring txHaveErrors;
	cstring txPreloadAssets, txCreatingListOfFiles, txConfiguringGame, txLoadingAbilities, txLoadingBuildings, txLoadingClasses, txLoadingDialogs,
		txLoadingItems, txLoadingLocations, txLoadingMusics, txLoadingObjects, txLoadingPerks, txLoadingQuests, txLoadingRequired, txLoadingUnits,
		txLoadingShaders, txLoadingLanguageFiles;
};
