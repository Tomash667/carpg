#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"
#include "InsideBuilding.h"
#include "EntryPoint.h"

//-----------------------------------------------------------------------------
// Budynek w mieœcie
struct CityBuilding
{
	Building* type;
	INT2 pt, unit_pt;
	int rot;
	VEC3 walk_pt;

	CityBuilding() {}
	explicit CityBuilding(Building* type) : type() {}
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
	enum class SettlementType
	{
		Village,
		City
	};

	enum Flags
	{
		HaveExit = 1 << 0,
		HaveTrainingGrounds = 1 << 1,
		HaveBlacksmith = 1 << 2,
		HaveMerchant = 1 << 3,
		HaveAlchemist = 1 << 4,
		HaveFoodSeller = 1 << 5,
		HaveInn = 1 << 6,
		HaveArena = 1 << 7
	};

	SettlementType settlement_type;
	int citizens, citizens_world, quest_mayor_time, quest_captain_time, arena_time, gates, flags, variant;
	CityQuestState quest_mayor, quest_captain;
	vector<CityBuilding> buildings; // when visited this contain buildings to spawn (only type), after entering it is fully filled
	vector<InsideBuilding*> inside_buildings;
	INT2 inside_offset;
	VEC3 arena_pos;
	vector<EntryPoint> entry_points;

	City() : quest_mayor(CityQuestState::None), quest_captain(CityQuestState::None), quest_mayor_time(-1), quest_captain_time(-1),
		inside_offset(1,0), arena_time(-1), flags(HaveExit), settlement_type(SettlementType::City), variant(-1)
	{

	}

	virtual ~City();

	// from Location
	virtual void Save(HANDLE file, bool local) override;
	virtual void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	virtual void BuildRefidTable() override;
	virtual bool FindUnit(Unit* unit, int* level) override;
	virtual Unit* FindUnit(UnitData* data, int& at_level) override;
	inline virtual LOCATION_TOKEN GetToken() const override { return LT_CITY; }

	/*inline CityBuilding* FindBuilding(BUILDING building_type)
	{
		for(vector<CityBuilding>::iterator it = buildings.begin(), end = buildings.end(); it != end; ++it)
		{
			if(it->type == building_type)
				return &*it;
		}
		return nullptr;
	}

	inline InsideBuilding* FindInsideBuilding(BUILDING building_type)
	{
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it)
		{
			if((*it)->type == building_type)
				return *it;
		}
		return nullptr;
	}

	inline InsideBuilding* FindInsideBuilding(BUILDING building_type, int& id)
	{
		id = 0;
		for(vector<InsideBuilding*>::iterator it = inside_buildings.begin(), end = inside_buildings.end(); it != end; ++it, ++id)
		{
			if((*it)->type == building_type)
				return *it;
		}
		id = -1;
		return nullptr;
	}*/

	

	//Unit* FindUnitInsideBuilding(const UnitData* ud, BUILDING building_type) const;

	inline bool IsInsideCity(const VEC3& _pos)
	{
		INT2 tile(int(_pos.x/2), int(_pos.z/2));
		if(tile.x <= int(0.15f*size) || tile.y <= int(0.15f*size) || tile.x >= int(0.85f*size) || tile.y >= int(0.85f*size))
			return false;
		else
			return true;
	}

	inline InsideBuilding* FindInsideBuilding(Building* type)
	{
		assert(type);
		for(InsideBuilding* i : inside_buildings)
		{
			if(i->type == type)
				return i;
		}
		return nullptr;
	}

	inline InsideBuilding* FindInsideBuilding(BuildingGroup* group)
	{
		assert(group >= 0);
		for(InsideBuilding* i : inside_buildings)
		{
			if(i->type->group == group)
				return i;
		}
		return nullptr;
	}

	inline InsideBuilding* FindInsideBuilding(BuildingGroup* group, int& index)
	{
		assert(group >= 0);
		index = 0;
		for(InsideBuilding* i : inside_buildings)
		{
			if(i->type->group == group)
				return i;
			++index;
		}
		index = -1;
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

	inline CityBuilding* FindBuilding(BuildingGroup* group)
	{
		assert(group >= 0);
		for(CityBuilding& b : buildings)
		{
			if(b.type->group == group)
				return &b;
		}
		return nullptr;
	}

	inline CityBuilding* FindBuilding(Building* type)
	{
		assert(type);
		for(CityBuilding& b : buildings)
		{
			if(b.type == type)
				return &b;
		}
		return nullptr;
	}

	inline bool IsVillage() const
	{
		return settlement_type == SettlementType::Village;
	}
};
