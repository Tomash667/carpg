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
	data.id = "banditsCollectToll";
	data.Add("startLoc", startLoc->index);
	data.Add("otherLoc", otherLoc);
	data.Add("enc", enc);
	data.Add("startTime", startTime);
	const bool inEnc = gameLevel->eventHandler == this;
	data.Add("inEnc", inEnc);
	if(inEnc)
		gameLevel->eventHandler = nullptr;
}
