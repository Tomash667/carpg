// wioska
#include "Pch.h"
#include "Base.h"
#include "Village.h"
#include "SaveState.h"
#include "ResourceManager.h"

//=================================================================================================
void Village::Save(HANDLE file, bool local)
{
	City::Save(file, local);

	FileWriter f(file);
	f << v_buildings;
}

//=================================================================================================
void Village::Load(HANDLE file, bool local)
{
	City::Load(file, local);

	FileReader f(file);
	f >> v_buildings;

	if(LOAD_VERSION <= V_0_3 && v_buildings[1] == B_COTTAGE)
		v_buildings[1] = B_NONE;

	// fix wrong village house building
	if(last_visit != -1 && LOAD_VERSION < V_0_4)
	{
		bool need_fix = false;

		if(LOAD_VERSION < V_0_3)
			need_fix = true;
		else if(LOAD_VERSION == V_0_3)
		{
			InsideBuilding* b = FindInsideBuilding(B_VILLAGE_HALL);
			// easiest way to find out if it uses old mesh
			if(b->top > 3.5f)
				need_fix = true;
		}

		if(need_fix)
		{
			FindBuilding(B_VILLAGE_HALL)->type = B_VILLAGE_HALL_OLD;
			for(Object& o : objects)
			{
				if(o.mesh->res->filename == "soltys.qmsh")
					o.mesh = ResourceManager::Get().GetLoadedMesh("soltys_old.qmsh")->data;
			}
			InsideBuilding* b = FindInsideBuilding(B_VILLAGE_HALL);
			b->type = B_VILLAGE_HALL_OLD;
			for(Object& o : b->objects)
			{
				if(o.mesh->res->filename == "soltys_srodek.qmsh")
					o.mesh = ResourceManager::Get().GetLoadedMesh("soltys_srodek_old.qmsh")->data;
			}
		}
	}
}
