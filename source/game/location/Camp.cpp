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
void Camp::Load(HANDLE file, bool local, LOCATION_TOKEN token)
{
	OutsideLocation::Load(file, local, token);

	ReadFile(file, &create_time, sizeof(create_time), &tmp, nullptr);
}
