// wnêtrze budynku
#include "Pch.h"
#include "Base.h"
#include "InsideBuilding.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
InsideBuilding::~InsideBuilding()
{
	DeleteElements(units);
	DeleteElements(doors);
	DeleteElements(items);
	DeleteElements(useables);
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
	ctx.useables = &useables;
	ctx.bloods = &bloods;
	ctx.lights = &lights;
	ctx.have_terrain = false;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Building;
	ctx.mine = INT2(level_shift.x*256, level_shift.y*256);
	ctx.maxe = ctx.mine + INT2(256,256);
	ctx.tmp_ctx = nullptr;
	ctx.masks = (!masks.empty() ? &masks : nullptr);
}

//=================================================================================================
void InsideBuilding::Save(HANDLE file, bool local)
{
	WriteFile(file, &offset, sizeof(offset), &tmp, nullptr);
	WriteFile(file, &inside_spawn, sizeof(inside_spawn), &tmp, nullptr);
	WriteFile(file, &outside_spawn, sizeof(outside_spawn), &tmp, nullptr);
	WriteFile(file, &xsphere_pos, sizeof(xsphere_pos), &tmp, nullptr);
	WriteFile(file, &enter_area, sizeof(enter_area), &tmp, nullptr);
	WriteFile(file, &exit_area, sizeof(exit_area), &tmp, nullptr);
	WriteFile(file, &outside_rot, sizeof(outside_rot), &tmp, nullptr);
	WriteFile(file, &top, sizeof(top), &tmp, nullptr);
	WriteFile(file, &xsphere_radius, sizeof(xsphere_radius), &tmp, nullptr);
	WriteFile(file, &type, sizeof(type), &tmp, nullptr);
	WriteFile(file, &level_shift, sizeof(level_shift), &tmp, nullptr);
	WriteFile(file, &arena1, sizeof(arena1), &tmp, nullptr);
	WriteFile(file, &arena2, sizeof(arena2), &tmp, nullptr);
	WriteFile(file, &enter_y, sizeof(enter_y), &tmp, nullptr);

	uint ile = units.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		(*it)->Save(file, local);

	ile = doors.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
		(*it)->Save(file, local);

	ile = objects.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		it->Save(file);

	ile = items.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		(*it)->Save(file);

	ile = useables.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
		(*it)->Save(file, local);

	FileWriter f(file);
	ile = bloods.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Save(f);

	f << lights.size();
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Save(f);

	if(local)
	{
		ile = ctx.pes->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(vector<ParticleEmitter*>::iterator it = ctx.pes->begin(), end = ctx.pes->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.tpes->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(vector<TrailParticleEmitter*>::iterator it = ctx.tpes->begin(), end = ctx.tpes->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.explos->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(vector<Explo*>::iterator it = ctx.explos->begin(), end = ctx.explos->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.electros->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(vector<Electro*>::iterator it = ctx.electros->begin(), end = ctx.electros->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.drains->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(vector<Drain>::iterator it = ctx.drains->begin(), end = ctx.drains->end(); it != end; ++it)
			it->Save(file);

		f << ctx.bullets->size();
		for(vector<Bullet>::iterator it = ctx.bullets->begin(), end = ctx.bullets->end(); it != end; ++it)
			it->Save(f);
	}
}

//=================================================================================================
void InsideBuilding::Load(HANDLE file, bool local)
{
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
	ReadFile(file, &type, sizeof(type), &tmp, nullptr);
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

	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	doors.resize(ile);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
	{
		*it = new Door;
		(*it)->Load(file, local);
	}

	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	objects.resize(ile);
	for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		it->Load(file);

	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	items.resize(ile);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		*it = new GroundItem;
		(*it)->Load(file);
	}

	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	useables.resize(ile);
	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
	{
		*it = new Useable;
		Useable::AddRefid(*it);
		(*it)->Load(file, local);
	}

	FileReader f(file);
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	bloods.resize(ile);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Load(f);

	f >> ile;
	lights.resize(ile);
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Load(f);

	if(local)
	{
		ctx.SetTmpCtx(Game::Get().tmp_ctx_pool.Get());

		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		ctx.pes->resize(ile);
		for(vector<ParticleEmitter*>::iterator it = ctx.pes->begin(), end = ctx.pes->end(); it != end; ++it)
		{
			*it = new ParticleEmitter;
			ParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		ctx.tpes->resize(ile);
		for(vector<TrailParticleEmitter*>::iterator it = ctx.tpes->begin(), end = ctx.tpes->end(); it != end; ++it)
		{
			*it = new TrailParticleEmitter;
			TrailParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		ctx.explos->resize(ile);
		for(vector<Explo*>::iterator it = ctx.explos->begin(), end = ctx.explos->end(); it != end; ++it)
		{
			*it = new Explo;
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		ctx.electros->resize(ile);
		for(vector<Electro*>::iterator it = ctx.electros->begin(), end = ctx.electros->end(); it != end; ++it)
		{
			*it = new Electro;
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		ctx.drains->resize(ile);
		for(vector<Drain>::iterator it = ctx.drains->begin(), end = ctx.drains->end(); it != end; ++it)
			it->Load(file);

		f >> ile;
		ctx.bullets->resize(ile);
		for(vector<Bullet>::iterator it = ctx.bullets->begin(), end = ctx.bullets->end(); it != end; ++it)
			it->Load(f);
	}

	// konwersja krzese³ w sto³ki
	if(LOAD_VERSION < V_0_2_12 && type == B_INN)
	{
		for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
		{
			Useable& u = **it;
			if(u.type == U_CHAIR)
				u.type = U_STOOL;
		}
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
