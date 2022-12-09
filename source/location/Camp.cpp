#include "Pch.h"
#include "Camp.h"

//=================================================================================================
void Camp::Save(GameWriter& f)
{
	OutsideLocation::Save(f);

	f << createTime;
}

//=================================================================================================
void Camp::Load(GameReader& f)
{
	OutsideLocation::Load(f);

	f >> createTime;
}
