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
		int old_clas;
		f >> old_clas;
		clas = old::ConvertOldClass(old_clas);
	}
	f >> unit_name;
	f >> target_unit;
	f >> in_location;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_Wanted::GetConversionData(ConversionData& data)
{
	data.id = "wanted";
	data.Add("startLoc", startLoc);
	data.Add("level", level);
	data.Add("crazy", crazy);
	data.Add("clas", clas);
	data.Add("name", unit_name);
	data.Add("startTime", start_time);
}
