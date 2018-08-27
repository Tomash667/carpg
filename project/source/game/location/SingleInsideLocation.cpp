#include "Pch.h"
#include "GameCore.h"
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
	ctx.usables = &usables;
	ctx.bloods = &bloods;
	ctx.lights = &lights;
	ctx.have_terrain = false;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Inside;
	ctx.building_id = -1;
	ctx.mine = Int2(0, 0);
	ctx.maxe = Int2(w, h);
	ctx.tmp_ctx = nullptr;
	ctx.masks = nullptr;
}

//=================================================================================================
void SingleInsideLocation::Save(GameWriter& f, bool local)
{
	InsideLocation::Save(f, local);

	if(last_visit != -1)
		SaveLevel(f, local);
}

//=================================================================================================
void SingleInsideLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	InsideLocation::Load(f, local, token);

	if(last_visit != -1)
		LoadLevel(f, local);
}

//=================================================================================================
void SingleInsideLocation::BuildRefidTable()
{
	InsideLocationLevel::BuildRefidTable();
}

//=================================================================================================
bool SingleInsideLocation::FindUnit(Unit* unit, int* level)
{
	if(InsideLocationLevel::FindUnit(unit))
	{
		if(level)
			*level = 0;
		return true;
	}
	else
		return false;
}

//=================================================================================================
Unit* SingleInsideLocation::FindUnit(UnitData* data, int& at_level)
{
	Unit* u = InsideLocationLevel::FindUnit(data);
	if(u)
		at_level = 0;
	return u;
}

//=================================================================================================
Chest* SingleInsideLocation::FindChestWithItem(const Item* item, int& at_level, int* index)
{
	Chest* chest = InsideLocationLevel::FindChestWithItem(item, index);
	if(chest)
		at_level = 0;
	return chest;
}

//=================================================================================================
Chest* SingleInsideLocation::FindChestWithQuestItem(int quest_refid, int& at_level, int* index)
{
	Chest* chest = InsideLocationLevel::FindChestWithQuestItem(quest_refid, index);
	if(chest)
		at_level = 0;
	return chest;
}
