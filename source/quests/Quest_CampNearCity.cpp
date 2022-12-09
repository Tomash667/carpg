#include "Pch.h"
#include "Quest_CampNearCity.h"

#include "Level.h"
#include "UnitGroup.h"

//=================================================================================================
Quest::LoadResult Quest_CampNearCity::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> group;
	f >> st;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_CampNearCity::GetConversionData(ConversionData& data)
{
	data.id = "camp_near_city";
	data.Add("startLoc", startLoc);
	data.Add("targetLoc", targetLoc);
	data.Add("startTime", startTime);
	data.Add("group", group);
	data.Add("st", st);

	if(gameLevel->eventHandler == this)
		gameLevel->eventHandler = nullptr;
}
