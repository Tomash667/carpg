#pragma once

//-----------------------------------------------------------------------------
#include "Bullet.h"
#include "Blood.h"
#include "GameLight.h"
#include <Mesh.h>

//-----------------------------------------------------------------------------
struct LightMask
{
	Vec2 pos, size;
};

//-----------------------------------------------------------------------------
// pre V_0_11 compatibility
namespace old
{
	enum class LoadCompatibility
	{
		None,
		InsideBuilding,
		InsideLocationLevel,
		InsideLocationLevelTraps,
		OutsideLocation
	};
}

//-----------------------------------------------------------------------------
// Part of location (outside, building or dungeon level)
struct LocationPart
{
	enum class Type
	{
		Outside,
		Inside,
		Building
	};

	static const int OUTSIDE_ID = -1;
	static const int OLD_EXIT_ID = -2; // pre V_0_11

	const int partId; // -1 outside, 0+ building or dungeon level
	const Type partType;
	LevelPart* lvlPart;
	vector<Unit*> units;
	vector<Object*> objects;
	vector<Usable*> usables;
	vector<Door*> doors;
	vector<Chest*> chests;
private:
	vector<GroundItem*> groundItems;
public:
	vector<Trap*> traps;
	vector<Blood> bloods;
	vector<GameLight> lights;
	vector<LightMask> masks;
	Int2 mine, maxe;
	const bool haveTerrain;

	LocationPart(Type partType, int partId, bool haveTerrain) : partType(partType), partId(partId), haveTerrain(haveTerrain), lvlPart(nullptr) {}
	~LocationPart();
	void BuildScene();
	void Update(float dt);
	void Save(GameWriter& f);
	void Load(GameReader& f, old::LoadCompatibility compatibility = old::LoadCompatibility::None);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void Clear();
	Unit* FindUnit(UnitData* ud);
	Usable* FindUsable(BaseUsable* base);
	bool RemoveItem(const Item* item);
	bool FindItemInCorpse(const Item* item, Unit** unit, int* slot);
	Object* FindObject(BaseObject* baseObj);
	Object* FindNearestObject(BaseObject* baseObj, const Vec3& pos);
	Chest* FindChestInRoom(const Room& p);
	Chest* GetRandomFarChest(const Int2& pt);
	bool HaveUnit(Unit* unit);
	Chest* FindChestWithItem(const Item* item, int* index);
	Chest* FindChestWithQuestItem(int questId, int* index);
	bool RemoveItemFromChest(const Item* item);
	bool RemoveItemFromUnit(const Item* item);
	Door* FindDoor(const Int2& pt);
	bool IsActive() const { return lvlPart != nullptr; }
	void SpellHitEffect(Bullet& bullet, const Vec3& pos, Unit* hitted);
	bool CheckForHit(Unit& unit, Unit*& hitted, Vec3& hitpoint);
	bool CheckForHit(Unit& unit, Unit*& hitted, Mesh::Point& hitbox, Mesh::Point* bone, Vec3& hitpoint);
	Explo* CreateExplo(Ability* ability, const Vec3& pos);
	void DestroyUsable(Usable* usable);

	// ground items
	vector<GroundItem*>& GetGroundItems() { return groundItems; }
	void AddGroundItem(GroundItem* groundItem, bool adjustY = true);
	bool RemoveGroundItem(const Item* item);
	void RemoveGroundItem(int questId);
	void RemoveGroundItem(GroundItem* item, bool del = true);
};
