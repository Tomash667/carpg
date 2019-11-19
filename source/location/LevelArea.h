#pragma once

//-----------------------------------------------------------------------------
#include "Collision.h"
#include "Bullet.h"
#include "SpellEffects.h"
#include "Blood.h"
#include "Light.h"

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
// Part of Location Level (outside, building or dungeon level)
struct LevelArea
{
	enum class Type
	{
		Outside,
		Inside,
		Building
	};

	static const int OUTSIDE_ID = -1;
	static const int OLD_EXIT_ID = -2;

	const int area_id; // -1 outside, 0+ building or dungeon level
	const Type area_type;
	Scene* scene;
	TmpLevelArea* tmp;
	vector<Unit*> units;
	vector<Object*> objects;
	vector<Usable*> usables;
	vector<Door*> doors;
	vector<Chest*> chests;
	vector<GroundItem*> items;
	vector<Trap*> traps;
	vector<Blood> bloods;
	vector<Light> lights;
	vector<LightMask> masks;
	Int2 mine, maxe;
	const bool have_terrain;
	bool is_active;

	LevelArea(Type area_type, int area_id, bool have_terrain) : area_type(area_type), area_id(area_id),
		have_terrain(have_terrain), scene(nullptr), tmp(nullptr), is_active(false) {}
	~LevelArea();
	void Save(GameWriter& f);
	void Load(GameReader& f, old::LoadCompatibility compatibility = old::LoadCompatibility::None);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void Clear();
	cstring GetName();
	Unit* FindUnit(UnitData* ud);
	Usable* FindUsable(BaseUsable* base);
	bool RemoveItem(const Item* item);
	bool FindItemInCorpse(const Item* item, Unit** unit, int* slot);
	bool RemoveGroundItem(const Item* item);
	bool FindItemInChest(const Item* item, Chest** chest, int* slot);
	Object* FindObject(BaseObject* base_obj);
	Chest* FindChestInRoom(const Room& p);
	Chest* GetRandomFarChest(const Int2& pt);
	bool HaveUnit(Unit* unit);
	Chest* FindChestWithItem(const Item* item, int* index);
	Chest* FindChestWithQuestItem(int quest_id, int* index);
	Door* FindDoor(const Int2& pt);
	void CreateNodes();
};

//-----------------------------------------------------------------------------
// Temporary level area (used only for active level areas to hold temporary entities)
struct TmpLevelArea : ObjectPoolProxy<TmpLevelArea>
{
	LevelArea* area;
	vector<Bullet> bullets;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*> tpes;
	vector<Explo*> explos;
	vector<Electro*> electros;
	vector<Drain> drains;
	vector<CollisionObject> colliders;

	void Clear();
	void Save(GameWriter& f);
	void Load(GameReader& f);
};

//-----------------------------------------------------------------------------
class LevelAreaContext
{
	friend struct ForLocation;

	struct Entry
	{
		LevelArea* area;
		int loc, level;
		bool active;
	};

	vector<Entry> entries;

public:
	GroundItem* FindQuestGroundItem(int quest_id, LevelAreaContext::Entry** entry = nullptr, int* item_index = nullptr);
	Unit* FindUnitWithQuestItem(int quest_id, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr, int* item_iindex = nullptr);
	bool FindUnit(Unit* unit, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(UnitData* data, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(delegate<bool(Unit*)> clbk, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	bool RemoveQuestGroundItem(int quest_id);
	bool RemoveQuestItemFromUnit(int quest_id);
	bool RemoveUnit(Unit* unit);
};

//-----------------------------------------------------------------------------
struct ForLocation
{
	ForLocation(int loc, int level = -1);
	ForLocation(const ForLocation& f) = delete;
	~ForLocation();
	ForLocation& operator = (const ForLocation& f) = delete;
	LevelAreaContext* operator -> () { return ctx; }

private:
	LevelAreaContext* ctx;
};
