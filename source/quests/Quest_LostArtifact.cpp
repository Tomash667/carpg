#include "Pch.h"
#include "Quest_LostArtifact.h"

//=================================================================================================
Quest::LoadResult Quest_LostArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> item;
	f >> st;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_LostArtifact::GetConversionData(ConversionData& data)
{
	data.id = "lostArtifact";
	data.Add("item", item);
	data.Add("startLoc", startLoc);
	data.Add("targetLoc", targetLoc);
	data.Add("startTime", startTime);
	data.Add("st", st);
	data.Add("atLevel", atLevel);
	data.Add("done", done);
}
