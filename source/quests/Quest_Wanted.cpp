#include "Pch.h"
#include "Quest_Wanted.h"

//=================================================================================================
Quest::LoadResult Quest_Wanted::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> level;
	f >> crazy;
	if(LOAD_VERSION >= V_0_12)
		clas = Class::TryGet(f.ReadString1());
	else
	{
		int oldClass;
		f >> oldClass;
		clas = old::ConvertOldClass(oldClass);
	}
	f >> unitName;
	f >> targetUnit;
	f >> inLocation;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_Wanted::GetConversionData(ConversionData& data)
{
	data.id = "wanted";
	data.Add("startLoc", startLoc);
	data.Add("targetLoc", targetLoc);
	data.Add("startTime", startTime);
	data.Add("level", level);
	data.Add("crazy", crazy);
	data.Add("unitName", unitName);
	data.Add("clas", clas);
}
