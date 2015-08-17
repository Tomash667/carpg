// podziemia
#include "Pch.h"
#include "Base.h"
#include "InsideLocation.h"

//=================================================================================================
void InsideLocation::Save(HANDLE file, bool local)
{
	Location::Save(file, local);

	WriteFile(file, &target, sizeof(target), &tmp, NULL);
	WriteFile(file, &special_room, sizeof(special_room), &tmp, NULL);
	WriteFile(file, &from_portal, sizeof(from_portal), &tmp, NULL);
}

//=================================================================================================
void InsideLocation::Load(HANDLE file, bool local)
{
	Location::Load(file, local);

	ReadFile(file, &target, sizeof(target), &tmp, NULL);
	ReadFile(file, &special_room, sizeof(special_room), &tmp, NULL);
	ReadFile(file, &from_portal, sizeof(from_portal), &tmp, NULL);
}
