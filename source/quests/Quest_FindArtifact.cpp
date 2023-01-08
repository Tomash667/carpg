#include "Pch.h"
#include "Quest_FindArtifact.h"

//=================================================================================================
Quest::LoadResult Quest_FindArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> item;
	f >> st;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_FindArtifact::GetConversionData(ConversionData& data)
{
	data.id = "findArtifact";
	data.Add("item", item);
	data.Add("startLoc", startLoc);
	data.Add("targetLoc", targetLoc);
	data.Add("startTime", startTime);
	data.Add("st", st);
	data.Add("done", done);
}
