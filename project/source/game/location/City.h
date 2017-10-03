#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"
#include "InsideBuilding.h"
#include "EntryPoint.h"
#include "Content.h"
#include "BuildingGroup.h"

//-----------------------------------------------------------------------------
// Budynek w mieœcie
struct CityBuilding
{
	Building* type;
	Int2 pt, unit_pt;
	int rot;
	Vec3 walk_pt;

	CityBuilding() {}
	explicit CityBuilding(Building* type) : type(type) {}
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
		// saved as byte in PreparedWorldData
	};

	SettlementType settlement_type;
	int citizens, citizens_world, quest_mayor_time, quest_captain_time, arena_time, gates, flags, variant;
	CityQuestState quest_mayor, quest_captain;
	vector<CityBuilding> buildings; // when visited this contain buildings to spawn (only type), after entering it is fully filled
	vector<InsideBuilding*> inside_buildings;
	Int2 inside_offset;
	Vec3 arena_pos;
	vector<EntryPoint> entry_points;

	City() : quest_mayor(CityQuestState::None), quest_captain(CityQuestState::None), quest_mayor_time(-1), quest_captain_time(-1),
		inside_offset(1, 0), arena_time(-1), flags(HaveExit), settlement_type(SettlementType::City), variant(-1)
	{
	}

	~City();

	// from Location
	void Save(HANDLE file, bool local) override;
	void Load(HANDLE file, bool local, LOCATION_TOKEN token) override;
	void BuildRefidTable() override;
	bool FindUnit(Unit* unit, int* level) override;
	Unit* FindUnit(UnitData* data, int& at_level) override;
	LOCATION_TOKEN GetToken() const override { return LT_CITY; }

	bool IsInsideCity(const Vec3& _pos);
	InsideBuilding* FindInsideBuilding(Building* type);
	InsideBuilding* FindInsideBuilding(BuildingGroup* group);
	InsideBuilding* FindInsideBuilding(BuildingGroup* group, int& index);
	InsideBuilding* FindInn() { return FindInsideBuilding(BuildingGroup::BG_INN); }
	InsideBuilding* FindInn(int& id) { return FindInsideBuilding(BuildingGroup::BG_INN, id); }
	CityBuilding* FindBuilding(BuildingGroup* group);
	CityBuilding* FindBuilding(Building* type);
	bool IsVillage() const { return settlement_type == SettlementType::Village; }
};
