#include "Pch.h"
#include "Quest_DeliverLetter.h"

//=================================================================================================
Quest::LoadResult Quest_DeliverLetter::Load(GameReader& f)
{
	Quest::Load(f);

	if(prog < Progress::Finished)
		f >> end_loc;
	else
		end_loc = 0;

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_DeliverLetter::GetConversionData(ConversionData& data)
{
	data.id = "deliver_letter";
	data.Add("start_loc", startLoc->index);
	data.Add("end_loc", end_loc);
	data.Add("start_time", start_time);
}
