#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
#include "LevelContext.h"
#include "ObjectEntity.h"

//-----------------------------------------------------------------------------
enum EnterFrom
{
	ENTER_FROM_PORTAL = 0,
	ENTER_FROM_OUTSIDE = -1,
	ENTER_FROM_UP_LEVEL = -2,
	ENTER_FROM_DOWN_LEVEL = -3
};

//-----------------------------------------------------------------------------
enum WarpTo
{
	WARP_OUTSIDE = -1,
	WARP_ARENA = -2
};

//-----------------------------------------------------------------------------
class Level : public GameComponent
{
	struct UnitWarpData
	{
		Unit* unit;
		int where;
	};

public:
	Level();
	void LoadLanguage() override;
	void LoadData() override;
	void PostInit() override;
	void Cleanup() override;
	void Reset();
	void WarpUnit(Unit* unit, int where)
	{
		UnitWarpData& uwd = Add1(unit_warp_data);
		uwd.unit = unit;
		uwd.where = where;
	}
	void ProcessUnitWarps();
	void ProcessRemoveUnits(bool leave);
	LevelContextEnumerator ForEachContext() { return LevelContextEnumerator(location); }
	void ApplyContext(ILevel* level, LevelContext& ctx);
	LevelContext& GetContext(Unit& unit);
	LevelContext& GetContext(const Vec3& pos);
	LevelContext& GetContextFromInBuilding(int in_building);
	Unit* FindUnit(int netid);
	Unit* FindUnit(delegate<bool(Unit*)> pred);
	Usable* FindUsable(int netid);
	Door* FindDoor(int netid);
	Door* FindDoor(LevelContext& ctx, const Int2& pt);
	Trap* FindTrap(int netid);
	Chest* FindChest(int netid);
	Electro* FindElectro(int netid);
	bool RemoveTrap(int netid);
	void RemoveUnit(Unit* unit, bool notify = true);
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
	// roti jest u�ywane tylko do ustalenia czy k�t jest zerowy czy nie, mo�na przerobi� t� funkcj� �eby tego nie u�ywa�a wog�le
	void ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, int roti,
		const Vec3& shift, Building* type, CityBuilding* building, bool recreate = false, Vec3* out_point = nullptr);
	void RecreateObjects(bool spawn_pes = true);
	ObjectEntity SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, float rot, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	ObjectEntity SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	void PickableItemBegin(LevelContext& ctx, Object& o);
	void PickableItemAdd(const Item* item);
	void AddGroundItem(LevelContext& ctx, GroundItem* item);
	GroundItem* FindGroundItem(int netid, LevelContext** ctx = nullptr);
	GroundItem* SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item);
	GroundItem* SpawnGroundItemInsideRoom(Room& room, const Item* item);
	GroundItem* SpawnGroundItemInsideRadius(const Item* item, const Vec2& pos, float radius, bool try_exact = false);
	GroundItem* SpawnGroundItemInsideRegion(const Item* item, const Vec2& pos, const Vec2& region_size, bool try_exact);
	Unit* SpawnUnitInsideRoom(Room& room, UnitData& unit, int level = -1, const Int2& pt = Int2(-1000, -1000), const Int2& pt2 = Int2(-1000, -1000));
	Unit* SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Room& room, UnitData& unit, int level = -1, const Int2& pt = Int2(-1000, -1000), const Int2& pt2 = Int2(-1000, -1000));
	Unit* SpawnUnitNearLocation(LevelContext& ctx, const Vec3& pos, UnitData& unit, const Vec3* look_at = nullptr, int level = -1, float extra_radius = 2.f);
	Unit* SpawnUnitInsideArea(LevelContext& ctx, const Box2d& area, UnitData& unit, int level = -1);
	enum SpawnUnitFlags
	{
		SU_MAIN_ROOM = 1 << 0,
		SU_TEMPORARY = 1 << 1
	};
	Unit* SpawnUnitInsideInn(UnitData& unit, int level = -1, InsideBuilding* inn = nullptr, int flags = 0);
	void SpawnUnitsGroup(LevelContext& ctx, const Vec3& pos, const Vec3* look_at, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback);
	struct IgnoreObjects
	{
		// nullptr lub tablica jednostek zako�czona nullptr
		const Unit** ignored_units;
		// nullptr lub tablica obiekt�w [u�ywalnych lub nie] zako�czona nullptr
		const void** ignored_objects;
		// czy ignorowa� bloki
		bool ignore_blocks;
		// czy ignorowa� obiekty
		bool ignore_objects;
	};
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const Vec3& pos, float radius, const IgnoreObjects* ignore = nullptr);
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const Box2d& box, const IgnoreObjects* ignore = nullptr);
	bool Collide(const vector<CollisionObject>& objects, const Vec3& pos, float radius);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin = 0.f);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin, float rot);
	bool CollideWithStairs(const CollisionObject& co, const Vec3& pos, float radius) const;
	bool CollideWithStairsRect(const CollisionObject& co, const Box2d& box) const;
	void CreateBlood(LevelContext& ctx, const Unit& unit, bool fully_created = false);
	void SpawnBlood();
	void WarpUnit(Unit& unit, const Vec3& pos);
	bool WarpToArea(LevelContext& ctx, const Box2d& area, float radius, Vec3& pos, int tries = 10);
	void WarpToInn(Unit& unit);
	void WarpNearLocation(LevelContext& ctx, Unit& uint, const Vec3& pos, float extra_radius, bool allow_exact, int tries = 20);
	// zwraca tymczasowy wska�nik na stworzon� pu�apk� lub nullptr (mo�e si� nie uda� tylko dla ARROW i POISON)
	Trap* CreateTrap(Int2 pt, TRAP_TYPE type, bool timed = false);
	void UpdateLocation(int days, int open_chance, bool reset);
	int GetDifficultyLevel() const;
	int GetChestDifficultyLevel() const;
	void OnReenterLevel();
	InsideBuilding* GetArena();
	cstring GetCurrentLocationText();
	void CheckIfLocationCleared();
	bool RemoveItemFromWorld(const Item* item);
	void SpawnTerrainCollider();
	void SpawnDungeonColliders();
	void SpawnDungeonCollider(const Vec3& pos);
	void RemoveColliders();
	Int2 GetSpawnPoint();
	Vec3 GetExitPos(Unit& u, bool force_border = false);
	bool CanSee(Unit& unit, Unit& unit2);
	bool CanSee(const Vec3& v1, const Vec3& v2);
	bool KillAll(int mode, Unit& unit, Unit* ignore);
	void AddPlayerTeam(const Vec3& pos, float rot, bool reenter, bool hide_weapon);
	void UpdateDungeonMinimap(bool in_level);
	void RevealMinimap();
	bool IsCity();
	void Update();
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f, bool loaded_resources);
	MusicType GetLocationMusic();

	Location* location; // same as W.current_location
	int location_index; // same as W.current_location_index
	int dungeon_level;

	// colliders
	CustomCollisionWorld* phy_world;
	btHeightfieldTerrainShape* terrain_shape;
	btCollisionObject* obj_terrain;
	btCollisionShape* shape_wall, *shape_stairs, *shape_stairs_part[2], *shape_block, *shape_barrier, *shape_door, *shape_arrow, *shape_summon;
	btBvhTriangleMeshShape* dungeon_shape;
	btCollisionObject* obj_dungeon;
	vector<Vec3> dungeon_shape_pos;
	vector<int> dungeon_shape_index;
	btTriangleIndexVertexArray* dungeon_shape_data;
	vector<btCollisionShape*> shapes;
	vector<CameraCollider> cam_colliders;

	Terrain* terrain;
	LocationEventHandler* event_handler;
	LevelContext local_ctx;
	City* city_ctx; // pointer to city or nullptr when not inside city
	int enter_from; // from where team entered level (used when spawning new player in MP)
	float light_angle; // random angle used for lighting in outside locations
	bool is_open, // is location loaded, team is inside or is on world map and can reenter
		entering; // true when entering location/generating/spawning unit, false when finished
	vector<Unit*> to_remove;
	ObjectPool<TmpLevelContext> tmp_ctx_pool;
	vector<CollisionObject> global_col;
	vector<Unit*> blood_to_spawn;

	// minimap
	bool minimap_opened_doors;
	vector<Int2> minimap_reveal, minimap_reveal_mp;
	uint minimap_size;

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

	cstring txLocationText, txLocationTextMap, txNewsCampCleared, txNewsLocCleared;
};
extern Level L;
