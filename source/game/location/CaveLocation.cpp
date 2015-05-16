// jaskinia
#include "Pch.h"
#include "Base.h"
#include "CaveLocation.h"

//=================================================================================================
void CaveLocation::Save(HANDLE file, bool local)
{
	SingleInsideLocation::Save(file, local);

	if(last_visit != -1)
	{
		uint ile = holes.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		if(ile)
			WriteFile(file, &holes[0], sizeof(INT2)*ile, &tmp, NULL);
		WriteFile(file, &ext, sizeof(ext), &tmp, NULL);
	}
}

//=================================================================================================
void CaveLocation::Load(HANDLE file, bool local)
{
	SingleInsideLocation::Load(file, local);

	if(last_visit != -1)
	{
		uint ile;
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		holes.resize(ile);
		if(ile)
			ReadFile(file, &holes[0], sizeof(INT2)*ile, &tmp, NULL);
		ReadFile(file, &ext, sizeof(ext), &tmp, NULL);
	}
}
