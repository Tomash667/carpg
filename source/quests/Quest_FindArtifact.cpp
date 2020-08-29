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
	data.id = "find_artifact";
	data.Add("item", item);
	data.Add("start_loc", startLoc);
	data.Add("target_loc", targetLoc);
	data.Add("start_time", start_time);
	data.Add("st", st);
	data.Add("done", done);
}
