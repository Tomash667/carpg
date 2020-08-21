#include "Pch.h"
#include "Quest_BanditsCollectToll.h"

#include "GameFile.h"
#include "Level.h"

//=================================================================================================
Quest::LoadResult Quest_BanditsCollectToll::Load(GameReader& f)
{
	Quest_Encounter::Load(f);

	f >> other_loc;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_BanditsCollectToll::GetConversionData(ConversionData& data)
{
	data.id = "bandits_collect_toll";
	data.Add("start_loc", startLoc->index);
	data.Add("other_loc", other_loc);
	data.Add("enc", enc);
	data.Add("start_time", start_time);
	const bool in_enc = game_level->event_handler == this;
	data.Add("in_enc", in_enc);
	if(in_enc)
		game_level->event_handler = nullptr;
}
