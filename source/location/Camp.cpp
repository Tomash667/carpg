#include "Pch.h"
#include "Camp.h"
#include "GameFile.h"

//=================================================================================================
void Camp::Save(GameWriter& f, bool local)
{
	OutsideLocation::Save(f, local);

	f << create_time;
}

//=================================================================================================
void Camp::Load(GameReader& f, bool local)
{
	OutsideLocation::Load(f, local);

	f >> create_time;
}
