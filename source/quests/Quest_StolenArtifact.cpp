#include "Pch.h"
#include "Quest_StolenArtifact.h"

#include "UnitGroup.h"

//=================================================================================================
Quest::LoadResult Quest_StolenArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> item;
	f >> group;
	f >> st;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_StolenArtifact::GetConversionData(ConversionData& data)
{
	data.id = "stolenArtifact";
	data.Add("item", item);
	data.Add("group", group);
	data.Add("startLoc", startLoc);
	data.Add("targetLoc", targetLoc);
	data.Add("startTime", startTime);
	data.Add("st", st);
	data.Add("atLevel", atLevel);
	data.Add("done", done);
}
