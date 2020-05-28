#include "Pch.h"
#include "Quest_FindArtifact.h"

#include "GameFile.h"
#include "SaveState.h"

//=================================================================================================
Quest::LoadResult Quest_FindArtifact::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> item;
	if(LOAD_VERSION >= V_0_8)
		f >> st;
	else if(target_loc != -1)
		st = GetTargetLocation().st;
	else
		st = 10;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_FindArtifact::GetConversionData(ConversionData& data)
{
	data.id = "find_artifact";
	data.Add("item", item);
	data.Add("start_loc", &GetStartLocation());
	data.Add("target_loc", target_loc == -1 ? nullptr : &GetTargetLocation());
	data.Add("start_time", start_time);
	data.Add("st", st);
	data.Add("done", done);
}
