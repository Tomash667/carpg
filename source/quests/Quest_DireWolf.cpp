#include "Pch.h"
#include "Quest_DireWolf.h"

//=================================================================================================
Quest::LoadResult Quest_DireWolf::Load(GameReader& f)
{
	Quest2::LoadQuest2(f, "direWolf");
	return LoadResult::Convert;
}

//=================================================================================================
void Quest_DireWolf::LoadDetails(GameReader& f)
{
	f >> forest;
}

//=================================================================================================
void Quest_DireWolf::GetConversionData(ConversionData& data)
{
	data.id = "direWolf";
	data.Add("startLoc", startLoc);
	data.Add("forest", forest);

	Cleanup();
}
