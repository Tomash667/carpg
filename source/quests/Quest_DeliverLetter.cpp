#include "Pch.h"
#include "Quest_DeliverLetter.h"

//=================================================================================================
Quest::LoadResult Quest_DeliverLetter::Load(GameReader& f)
{
	Quest::Load(f);

	if(prog < Progress::Finished)
		f >> endLoc;
	else
		endLoc = 0;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_DeliverLetter::GetConversionData(ConversionData& data)
{
	data.id = "deliver_letter";
	data.Add("startLoc", startLoc->index);
	data.Add("endLoc", endLoc);
	data.Add("startTime", startTime);
}
