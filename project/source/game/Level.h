#pragma once

#include "LevelContext.h"
#include "ObjectEntity.h"

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
	void LoadData();
	void Reset();
	void WarpUnit(Unit* unit, int where)
	{
		UnitWarpData& uwd = Add1(unit_warp_data);
		uwd.unit = unit;
		uwd.where = where;
	}
	void ProcessUnitWarps();
	void ProcessRemoveUnits(bool clear);
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
	// for object rot must be 0, PI/2, PI or PI*3/2
	ObjectEntity SpawnObjectEntity(LevelContext& ctx, BaseObject* base, const Vec3& pos, float rot, float scale = 1.f, int flags = 0,
		Vec3* out_point = nullptr, int variant = -1);
	enum SpawnObjectExtrasFlags
	{
		SOE_DONT_SPAWN_PARTICLES = 1 << 0,
		SOE_MAGIC_LIGHT = 1 << 1,
		SOE_DONT_CREATE_LIGHT = 1 << 2
	};
	void SpawnObjectExtras(LevelContext& ctx, BaseObject* obj, const Vec3& pos, float rot, void* user_ptr, float scale = 1.f, int flags = 0);
	// roti jest u¿ywane tylko do ustalenia czy k¹t jest zerowy czy nie, mo¿na przerobiæ t¹ funkcjê ¿eby tego nie u¿ywa³a wogóle
	void ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, int roti,
		const Vec3& shift, Building* type, CityBuilding* building, bool recreate = false, Vec3* out_point = nullptr);
	ObjectEntity SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, float rot, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	ObjectEntity SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	void PickableItemBegin(LevelContext& ctx, Object& o);
	void PickableItemAdd(const Item* item);
	void SpawnUnitsGroup(LevelContext& ctx, const Vec3& pos, const Vec3* look_at, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback);

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
	TexturePtr tFlare, tFlare2, tWater;
	Matrix m1, m2, m3;
	// pickable items
	struct PickableItem
	{
		uint spawn;
		Vec3 pos;
	};
	LevelContext* pickable_ctx;
	Object* pickable_obj;
	vector<Box> pickable_spawns;
	vector<PickableItem> pickable_items;
};
extern Level L;
