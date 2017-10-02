// kontekst poziomu
#pragma once

//-----------------------------------------------------------------------------
#include "BaseUsable.h"
#include "Bullet.h"
#include "Collision.h"
#include "SpellEffects.h"

//-----------------------------------------------------------------------------
struct Blood;
struct Chest;
struct Door;
struct GroundItem;
struct Item;
struct Light;
struct BaseObject;
struct Object;
struct ParticleEmitter;
struct Room;
struct TrailParticleEmitter;
struct Trap;
struct Unit;
struct UnitData;
struct Usable;

//-----------------------------------------------------------------------------
struct TmpLevelContext
{
	vector<Bullet> bullets;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*> tpes;
	vector<Explo*> explos;
	vector<Electro*> electros;
	vector<Drain> drains;
	vector<CollisionObject> colliders;

	void Clear();
};

//-----------------------------------------------------------------------------
struct LightMask
{
	Vec2 pos, size;
};

//-----------------------------------------------------------------------------
struct LevelContext
{
	enum Type
	{
		Outside,
		Inside,
		Building
	};

	vector<Unit*>* units;
	vector<Object*>* objects;
	vector<Chest*>* chests; // nullable
	vector<Trap*>* traps; // nullable
	vector<Door*>* doors; // nullable
	vector<GroundItem*>* items;
	vector<Usable*>* usables;
	vector<Bullet>* bullets;
	vector<Blood>* bloods;
	vector<ParticleEmitter*>* pes;
	vector<TrailParticleEmitter*>* tpes;
	vector<Explo*>* explos;
	vector<Electro*>* electros;
	vector<Drain>* drains;
	vector<CollisionObject>* colliders;
	vector<Light>* lights;
	vector<LightMask>* masks; // nullable
	TmpLevelContext* tmp_ctx;
	Type type;
	int building_id;
	Int2 mine, maxe;
	bool have_terrain, require_tmp_ctx;

	void SetTmpCtx(TmpLevelContext* ctx);
	void RemoveDeadUnits();
	Unit* FindUnitById(UnitData* ud);
	Usable* FindUsable(BaseUsable* base);
	Usable* FindUsable(cstring id)
	{
		return FindUsable(BaseUsable::Get(id));
	}
	bool RemoveItemFromWorld(const Item* item);
	bool FindItemInCorpse(const Item* item, Unit** unit, int* slot);
	bool RemoveGroundItem(const Item* item);
	bool FindItemInChest(const Item* item, Chest** chest, int* slot);
	Object* FindObject(BaseObject* base_obj);
	Chest* FindChestInRoom(const Room& p);
	Chest* GetRandomFarChest(const Int2& pt);
};

//-----------------------------------------------------------------------------
// interfejs poziomu (mo¿e byæ to zewnêtrzna mapa, podziemia lub budynek)
struct ILevel
{
	virtual void ApplyContext(LevelContext& ctx) = 0;
};
