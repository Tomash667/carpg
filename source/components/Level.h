#pragma once

//-----------------------------------------------------------------------------
#include "LocationPart.h"
#include "ObjectEntity.h"
#include "GameCamera.h"
#include "GameCommon.h"
#include "Collision.h"

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
	int GetDungeonLevel() { return dungeonLevel; }
	void WarpUnit(Unit* unit, int where, int building = -1)
	{
		UnitWarpData& uwd = Add1(unitWarpData);
		uwd.unit = unit;
		uwd.where = where;
		uwd.building = building;
	}
	void ProcessUnitWarps();
	void ProcessRemoveUnits(bool leave);
	vector<std::reference_wrapper<LocationPart>>& ForEachPart() { return locParts; }
	void Apply();
	LocationPart& GetLocationPart(const Vec3& pos);
	LocationPart* GetLocationPartById(int partId);
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
	void RemoveUnit(UnitData* ud, bool onLeave);
	// for object rot must be 0, PI/2, PI or PI*3/2
	ObjectEntity SpawnObjectEntity(LocationPart& locPart, BaseObject* base, const Vec3& pos, float rot, float scale = 1.f, int flags = 0,
		Vec3* outPoint = nullptr, int variant = -1);
	enum SpawnObjectExtrasFlags
	{
		SOE_DONT_SPAWN_PARTICLES = 1 << 0,
		SOE_MAGIC_LIGHT = 1 << 1,
		SOE_DONT_CREATE_LIGHT = 1 << 2
	};
	void SpawnObjectExtras(LocationPart& locPart, BaseObject* obj, const Vec3& pos, float rot, void* userPtr, float scale = 1.f, int flags = 0);
	void ProcessBuildingObjects(LocationPart& locPart, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* insideMesh, float rot, GameDirection dir,
		const Vec3& shift, Building* building, CityBuilding* cityBuilding, bool recreate = false, Vec3* outPoint = nullptr);
	void RecreateObjects(bool spawnParticles = true);
	ObjectEntity SpawnObjectNearLocation(LocationPart& locPart, BaseObject* obj, const Vec2& pos, float rot, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	ObjectEntity SpawnObjectNearLocation(LocationPart& locPart, BaseObject* obj, const Vec2& pos, const Vec2& rotTarget, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	Object* SpawnObject(BaseObject* obj, const Vec3& pos, float rot)
	{
		return SpawnObjectEntity(*localPart, obj, pos, rot);
	}
	void PickableItemBegin(LocationPart& locPart, Object& o);
	bool PickableItemAdd(const Item* item);
	void PickableItemsFromStock(LocationPart& locPart, Object& o, Stock& stock);
	GroundItem* FindGroundItem(int id, LocationPart** locPartResult = nullptr);
	GroundItem* SpawnGroundItemInsideAnyRoom(const Item* item);
	GroundItem* SpawnGroundItemInsideRoom(Room& room, const Item* item);
	GroundItem* SpawnGroundItemInsideRadius(const Item* item, const Vec2& pos, float radius, bool tryExact = false);
	GroundItem* SpawnGroundItemInsideRegion(const Item* item, const Vec2& pos, const Vec2& regionSize, bool tryExact);
	Unit* CreateUnit(UnitData& base, int level = -1, bool createPhysics = true);
	Unit* CreateUnitWithAI(LocationPart& locPart, UnitData& unit, int level = -1, const Vec3* pos = nullptr, const float* rot = nullptr);
	Vec3 FindSpawnPos(Room* room, Unit* unit);
	Vec3 FindSpawnPos(LocationPart& locPart, Unit* unit);
	Unit* SpawnUnitInsideRoom(Room& room, UnitData& unit, int level = -1, const Int2& awayPt = Int2(-1000, -1000), const Int2& excludedPt = Int2(-1000, -1000));
	Unit* SpawnUnitInsideRoomS(Room& room, UnitData& unit, int level = -1) { return SpawnUnitInsideRoom(room, unit, level); }
	Unit* SpawnUnitInsideRoomOrNear(Room& room, UnitData& unit, int level = -1, const Int2& awayPt = Int2(-1000, -1000), const Int2& excludedPt = Int2(-1000, -1000));
	Unit* SpawnUnitNearLocation(LocationPart& locPart, const Vec3& pos, UnitData& unit, const Vec3* lookAt = nullptr, int level = -1, float extraRadius = 2.f);
	Unit* SpawnUnitInsideRegion(LocationPart& locPart, const Box2d& region, UnitData& unit, int level = -1);
	enum SpawnUnitFlags
	{
		SU_MAIN_ROOM = 1 << 0,
		SU_TEMPORARY = 1 << 1
	};
	Unit* SpawnUnitInsideInn(UnitData& unit, int level = -1, InsideBuilding* inn = nullptr, int flags = 0);
	void SpawnUnitsGroup(LocationPart& locPart, const Vec3& pos, const Vec3* lookAt, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback);
	Unit* SpawnUnit(LocationPart& locPart, TmpSpawn spawn);
	Unit* SpawnUnit(LocationPart& locPart, UnitData& unit, int level = -1);
	void SpawnUnits(UnitGroup* group, int level);
	struct IgnoreObjects
	{
		const Unit** ignoredUnits; // nullptr or array of units with last element of nullptr
		const void** ignoredObjects; // nullptr or array of objects/usable objects with last element of nullptr
		bool ignoreBlocks, ignoreObjects, ignoreUnits, ignoreDoors;
	};
	void GatherCollisionObjects(LocationPart& locPart, vector<CollisionObject>& objects, const Vec3& pos, float radius, const IgnoreObjects* ignore = nullptr);
	void GatherCollisionObjects(LocationPart& locPart, vector<CollisionObject>& objects, const Box2d& box, const IgnoreObjects* ignore = nullptr);
	bool Collide(const vector<CollisionObject>& objects, const Vec3& pos, float radius);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin = 0.f);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin, float rot);
	bool CollideWithStairs(const CollisionObject& cobj, const Vec3& pos, float radius) const;
	bool CollideWithStairsRect(const CollisionObject& cobj, const Box2d& box) const;
	void CreateBlood(LocationPart& locPart, const Unit& unit, bool fullyCreated = false);
	void SpawnBlood();
	void WarpUnit(Unit& unit, const Vec3& pos);
	bool WarpToRegion(LocationPart& locPart, const Box2d& region, float radius, Vec3& pos, int tries = 10);
	void WarpNearLocation(LocationPart& locPart, Unit& uint, const Vec3& pos, float extraRadius, bool allowExact, int tries = 20);
	Trap* TryCreateTrap(Int2 pt, TRAP_TYPE type);
	Trap* CreateTrap(const Vec3& pos, TRAP_TYPE type, int id = -1);
	void UpdateLocation(int days, int openChance, bool reset);
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
	Vec3 GetExitPos(Unit& u, bool forceBorder = false);
	bool CanSee(Unit& unit, Unit& unit2);
	bool CanSee(LocationPart& locPart, const Vec3& v1, const Vec3& v2, bool isDoor = false, void* ignored = nullptr);
	void KillAll(bool friendly, Unit& unit, Unit* ignore);
	void AddPlayerTeam(const Vec3& pos, float rot);
	void UpdateDungeonMinimap(bool inLevel);
	void RevealMinimap();
	bool IsSettlement() { return cityCtx != nullptr; }
	bool IsSafeSettlement();
	bool IsCity();
	bool IsVillage();
	bool IsTutorial();
	bool IsOutside();
	void Update();
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f, bool loadedResources);
	MusicType GetLocationMusic();
	void CleanLevel(int buildingId = -2);
	GroundItem* SpawnItem(const Item* item, const Vec3& pos);
	GroundItem* SpawnItemNearLocation(LocationPart& locPart, const Item* item);
	GroundItem* SpawnItemAtObject(const Item* item, Object* obj);
	void SpawnItemRandomly(const Item* item, uint count);
	Unit* GetNearestEnemy(Unit* unit);
	Unit* SpawnUnitNearLocationS(UnitData* ud, const Vec3& pos, float range = 2.f, int level = -1);
	GroundItem* FindNearestItem(const Item* item, const Vec3& pos);
	GroundItem* FindItem(const Item* item);
	Unit* GetMayor();
	bool IsSafe();
	bool CanFastTravel();
	CanLeaveLocationResult CanLeaveLocation(Unit& unit, bool checkDist = true);
	bool CanShootAtLocation(const Unit& me, const Unit& target, const Vec3& pos) const { return CanShootAtLocation2(me, &target, pos); }
	bool CanShootAtLocation(const Vec3& from, const Vec3& to) const;
	bool CanShootAtLocation2(const Unit& me, const void* ptr, const Vec3& to) const;
	bool RayTest(const Vec3& from, const Vec3& to, Unit* ignore, Vec3& hitpoint, Unit*& hitted);
	bool LineTest(btCollisionShape* shape, const Vec3& from, const Vec3& dir, delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, float& t,
		vector<float>* tList = nullptr, bool useClbk2 = false, float* endT = nullptr);
	bool ContactTest(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool useClbk2 = false);
	int CheckMove(Vec3& pos, const Vec3& dir, float radius, Unit* me, bool* isSmall = nullptr);
	void SpawnUnitEffect(Unit& unit);
	MeshInstance* GetBowInstance(Mesh* mesh);
	void FreeBowInstance(MeshInstance*& meshInst)
	{
		assert(meshInst);
		bowInstances.push_back(meshInst);
		meshInst = nullptr;
	}
	CityBuilding* GetRandomBuilding(BuildingGroup* group);
	Room* GetRoom(RoomTarget target);
	Room* GetFarRoom();
	Object* FindObjectInRoom(Room& room, BaseObject* base);
	CScriptArray* FindPath(Room& from, Room& to);
	CScriptArray* GetUnits();
	CScriptArray* GetUnits(Room& room);
	CScriptArray* GetNearbyUnits(const Vec3& pos, float dist);
	bool FindPlaceNearWall(BaseObject& obj, SpawnPoint& point);
	void CreateObjectsMeshInstance();
	void RemoveTmpObjectPhysics();
	void RecreateTmpObjectPhysics();
	Vec3 GetSpawnCenter();
	// --- boss
	void StartBossFight(Unit& unit);
	void EndBossFight();
	// ---
	void CreateSpellParticleEffect(LocationPart* locPart, Ability* ability, const Vec3& pos, const Vec2& bounds);

	Location* location; // same as world->current_location
	int locationIndex; // same as world->current_location_index
	int dungeonLevel;
	InsideLocationLevel* lvl; // null when in outside location
	GameCamera camera;
	LocationPart* localPart;
	Unit* boss;

	// colliders
	btHeightfieldTerrainShape* terrainShape;
	btCollisionObject* objTerrain;
	btCollisionShape* shapeWall, *shapeStairs, *shapeStairsPart[2], *shapeBlock, *shapeBarrier, *shapeDoor, *shapeArrow, *shapeSummon, *shapeFloor;
	btBvhTriangleMeshShape* dungeonShape;
	btCollisionObject* objDungeon;
	SimpleMesh* dungeonMesh;
	btTriangleIndexVertexArray* dungeonShapeData;
	vector<btCollisionShape*> shapes;
	vector<CameraCollider> camColliders;

	Terrain* terrain;
	LocationEventHandler* eventHandler;
	City* cityCtx; // pointer to city or nullptr when not inside city
	int enterFrom; // from where team entered level (used when spawning new player in MP)
	float lightAngle; // random angle used for lighting in outside locations
	bool isOpen, // is location loaded & team is inside
		ready, // true when everything is read, false when entering/loading/leaving location
		canFastTravel; // used by MP clients
	vector<Unit*> toRemove;
	vector<CollisionObject> globalCol;
	vector<Unit*> bloodToSpawn;

	// minimap
	vector<Int2> minimapReveal, minimapRevealMp;
	uint minimapSize;
	bool minimapOpenedDoors;

private:
	vector<std::reference_wrapper<LocationPart>> locParts;
	vector<MeshInstance*> bowInstances;
	vector<UnitWarpData> unitWarpData;

	// pickable items
	struct PickableItem
	{
		uint spawn;
		Vec3 pos;
	};
	LocationPart* pickableLocPart;
	Object* pickableObj;
	vector<Box> pickableSpawns;
	vector<PickableItem> pickableItems;
	vector<ItemSlot> pickableTmpStock;

	cstring txLocationText, txLocationTextMap, txWorldMap, txNewsCampCleared, txNewsLocCleared;
};
