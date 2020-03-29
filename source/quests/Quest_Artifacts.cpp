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
	data.id = "global_init";
	data.Add("artifacts_loc", target_loc);
	data.Add("done", done);
}
