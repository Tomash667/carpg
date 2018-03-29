// jaskinia
#include "Pch.h"
#include "GameCore.h"
#include "CaveLocation.h"
#include "SaveState.h"

//=================================================================================================
void CaveLocation::Save(HANDLE file, bool local)
{
	SingleInsideLocation::Save(file, local);

	if(last_visit != -1)
	{
		uint ile = holes.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		if(ile)
			WriteFile(file, &holes[0], sizeof(Int2)*ile, &tmp, nullptr);
		WriteFile(file, &ext, sizeof(ext), &tmp, nullptr);
	}
}

//=================================================================================================
void CaveLocation::Load(HANDLE file, bool local, LOCATION_TOKEN token)
{
	SingleInsideLocation::Load(file, local, token);

	if(last_visit != -1)
	{
		uint ile;
		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		holes.resize(ile);
		if(ile)
			ReadFile(file, &holes[0], sizeof(Int2)*ile, &tmp, nullptr);
		ReadFile(file, &ext, sizeof(ext), &tmp, nullptr);

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
