#include "Pch.h"
#include "Quest_CampNearCity.h"

#include "GameFile.h"
#include "Level.h"
#include "SaveState.h"

//=================================================================================================
Quest::LoadResult Quest_CampNearCity::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> group;
	if(LOAD_VERSION >= V_0_8)
		f >> st;
	else if(targetLoc)
		st = targetLoc->st;
	else
		st = 10;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_CampNearCity::GetConversionData(ConversionData& data)
{
	data.id = "camp_near_city";
	data.Add("start_loc", startLoc);
	data.Add("target_loc", targetLoc);
	data.Add("start_time", start_time);
	data.Add("group", group);
	data.Add("st", st);

	if(game_level->event_handler == this)
		game_level->event_handler = nullptr;
}
