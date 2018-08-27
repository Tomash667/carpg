#include "Pch.h"
#include "GameCore.h"
#include "Camp.h"
#include "GameFile.h"

//=================================================================================================
void Camp::Save(GameWriter& f, bool local)
{
	OutsideLocation::Save(f, local);

	f << create_time;
}

//=================================================================================================
void Camp::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	OutsideLocation::Load(f, local, token);

	f >> create_time;
}
