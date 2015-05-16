// wioska
#include "Pch.h"
#include "Base.h"
#include "Village.h"

//=================================================================================================
void Village::Save(HANDLE file, bool local)
{
	City::Save(file, local);

	File f(file);
	f << v_buildings;
}

//=================================================================================================
void Village::Load(HANDLE file, bool local)
{
	City::Load(file, local);

	File f(file);
	f >> v_buildings;
}
