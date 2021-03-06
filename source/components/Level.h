#pragma once

//-----------------------------------------------------------------------------
#include "LevelArea.h"
#include "ObjectEntity.h"
#include "GameCamera.h"
#include "GameCommon.h"

//-----------------------------------------------------------------------------
enum EnterFrom
{
	ENTER_FROM_PORTAL = 0,
	ENTER_FROM_OUTSIDE = -1,
	ENTER_FROM_PREV_LEVEL = -2,
	ENTER_FROM_NEXT_LEVEL = -3
};

//-----------------------------------------------------------------------------
enum WarpTo
{
	WARP_OUTSIDE = -1,
	WARP_ARENA = -2
};

//-----------------------------------------------------------------------------
enum class CanLeaveLocationResult
{
	Yes,
	TeamTooFar,
	InCombat
};

//-----------------------------------------------------------------------------
class Level
{
	struct UnitWarpData
	{
		Unit* unit;
		int where;
		int building;
	};

public:
	Level();
	~Level();
	void LoadLanguage();
	void Init();
	void Reset();
	Location* GetLocation() { return location; }
	int GetDungeonLevel() { return dungeon_level; }
	void WarpUnit(Unit* unit, int where, int building = -1)
	{
		UnitWarpData& uwd = Add1(unit_warp_data);
		uwd.unit = unit;
		uwd.where = where;
		uwd.building = building;
	}
	void ProcessUnitWarps();
	void ProcessRemoveUnits(bool leave);
	vector<std::reference_wrapper<LevelArea>>& ForEachArea() { return areas; }
	void Apply();
	LevelArea& GetArea(const Vec3& pos);
	LevelArea* GetAreaById(int area_id);
	Unit* FindUnit(int id);
	Unit* FindUnit(delegate<bool(Unit*)> pred);
	Unit* FindUnit(UnitData* ud);
	Usable* FindUsable(int id);
	Door* FindDoor(int id);
	Trap* FindTrap(int id);
	Trap* FindTrap(BaseTrap* base, const Vec3& pos);
	Chest* FindChest(int id);
	Chest* GetRandomChest(Room& room);
	Chest* GetTreasureChest();
	Electro* FindElectro(int id);
	bool RemoveTrap(int id);
	void RemoveOldTrap(BaseTrap* baseTrap, Unit* owner, uint maxAllowed);
	void RemoveUnit(Unit* unit, bool notify = true);
	void RemoveUnit(UnitData* ud, bool on_leave);
	// for object rot must be 0, PI/2, PI or PI*3/2
	ObjectEntity SpawnObjectEntity(LevelArea& area, BaseObject* base, const Vec3& pos, float rot, float scale = 1.f, int flags = 0,
		Vec3* out_point = nullptr, int variant = -1);
	enum SpawnObjectExtrasFlags
	{
		SOE_DONT_SPAWN_PARTICLES = 1 << 0,
		SOE_MAGIC_LIGHT = 1 << 1,
		SOE_DONT_CREATE_LIGHT = 1 << 2
	};
	void SpawnObjectExtras(LevelArea& area, BaseObject* obj, const Vec3& pos, float rot, void* user_ptr, float scale = 1.f, int flags = 0);
	void ProcessBuildingObjects(LevelArea& area, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, GameDirection dir,
		const Vec3& shift, Building* building, CityBuilding* city_building, bool recreate = false, Vec3* out_point = nullptr);
	void RecreateObjects(bool spawn_pes = true);
	ObjectEntity SpawnObjectNearLocation(LevelArea& area, BaseObject* obj, const Vec2& pos, float rot, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	ObjectEntity SpawnObjectNearLocation(LevelArea& area, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	Object* SpawnObject(BaseObject* obj, const Vec3& pos, float rot)
	{
		return SpawnObjectEntity(*local_area, obj, pos, rot);
	}
	void PickableItemBegin(LevelArea& area, Object& o);
	bool PickableItemAdd(const Item* item);
	void PickableItemsFromStock(LevelArea& area, Object& o, Stock& stock);
	void AddGroundItem(LevelArea& area, GroundItem* item);
	GroundItem* FindGroundItem(int id, LevelArea** area = nullptr);
	GroundItem* SpawnGroundItemInsideAnyRoom(const Item* item);
	GroundItem* SpawnGroundItemInsideRoom(Room& room, const Item* item);
	GroundItem* SpawnGroundItemInsideRadius(const Item* item, const Vec2& pos, float radius, bool try_exact = false);
	GroundItem* SpawnGroundItemInsideRegion(const Item* item, const Vec2& pos, const Vec2& region_size, bool try_exact);
	Unit* CreateUnit(UnitData& base, int level = -1, bool create_physics = true);
	Unit* CreateUnitWithAI(LevelArea& area, UnitData& unit, int level = -1, const Vec3* pos = nullptr, const float* rot = nullptr);
	Vec3 FindSpawnPos(Room* room, Unit* unit);
	Unit* SpawnUnitInsideRoom(Room& room, UnitData& unit, int level = -1, const Int2& awayPt = Int2(-1000, -1000), const Int2& excludedPt = Int2(-1000, -1000));
	Unit* SpawnUnitInsideRoomS(Room& room, UnitData& unit, int level = -1) { return SpawnUnitInsideRoom(room, unit, level); }
	Unit* SpawnUnitInsideRoomOrNear(Room& room, UnitData& unit, int level = -1, const Int2& awayPt = Int2(-1000, -1000), const Int2& excludedPt = Int2(-1000, -1000));
	Unit* SpawnUnitNearLocation(LevelArea& area, const Vec3& pos, UnitData& unit, const Vec3* look_at = nullptr, int level = -1, float extra_radius = 2.f);
	Unit* SpawnUnitInsideRegion(LevelArea& area, const Box2d& region, UnitData& unit, int level = -1);
	enum SpawnUnitFlags
	{
		SU_MAIN_ROOM = 1 << 0,
		SU_TEMPORARY = 1 << 1
	};
	Unit* SpawnUnitInsideInn(UnitData& unit, int level = -1, InsideBuilding* inn = nullptr, int flags = 0);
	void SpawnUnitsGroup(LevelArea& area, const Vec3& pos, const Vec3* look_at, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback);
	Unit* SpawnUnit(LevelArea& area, TmpSpawn spawn);
	struct IgnoreObjects
	{
		const Unit** ignored_units; // nullptr or array of units with last element of nullptr
		const void** ignored_objects; // nullptr or array of objects/usable objects with last element of nullptr
		bool ignore_blocks, ignore_objects, ignore_units, ignore_doors;
	};
	void GatherCollisionObjects(LevelArea& area, vector<CollisionObject>& objects, const Vec3& pos, float radius, const IgnoreObjects* ignore = nullptr);
	void GatherCollisionObjects(LevelArea& area, vector<CollisionObject>& objects, const Box2d& box, const IgnoreObjects* ignore = nullptr);
	bool Collide(const vector<CollisionObject>& objects, const Vec3& pos, float radius);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin = 0.f);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin, float rot);
	bool CollideWithStairs(const CollisionObject& cobj, const Vec3& pos, float radius) const;
	bool CollideWithStairsRect(const CollisionObject& cobj, const Box2d& box) const;
	void CreateBlood(LevelArea& area, const Unit& unit, bool fully_created = false);
	void SpawnBlood();
	void WarpUnit(Unit& unit, const Vec3& pos);
	bool WarpToRegion(LevelArea& area, const Box2d& region, float radius, Vec3& pos, int tries = 10);
	void WarpNearLocation(LevelArea& area, Unit& uint, const Vec3& pos, float extra_radius, bool allow_exact, int tries = 20);
	// return pointer to temporary or nullptr (can fail only for arrow and poison traps)
	Trap* CreateTrap(Int2 pt, TRAP_TYPE type);
	Trap* CreateTrap(const Vec3& pos, TRAP_TYPE type, int id = -1);
	void UpdateLocation(int days, int open_chance, bool reset);
	int GetDifficultyLevel() const;
	int GetChestDifficultyLevel() const;
	void OnRevisitLevel();
	bool HaveArena();
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
	bool CanSee(LevelArea& area, const Vec3& v1, const Vec3& v2, bool is_door = false, void* ignored = nullptr);
	void KillAll(bool friendly, Unit& unit, Unit* ignore);
	void AddPlayerTeam(const Vec3& pos, float rot);
	void UpdateDungeonMinimap(bool in_level);
	void RevealMinimap();
	bool IsSettlement() { return city_ctx != nullptr; }
	bool IsCity();
	bool IsVillage();
	bool IsTutorial();
	bool IsOutside();
	void Update();
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f, bool loaded_resources);
	MusicType GetLocationMusic();
	void CleanLevel(int building_id = -2);
	GroundItem* SpawnItem(const Item* item, const Vec3& pos);
	GroundItem* SpawnItemAtObject(const Item* item, Object* obj);
	void SpawnItemRandomly(const Item* item, uint count);
	Unit* GetNearestEnemy(Unit* unit);
	Unit* SpawnUnitNearLocationS(UnitData* ud, const Vec3& pos, float range = 2.f, int level = -1);
	GroundItem* FindNearestItem(const Item* item, const Vec3& pos);
	GroundItem* FindItem(const Item* item);
	Unit* GetMayor();
	bool IsSafe();
	bool CanFastTravel();
	CanLeaveLocationResult CanLeaveLocation(Unit& unit, bool check_dist = true);
	void SetOutsideParams();
	bool CanShootAtLocation(const Unit& me, const Unit& target, const Vec3& pos) const { return CanShootAtLocation2(me, &target, pos); }
	bool CanShootAtLocation(const Vec3& from, const Vec3& to) const;
	bool CanShootAtLocation2(const Unit& me, const void* ptr, const Vec3& to) const;
	bool RayTest(const Vec3& from, const Vec3& to, Unit* ignore, Vec3& hitpoint, Unit*& hitted);
	bool LineTest(btCollisionShape* shape, const Vec3& from, const Vec3& dir, delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, float& t,
		vector<float>* t_list = nullptr, bool use_clbk2 = false, float* end_t = nullptr);
	bool ContactTest(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool use_clbk2 = false);
	int CheckMove(Vec3& pos, const Vec3& dir, float radius, Unit* me, bool* is_small = nullptr);
	void SpawnUnitEffect(Unit& unit);
	MeshInstance* GetBowInstance(Mesh* mesh);
	void FreeBowInstance(MeshInstance*& mesh_inst)
	{
		assert(mesh_inst);
		bow_instances.push_back(mesh_inst);
		mesh_inst = nullptr;
	}
	CityBuilding* GetRandomBuilding(BuildingGroup* group);
	Room* GetRoom(RoomTarget target);
	Room* GetFarRoom();
	Object* FindObjectInRoom(Room& room, BaseObject* base);
	CScriptArray* FindPath(Room& from, Room& to);
	CScriptArray* GetUnits(Room& room);
	bool FindPlaceNearWall(BaseObject& obj, SpawnPoint& point);
	void CreateObjectsMeshInstance();
	void RemoveTmpObjectPhysics();
	void RecreateTmpObjectPhysics();
	Vec3 GetSpawnCenter();
	// --- boss
	void StartBossFight(Unit& unit);
	void EndBossFight();
	// ---
	void CreateSpellParticleEffect(LevelArea* area, Ability* ability, const Vec3& pos, const Vec2& bounds);

	Location* location; // same as world->current_location
	int location_index; // same as world->current_location_index
	int dungeon_level;
	InsideLocationLevel* lvl; // null when in outside location
	GameCamera camera;
	vector<std::reference_wrapper<LevelArea>> areas;
	LevelArea* local_area;
	Unit* boss;

	// colliders
	btHeightfieldTerrainShape* terrain_shape;
	btCollisionObject* obj_terrain;
	btCollisionShape* shape_wall, *shape_stairs, *shape_stairs_part[2], *shape_block, *shape_barrier, *shape_door, *shape_arrow, *shape_summon, *shape_floor;
	btBvhTriangleMeshShape* dungeon_shape;
	btCollisionObject* obj_dungeon;
	SimpleMesh* dungeon_mesh;
	btTriangleIndexVertexArray* dungeon_shape_data;
	vector<btCollisionShape*> shapes;
	vector<CameraCollider> cam_colliders;

	Scene* scene;
	Terrain* terrain;
	LocationEventHandler* event_handler;
	City* city_ctx; // pointer to city or nullptr when not inside city
	int enter_from; // from where team entered level (used when spawning new player in MP)
	float light_angle; // random angle used for lighting in outside locations
	bool is_open, // is location loaded & team is inside
		entering, // true when entering location/generating/spawning unit, false when finished
		can_fast_travel; // used by MP clients
	vector<Unit*> to_remove;
	vector<CollisionObject> global_col;
	vector<Unit*> blood_to_spawn;

	// minimap
	bool minimap_opened_doors;
	vector<Int2> minimap_reveal, minimap_reveal_mp;
	uint minimap_size;

private:
	vector<MeshInstance*> bow_instances;
	vector<UnitWarpData> unit_warp_data;

	// pickable items
	struct PickableItem
	{
		uint spawn;
		Vec3 pos;
	};
	LevelArea* pickable_area;
	Object* pickable_obj;
	vector<Box> pickable_spawns;
	vector<PickableItem> pickable_items;
	vector<ItemSlot> pickable_tmp_stock;

	cstring txLocationText, txLocationTextMap, txWorldMap, txNewsCampCleared, txNewsLocCleared;
};
