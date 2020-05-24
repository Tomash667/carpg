#include "Pch.h"
#include "Camp.h"

#include "GameFile.h"

//=================================================================================================
void Camp::Save(GameWriter& f)
{
	OutsideLocation::Save(f);

	f << create_time;
}

//=================================================================================================
void Camp::Load(GameReader& f)
{
	OutsideLocation::Load(f);

	f >> create_time;
}
