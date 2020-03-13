#include "Pch.h"
#include "Quest_Crazies.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "World.h"
#include "Level.h"
#include "AIController.h"
#include "Team.h"
#include "GameGui.h"
#include "GameMessages.h"

//=================================================================================================
Quest::LoadResult Quest_Crazies::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> crazies_state;
	f >> days;
	f >> check_stone;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_Crazies::GetConversionData(ConversionData& data)
{
	data.id = "crazies";
	data.Add("target_loc", target_loc);
	data.Add("crazies_state", crazies_state);
	data.Add("days", days);
	data.Add("check_stone", check_stone);
}
