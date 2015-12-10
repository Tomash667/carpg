#include "Pch.h"
#include "Base.h"
#include "Camp.h"

//=================================================================================================
void Camp::Save(HANDLE file, bool local)
{
	OutsideLocation::Save(file, local);

	WriteFile(file, &create_time, sizeof(create_time), &tmp, nullptr);
}

//=================================================================================================
void Camp::Load(HANDLE file, bool local)
{
	OutsideLocation::Load(file, local);

	ReadFile(file, &create_time, sizeof(create_time), &tmp, nullptr);
}
