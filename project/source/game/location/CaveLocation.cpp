#include "Pch.h"
#include "GameCore.h"
#include "CaveLocation.h"
#include "SaveState.h"
#include "GameFile.h"

//=================================================================================================
void CaveLocation::Save(GameWriter& f, bool local)
{
	SingleInsideLocation::Save(f, local);

	if(last_visit != -1)
	{
		f << holes;
		f << ext;
	}
}

//=================================================================================================
void CaveLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	SingleInsideLocation::Load(f, local, token);

	if(last_visit != -1)
	{
		f >> holes;
		f >> ext;

		// fix for broken corridor in mine & portal
		// propably fixed at unknown point of time, here just repair it for old saves
		if(LOAD_VERSION < V_0_7 && rooms.size() == 2u && rooms[1].pos != Int2(0, 0))
		{
			Room& room = rooms[1];
			room.pos = Int2(0, 0);
			room.size = Int2(0, 0);
		}
	}
}
