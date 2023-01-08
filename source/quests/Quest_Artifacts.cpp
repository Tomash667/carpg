#include "Pch.h"
#include "Quest_Artifacts.h"

//=================================================================================================
Quest::LoadResult Quest_Artifacts::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_Artifacts::GetConversionData(ConversionData& data)
{
	data.id = "globalInit";
	data.Add("artifactsLoc", targetLoc ? targetLoc->index : -1);
	data.Add("done", done);
}
