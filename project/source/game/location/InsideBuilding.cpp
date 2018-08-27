// wnêtrze budynku
#include "Pch.h"
#include "GameCore.h"
#include "InsideBuilding.h"
#include "Game.h"
#include "SaveState.h"
#include "Content.h"
#include "Door.h"
#include "BuildingGroup.h"

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
void InsideBuilding::Save(HANDLE file, bool local)
{
	FileWriter f(file);

	WriteFile(file, &offset, sizeof(offset), &tmp, nullptr);
	WriteFile(file, &inside_spawn, sizeof(inside_spawn), &tmp, nullptr);
	WriteFile(file, &outside_spawn, sizeof(outside_spawn), &tmp, nullptr);
	WriteFile(file, &xsphere_pos, sizeof(xsphere_pos), &tmp, nullptr);
	WriteFile(file, &enter_area, sizeof(enter_area), &tmp, nullptr);
	WriteFile(file, &exit_area, sizeof(exit_area), &tmp, nullptr);
	WriteFile(file, &outside_rot, sizeof(outside_rot), &tmp, nullptr);
	WriteFile(file, &top, sizeof(top), &tmp, nullptr);
	WriteFile(file, &xsphere_radius, sizeof(xsphere_radius), &tmp, nullptr);
	WriteString1(file, type->id);
	WriteFile(file, &level_shift, sizeof(level_shift), &tmp, nullptr);
	WriteFile(file, &arena1, sizeof(arena1), &tmp, nullptr);
	WriteFile(file, &arena2, sizeof(arena2), &tmp, nullptr);
	WriteFile(file, &enter_y, sizeof(enter_y), &tmp, nullptr);

	uint ile = units.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		(*it)->Save(file, local);

	f << doors.size();
	for(Door* door : doors)
		door->Save(f, local);

	f << objects.size();
	for(Object* object : objects)
		object->Save(f);

	f << items.size();
	for(GroundItem* item : items)
		item->Save(f);

	ile = usables.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
		(*it)->Save(file, local);

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
void InsideBuilding::Load(HANDLE file, bool local)
{
	FileReader f(file);
	ApplyContext(ctx);

	ReadFile(file, &offset, sizeof(offset), &tmp, nullptr);
	ReadFile(file, &inside_spawn, sizeof(inside_spawn), &tmp, nullptr);
	ReadFile(file, &outside_spawn, sizeof(outside_spawn), &tmp, nullptr);
	ReadFile(file, &xsphere_pos, sizeof(xsphere_pos), &tmp, nullptr);
	ReadFile(file, &enter_area, sizeof(enter_area), &tmp, nullptr);
	ReadFile(file, &exit_area, sizeof(exit_area), &tmp, nullptr);
	ReadFile(file, &outside_rot, sizeof(outside_rot), &tmp, nullptr);
	ReadFile(file, &top, sizeof(top), &tmp, nullptr);
	ReadFile(file, &xsphere_radius, sizeof(xsphere_radius), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_5)
	{
		FileReader f(file);
		type = Building::Get(f.ReadString1());
	}
	else
	{
		OLD_BUILDING old_type;
		ReadFile(file, &old_type, sizeof(old_type), &tmp, nullptr);
		type = Building::GetOld(old_type);
	}
	assert(type != nullptr);
	ReadFile(file, &level_shift, sizeof(level_shift), &tmp, nullptr);
	ReadFile(file, &arena1, sizeof(arena1), &tmp, nullptr);
	ReadFile(file, &arena2, sizeof(arena2), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &enter_y, sizeof(enter_y), &tmp, nullptr);
	else
		enter_y = 1.1f;

	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	units.resize(ile);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		*it = new Unit;
		Unit::AddRefid(*it);
		(*it)->Load(file, local);
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

	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	usables.resize(ile);
	for(vector<Usable*>::iterator it = usables.begin(), end = usables.end(); it != end; ++it)
	{
		*it = new Usable;
		Usable::AddRefid(*it);
		(*it)->Load(file, local);
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
