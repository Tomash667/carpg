// podziemia z jednym poziomem
#include "Pch.h"
#include "Base.h"
#include "SingleInsideLocation.h"

//=================================================================================================
void SingleInsideLocation::ApplyContext(LevelContext& ctx)
{
	ctx.units = &units;
	ctx.objects = &objects;
	ctx.chests = &chests;
	ctx.traps = &traps;
	ctx.doors = &doors;
	ctx.items = &items;
	ctx.useables = &useables;
	ctx.bloods = &bloods;
	ctx.lights = &lights;
	ctx.have_terrain = false;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Inside;
	ctx.building_id = -1;
	ctx.mine = INT2(0,0);
	ctx.maxe = INT2(w,h);
	ctx.tmp_ctx = NULL;
	ctx.masks = NULL;
}

//=================================================================================================
void SingleInsideLocation::Save(HANDLE file, bool local)
{
	InsideLocation::Save(file, local);

	if(last_visit != -1)
		SaveLevel(file, local);
}

//=================================================================================================
void SingleInsideLocation::Load(HANDLE file, bool local)
{
	InsideLocation::Load(file, local);

	if(last_visit != -1)
		LoadLevel(file, local);
}

//=================================================================================================
void SingleInsideLocation::BuildRefidTable()
{
	InsideLocationLevel::BuildRefidTable();
}
