// miasto
#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"
#include "InsideBuilding.h"
#include "EntryPoint.h"

//-----------------------------------------------------------------------------
// Budynek w mieœcie
struct CityBuilding
{
	BUILDING type;
	INT2 pt, unit_pt;
	int rot;
	VEC3 walk_pt;
};

//-----------------------------------------------------------------------------
enum class CityQuestState
{
	None,
	InProgress,
	Failed
};

//-----------------------------------------------------------------------------
struct City : public OutsideLocation
{
	int citizens, citizens_world, quest_mayor_time, quest_captain_time, arena_time, gates;
	CityQuestState quest_mayor, quest_captain;
	vector<CityBuilding> buildings;
	vector<InsideBuilding*> inside_buildings;
	INT2 inside_offset;
	VEC3 arena_pos;
	vector<EntryPoint> entry_points;
	bool have_exit;

	City() : quest_mayor(CityQuestState::None), quest_captain(CityQuestState::None), quest_mayor_time(-1), quest_captain_time(-1), inside_offset(1,0), arena_time(-1), have_exit(true)
	{

	}

	virtual ~City();

	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);

	inline CityBuilding* FindBuilding(BUILDING type)
	{
		for(vector<CityBuilding>::iterator it = buildings.begin(), end = buildings.end(); it != end; ++it)
		{
			if(it->type == type)
				return &*it;
		}
		return nullptr;
	}

	inline InsideBuilding* FindInsideBuilding(BUILDING type)
	{
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it)
		{
			if((*it)->type == type)
				return *it;
		}
		return nullptr;
	}

	inline InsideBuilding* FindInsideBuilding(BUILDING type, int& id)
	{
		id = 0;
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it, ++id)
		{
			if((*it)->type == type)
				return *it;
		}
		id = -1;
		return nullptr;
	}

	virtual void BuildRefidTable();
	virtual bool FindUnit(Unit* unit, int* level);
	virtual Unit* FindUnit(UnitData* data, int& at_level);
	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_CITY;
	}

	Unit* FindUnitInsideBuilding(const UnitData* ud, BUILDING building_type) const;

	inline bool IsInsideCity(const VEC3& pos)
	{
		INT2 tile(int(pos.x/2), int(pos.z/2));
		if(tile.x <= int(0.15f*size) || tile.y <= int(0.15f*size) || tile.x >= int(0.85f*size) || tile.y >= int(0.85f*size))
			return false;
		else
			return true;
	}

	inline InsideBuilding* FindInsideBuilding(BUILDING_GROUP group)
	{
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it)
		{
			if(::buildings[(*it)->type].group == group)
				return *it;
		}
		return nullptr;
	}

	inline InsideBuilding* FindInsideBuilding(BUILDING_GROUP group, int& id)
	{
		id = 0;
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it, ++id)
		{
			if(::buildings[(*it)->type].group == group)
				return *it;
		}
		id = -1;
		return nullptr;
	}

	inline InsideBuilding* FindInn()
	{
		return FindInsideBuilding(BG_INN);
	}

	inline InsideBuilding* FindInn(int& id)
	{
		return FindInsideBuilding(BG_INN, id);
	}

	inline CityBuilding* FindBuilding(BUILDING_GROUP group)
	{
		for(vector<CityBuilding>::iterator it = buildings.begin(), end = buildings.end(); it != end; ++it)
		{
			if(::buildings[it->type].group == group)
				return &*it;
		}
		return nullptr;
	}
};
