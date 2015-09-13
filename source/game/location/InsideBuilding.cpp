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
	ctx.chests = NULL;
	ctx.traps = NULL;
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
	ctx.tmp_ctx = NULL;
	ctx.masks = (!masks.empty() ? &masks : NULL);
}

//=================================================================================================
void InsideBuilding::Save(HANDLE file, bool local)
{
	WriteFile(file, &offset, sizeof(offset), &tmp, NULL);
	WriteFile(file, &inside_spawn, sizeof(inside_spawn), &tmp, NULL);
	WriteFile(file, &outside_spawn, sizeof(outside_spawn), &tmp, NULL);
	WriteFile(file, &xsphere_pos, sizeof(xsphere_pos), &tmp, NULL);
	WriteFile(file, &enter_area, sizeof(enter_area), &tmp, NULL);
	WriteFile(file, &exit_area, sizeof(exit_area), &tmp, NULL);
	WriteFile(file, &outside_rot, sizeof(outside_rot), &tmp, NULL);
	WriteFile(file, &top, sizeof(top), &tmp, NULL);
	WriteFile(file, &xsphere_radius, sizeof(xsphere_radius), &tmp, NULL);
	WriteFile(file, &type, sizeof(type), &tmp, NULL);
	WriteFile(file, &level_shift, sizeof(level_shift), &tmp, NULL);
	WriteFile(file, &arena1, sizeof(arena1), &tmp, NULL);
	WriteFile(file, &arena2, sizeof(arena2), &tmp, NULL);
	WriteFile(file, &enter_y, sizeof(enter_y), &tmp, NULL);

	uint ile = units.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		(*it)->Save(file, local);

	ile = doors.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
		(*it)->Save(file, local);

	ile = objects.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		it->Save(file);

	ile = items.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		(*it)->Save(file);

	ile = useables.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
		(*it)->Save(file, local);

	FileWriter f(file);
	ile = bloods.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Save(f);

	f << lights.size();
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Save(f);

	if(local)
	{
		ile = ctx.pes->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<ParticleEmitter*>::iterator it = ctx.pes->begin(), end = ctx.pes->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.tpes->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<TrailParticleEmitter*>::iterator it = ctx.tpes->begin(), end = ctx.tpes->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.explos->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Explo*>::iterator it = ctx.explos->begin(), end = ctx.explos->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.electros->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Electro*>::iterator it = ctx.electros->begin(), end = ctx.electros->end(); it != end; ++it)
			(*it)->Save(file);

		ile = ctx.drains->size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
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

	ReadFile(file, &offset, sizeof(offset), &tmp, NULL);
	ReadFile(file, &inside_spawn, sizeof(inside_spawn), &tmp, NULL);
	ReadFile(file, &outside_spawn, sizeof(outside_spawn), &tmp, NULL);
	ReadFile(file, &xsphere_pos, sizeof(xsphere_pos), &tmp, NULL);
	ReadFile(file, &enter_area, sizeof(enter_area), &tmp, NULL);
	ReadFile(file, &exit_area, sizeof(exit_area), &tmp, NULL);
	ReadFile(file, &outside_rot, sizeof(outside_rot), &tmp, NULL);
	ReadFile(file, &top, sizeof(top), &tmp, NULL);
	ReadFile(file, &xsphere_radius, sizeof(xsphere_radius), &tmp, NULL);
	ReadFile(file, &type, sizeof(type), &tmp, NULL);
	ReadFile(file, &level_shift, sizeof(level_shift), &tmp, NULL);
	ReadFile(file, &arena1, sizeof(arena1), &tmp, NULL);
	ReadFile(file, &arena2, sizeof(arena2), &tmp, NULL);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &enter_y, sizeof(enter_y), &tmp, NULL);
	else
		enter_y = 1.1f;

	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	units.resize(ile);
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		*it = new Unit;
		Unit::AddRefid(*it);
		(*it)->Load(file, local);
	}

	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	doors.resize(ile);
	for(vector<Door*>::iterator it = doors.begin(), end = doors.end(); it != end; ++it)
	{
		*it = new Door;
		(*it)->Load(file, local);
	}

	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	objects.resize(ile);
	for(vector<Object>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
		it->Load(file);

	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	items.resize(ile);
	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		*it = new GroundItem;
		(*it)->Load(file);
	}

	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	useables.resize(ile);
	for(vector<Useable*>::iterator it = useables.begin(), end = useables.end(); it != end; ++it)
	{
		*it = new Useable;
		Useable::AddRefid(*it);
		(*it)->Load(file, local);
	}

	FileReader f(file);
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	bloods.resize(ile);
	for(vector<Blood>::iterator it = bloods.begin(), end = bloods.end(); it != end; ++it)
		it->Load(f);

	f >> ile;
	lights.resize(ile);
	for(vector<Light>::iterator it = lights.begin(), end = lights.end(); it != end; ++it)
		it->Load(f);

	if(local)
	{
		ctx.SetTmpCtx(Game::_game->tmp_ctx_pool.Get());

		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		ctx.pes->resize(ile);
		for(vector<ParticleEmitter*>::iterator it = ctx.pes->begin(), end = ctx.pes->end(); it != end; ++it)
		{
			*it = new ParticleEmitter;
			ParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		ctx.tpes->resize(ile);
		for(vector<TrailParticleEmitter*>::iterator it = ctx.tpes->begin(), end = ctx.tpes->end(); it != end; ++it)
		{
			*it = new TrailParticleEmitter;
			TrailParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		ctx.explos->resize(ile);
		for(vector<Explo*>::iterator it = ctx.explos->begin(), end = ctx.explos->end(); it != end; ++it)
		{
			*it = new Explo;
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		ctx.electros->resize(ile);
		for(vector<Electro*>::iterator it = ctx.electros->begin(), end = ctx.electros->end(); it != end; ++it)
		{
			*it = new Electro;
			(*it)->Load(file);
		}

		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
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
			if(u.type == U_KRZESLO)
				u.type = U_STOLEK;
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
	return NULL;
}
