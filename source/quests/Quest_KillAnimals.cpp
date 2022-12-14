#include "Pch.h"
#include "Quest_KillAnimals.h"

#include "Level.h"

//=================================================================================================
Quest::LoadResult Quest_KillAnimals::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_17)
		Quest2::LoadQuest2(f, "kill_animals");
	else
	{
		Quest::Load(f);
		f >> targetLoc;
		f.Skip<bool>(); // done
		f.Skip<int>(); // atLevel
		f >> st;
	}

	return LoadResult::Convert;
}

//=================================================================================================
void Quest_KillAnimals::LoadDetails(GameReader& f)
{
	f >> targetLoc;
	f >> st;
}

//=================================================================================================
void Quest_KillAnimals::GetConversionData(ConversionData& data)
{
	data.id = "kill_animals";
	data.Add("startLoc", startLoc);
	data.Add("targetLoc", targetLoc);
	data.Add("startTime", startTime);
	data.Add("st", st);

	if(gameLevel->eventHandler == this)
		gameLevel->eventHandler = nullptr;

	Cleanup();
}
