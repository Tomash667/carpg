#include "Pch.h"
#include "GameCore.h"
#include "InsideBuilding.h"
#include "Game.h"
#include "SaveState.h"
#include "Content.h"
#include "Door.h"
#include "BuildingGroup.h"
#include "GameFile.h"

//=================================================================================================
InsideBuilding::~InsideBuilding()
{
	DeleteElements(objects);
	DeleteElements(units);
	DeleteElements(doors);
	DeleteElements(items);
	DeleteElements(usables);
}

//=================================================================================================
void InsideBuilding::ApplyContext(LevelContext& ctx)
{
	ctx.units = &units;
	ctx.objects = &objects;
	ctx.chests = nullptr;
	ctx.traps = nullptr;
	ctx.doors = &doors;
	ctx.items = &items;
	ctx.usables = &usables;
	ctx.bloods = &bloods;
	ctx.lights = &lights;
	ctx.have_terrain = false;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Building;
	ctx.mine = Int2(level_shift.x * 256, level_shift.y * 256);
	ctx.maxe = ctx.mine + Int2(256, 256);
	ctx.tmp_ctx = nullptr;
	ctx.masks = (!masks.empty() ? &masks : nullptr);
}

//=================================================================================================
void InsideBuilding::Save(GameWriter& f, bool local)
{
	f << offset;
	f << inside_spawn;
	f << outside_spawn;
	f << xsphere_pos;
	f << enter_area;
	f << exit_area;
	f << outside_rot;
	f << top;
	f << xsphere_radius;
	f << type->id;
	f << level_shift;
	f << arena1;
	f << arena2;
	f << enter_y;

	f << units.size();
	for(Unit* unit : units)
		unit->Save(f, local);

	f << doors.size();
	for(Door* door : doors)
		door->Save(f, local);

	f << objects.size();
	for(Object* object : objects)
		object->Save(f);

	f << items.size();
	for(GroundItem* item : items)
		item->Save(f);

	f << usables.size();
	for(Usable* usable : usables)
		usable->Save(f, local);

	f << bloods.size();
	for(Blood& blood : bloods)
		blood.Save(f);

	f << lights.size();
	for(Light& light : lights)
		light.Save(f);

	if(local)
	{
		f << ctx.pes->size();
		for(ParticleEmitter* pe : *ctx.pes)
			pe->Save(f);

		f << ctx.tpes->size();
		for(TrailParticleEmitter* tpe : *ctx.tpes)
			tpe->Save(f);

		f << ctx.explos->size();
		for(Explo* explo : *ctx.explos)
			explo->Save(f);

		f << ctx.electros->size();
		for(Electro* electro : *ctx.electros)
			electro->Save(f);

		f << ctx.drains->size();
		for(Drain& drain : *ctx.drains)
			drain.Save(f);

		f << ctx.bullets->size();
		for(Bullet& bullet : *ctx.bullets)
			bullet.Save(f);
	}
}

//=================================================================================================
void InsideBuilding::Load(GameReader& f, bool local)
{
	ApplyContext(ctx);

	f >> offset;
	f >> inside_spawn;
	f >> outside_spawn;
	f >> xsphere_pos;
	f >> enter_area;
	f >> exit_area;
	f >> outside_rot;
	f >> top;
	f >> xsphere_radius;
	if(LOAD_VERSION >= V_0_5)
		type = Building::Get(f.ReadString1());
	else
	{
		OLD_BUILDING old_type;
		f >> old_type;
		type = Building::GetOld(old_type);
	}
	assert(type != nullptr);
	f >> level_shift;
	f >> arena1;
	f >> arena2;
	if(LOAD_VERSION >= V_0_3)
		f >> enter_y;
	else
		enter_y = 1.1f;

	units.resize(f.Read<uint>());
	for(Unit*& unit : units)
	{
		unit = new Unit;
		Unit::AddRefid(unit);
		unit->Load(f, local);
	}

	doors.resize(f.Read<uint>());
	for(Door*& door : doors)
	{
		door = new Door;
		door->Load(f, local);
	}

	objects.resize(f.Read<uint>());
	for(Object*& object : objects)
	{
		object = new Object;
		object->Load(f);
	}

	items.resize(f.Read<uint>());
	for(GroundItem*& item : items)
	{
		item = new GroundItem;
		item->Load(f);
	}

	usables.resize(f.Read<uint>());
	for(Usable*& usable : usables)
	{
		usable = new Usable;
		Usable::AddRefid(usable);
		usable->Load(f, local);
	}

	bloods.resize(f.Read<uint>());
	for(Blood& blood : bloods)
		blood.Load(f);

	lights.resize(f.Read<uint>());
	for(Light& light : lights)
		light.Load(f);

	if(local)
	{
		ctx.SetTmpCtx(Game::Get().tmp_ctx_pool.Get());

		ctx.pes->resize(f.Read<uint>());
		for(ParticleEmitter*& pe : *ctx.pes)
		{
			pe = new ParticleEmitter;
			ParticleEmitter::AddRefid(pe);
			pe->Load(f);
		}

		ctx.tpes->resize(f.Read<uint>());
		for(TrailParticleEmitter*& tpe : *ctx.tpes)
		{
			tpe = new TrailParticleEmitter;
			TrailParticleEmitter::AddRefid(tpe);
			tpe->Load(f);
		}

		ctx.explos->resize(f.Read<uint>());
		for(Explo*& explo : *ctx.explos)
		{
			explo = new Explo;
			explo->Load(f);
		}

		ctx.electros->resize(f.Read<uint>());
		for(Electro*& electro : *ctx.electros)
		{
			electro = new Electro;
			electro->Load(f);
		}

		ctx.drains->resize(f.Read<uint>());
		for(Drain& drain : *ctx.drains)
			drain.Load(f);

		ctx.bullets->resize(f.Read<uint>());
		for(Bullet& bullet : *ctx.bullets)
			bullet.Load(f);
	}
}

//=================================================================================================
Unit* InsideBuilding::FindUnit(const UnitData* ud) const
{
	assert(ud);

	for(vector<Unit*>::const_iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		if((*it)->data == ud)
			return *it;
	}

	assert(0);
	return nullptr;
}
