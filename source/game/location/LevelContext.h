// kontekst poziomu
#pragma once

//-----------------------------------------------------------------------------
#include "Unit.h"
#include "Chest.h"
#include "Object.h"
#include "Trap.h"
#include "Door.h"
#include "GroundItem.h"
#include "Useable.h"
#include "SpellEffects.h"
#include "ParticleSystem.h"
#include "Collision.h"
#include "Mapa2.h"
#include "Bullet.h"

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

	inline void Clear()
	{
		bullets.clear();
		DeleteElements(pes);
		DeleteElements(tpes);
		DeleteElements(explos);
		DeleteElements(electros);
		drains.clear();
		colliders.clear();
	}
};

//-----------------------------------------------------------------------------
struct LightMask
{
	VEC2 pos, size;
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
	vector<Object>* objects;
	vector<Chest*>* chests; // nullable
	vector<Trap*>* traps; // nullable
	vector<Door*>* doors; // nullable
	vector<GroundItem*>* items;
	vector<Useable*>* useables;
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
	INT2 mine, maxe;
	bool have_terrain, require_tmp_ctx;

	inline void SetTmpCtx(TmpLevelContext* tmp)
	{
		assert(tmp);
		tmp_ctx = tmp;
		bullets = &tmp->bullets;
		pes = &tmp->pes;
		tpes = &tmp->tpes;
		explos = &tmp->explos;
		electros = &tmp->electros;
		drains = &tmp->drains;
		colliders = &tmp->colliders;
		tmp->Clear();
	}

	void RemoveDeadUnits();

	inline Unit* FindUnitById(UnitData* ud)
	{
		assert(ud);

		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			if((*it)->data == ud)
				return *it;
		}

		return nullptr;
	}

	inline Object* FindObjectById(Obj* obj)
	{
		assert(obj);

		for(vector<Object>::iterator it = objects->begin(), end = objects->end(); it != end; ++it)
		{
			if(it->base == obj)
				return &*it;
		}

		return nullptr;
	}

	inline Useable* FindUseableById(int type)
	{
		for(vector<Useable*>::iterator it = useables->begin(), end = useables->end(); it != end; ++it)
		{
			if((*it)->type == type)
				return *it;
		}

		return nullptr;
	}

	bool RemoveItemFromWorld(const Item* item);
	bool FindItemInCorpse(const Item* item, Unit** unit, int* slot);
	bool RemoveGroundItem(const Item* item);
	bool FindItemInChest(const Item* item, Chest** chest, int* slot);
	Object* FindObject(Obj* obj);
	Chest* FindChestInRoom(const Room& p);
	Chest* GetRandomFarChest(const INT2& pt);
};

//-----------------------------------------------------------------------------
// interfejs poziomu (mo¿e byæ to zewnêtrzna mapa, podziemia lub budynek)
struct ILevel
{
	virtual void ApplyContext(LevelContext& ctx) = 0;
};
