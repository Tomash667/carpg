#pragma once

#include "LevelContext.h"

enum EnterFrom
{
	ENTER_FROM_PORTAL = 0,
	ENTER_FROM_OUTSIDE = -1,
	ENTER_FROM_UP_LEVEL = -2,
	ENTER_FROM_DOWN_LEVEL = -3
};

enum WarpTo
{
	WARP_OUTSIDE = -1,
	WARP_ARENA = -2
};

class Level
{
	struct UnitWarpData
	{
		Unit* unit;
		int where;
	};

public:
	void Reset();
	void WarpUnit(Unit* unit, int where)
	{
		UnitWarpData& uwd = Add1(unit_warp_data);
		uwd.unit = unit;
		uwd.where = where;
	}
	void ProcessUnitWarps();
	void ProcessRemoveUnits(bool clear);
	void SpawnOutsideBariers();
	LevelContextEnumerator ForEachContext() { return LevelContextEnumerator(location); }
	void ApplyContext(ILevel* level, LevelContext& ctx);
	LevelContext& GetContext(Unit& unit);
	LevelContext& GetContext(const Vec3& pos);
	LevelContext& GetContextFromInBuilding(int in_building);
	Unit* FindUnit(int netid);
	Unit* FindUnit(delegate<bool(Unit*)> pred);
	Usable* FindUsable(int netid);
	Door* FindDoor(int netid);
	Trap* FindTrap(int netid);
	Chest* FindChest(int netid);
	Electro* FindElectro(int netid);
	bool RemoveTrap(int netid);
	void RespawnUnits();

	Location* location; // same as W.current_location
	int location_index; // same as W.current_location_index
	int dungeon_level;
	LocationEventHandler* event_handler;
	LevelContext local_ctx;
	City* city_ctx; // pointer to city or nullptr when not inside city
	int enter_from; // from where team entered level (used when spawning new player in MP)
	float light_angle; // random angle used for lighting in outside locations
	bool is_open; // is location loaded, team is inside or is on world map and can reenter
	vector<Unit*> to_remove;
	ObjectPool<TmpLevelContext> tmp_ctx_pool;

private:
	vector<UnitWarpData> unit_warp_data;
};
extern Level L;
