// podziemia
#include "Pch.h"
#include "Base.h"
#include "InsideLocation.h"

//=================================================================================================
void InsideLocation::Save(HANDLE file, bool local)
{
	Location::Save(file, local);

	WriteFile(file, &cel, sizeof(cel), &tmp, NULL);
	WriteFile(file, &specjalny_pokoj, sizeof(specjalny_pokoj), &tmp, NULL);
	WriteFile(file, &from_portal, sizeof(from_portal), &tmp, NULL);
}

//=================================================================================================
void InsideLocation::Load(HANDLE file, bool local)
{
	Location::Load(file, local);

	ReadFile(file, &cel, sizeof(cel), &tmp, NULL);
	ReadFile(file, &specjalny_pokoj, sizeof(specjalny_pokoj), &tmp, NULL);
	ReadFile(file, &from_portal, sizeof(from_portal), &tmp, NULL);
}
