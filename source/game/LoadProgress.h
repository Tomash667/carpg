#pragma once

//-----------------------------------------------------------------------------
enum LoadProgress
{
	Task_None = -1,

	// first stage
	Task_AddFilesystem = 0,
	Task_LoadItemsDatafile,
	Task_LoadMusicDatafile,
	Task_LoadLanguageFiles,
	Task_LoadShaders,
	Task_ConfigureGame,

	// second stage
	Task_LoadGuiTextures,
	Task_LoadTerrainTextures,
	Task_LoadParticles,
	Task_LoadPhysicMeshes,
	Task_LoadModels,
	Task_LoadBuildings,
	Task_LoadTraps,
	Task_LoadSpells,
	Task_LoadObjects,
	Task_LoadUnits,
	Task_LoadItems,
	Task_LoadSounds,
	Task_LoadMusic
};
