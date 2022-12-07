#include "Pch.h"
#include "Quest_BanditsCollectToll.h"

#include "Level.h"

//=================================================================================================
Quest::LoadResult Quest_BanditsCollectToll::Load(GameReader& f)
{
	Quest_Encounter::Load(f);

	f >> otherLoc;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_BanditsCollectToll::GetConversionData(ConversionData& data)
{
	data.id = "bandits_collect_toll";
	data.Add("start_loc", startLoc->index);
	data.Add("other_loc", otherLoc);
	data.Add("enc", enc);
	data.Add("start_time", startTime);
	const bool in_enc = gameLevel->eventHandler == this;
	data.Add("in_enc", in_enc);
	if(in_enc)
		gameLevel->eventHandler = nullptr;
}
