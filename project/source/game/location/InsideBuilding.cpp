#include "Pch.h"
#include "GameCore.h"
#include "InsideBuilding.h"
#include "SaveState.h"
#include "Content.h"
#include "Door.h"
#include "BuildingGroup.h"
#include "GameFile.h"
#include "Level.h"
#include "Object.h"
#include "GroundItem.h"
#include "ParticleSystem.h"
#include "BitStreamFunc.h"
#include "Game.h"

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
		ctx.SetTmpCtx(L.tmp_ctx_pool.Get());

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
void InsideBuilding::Write(BitStreamWriter& f)
{
	f << level_shift;
	f << type->id;
	// usable objects
	f.WriteCasted<byte>(usables.size());
	for(Usable* usable : usables)
		usable->Write(f);
	// units
	f.WriteCasted<byte>(units.size());
	for(Unit* unit : units)
		unit->Write(f);
	// doors
	f.WriteCasted<byte>(doors.size());
	for(Door* door : doors)
		door->Write(f);
	// ground items
	f.WriteCasted<byte>(items.size());
	for(GroundItem* item : items)
		item->Write(f);
	// bloods
	f.WriteCasted<word>(bloods.size());
	for(Blood& blood : bloods)
		blood.Write(f);
	// objects
	f.WriteCasted<byte>(objects.size());
	for(Object* object : objects)
		object->Write(f);
	// lights
	f.WriteCasted<byte>(lights.size());
	for(Light& light : lights)
		light.Write(f);
	// other
	f << xsphere_pos;
	f << enter_area;
	f << exit_area;
	f << top;
	f << xsphere_radius;
	f << enter_y;
}

//=================================================================================================
bool InsideBuilding::Load(BitStreamReader& f)
{
	byte count;

	f >> level_shift;
	const string& building_id = f.ReadString1();
	if(!f)
	{
		Error("Broken packet for inside building.");
		return false;
	}
	type = Building::Get(building_id);
	if(!type || !type->inside_mesh)
	{
		Error("Invalid building id '%s'.", building_id.c_str());
		return false;
	}
	offset = Vec2(512.f*level_shift.x + 256.f, 512.f*level_shift.y + 256.f);
	L.ProcessBuildingObjects(ctx, L.city_ctx, this, type->inside_mesh, nullptr, 0, 0, Vec3(offset.x, 0, offset.y), type, nullptr, true);

	// usable objects
	f >> count;
	if(!f.Ensure(Usable::MIN_SIZE * count))
	{
		Error("Broken packet for usable object count.");
		return false;
	}
	usables.resize(count);
	for(Usable*& usable : usables)
	{
		usable = new Usable;
		if(!usable->Read(f))
		{
			Error("Broken packet for usable objects.");
			return false;
		}
	}

	// units
	f >> count;
	if(!f.Ensure(Unit::MIN_SIZE * count))
	{
		Error("Broken packet for unit count.");
		return false;
	}
	units.resize(count);
	for(Unit*& unit : units)
	{
		unit = new Unit;
		if(!unit->Read(f))
		{
			Error("Broken packet for units.");
			return false;
		}
		unit->in_building = ctx.building_id;
	}

	// doors
	f >> count;
	if(!f.Ensure(count * Door::MIN_SIZE))
	{
		Error("Broken packet for door count.");
		return false;
	}
	doors.resize(count);
	for(Door*& door : doors)
	{
		door = new Door;
		if(!door->Read(f))
		{
			Error("Broken packet for doors.");
			return false;
		}
	}

	// ground items
	f >> count;
	if(!f.Ensure(count * GroundItem::MIN_SIZE))
	{
		Error("Broken packet for ground item count.");
		return false;
	}
	items.resize(count);
	for(GroundItem*& item : items)
	{
		item = new GroundItem;
		if(!item->Read(f))
		{
			Error("Broken packet for ground items.");
			return false;
		}
	}

	// bloods
	word count2;
	f >> count2;
	if(!f.Ensure(count2 * Blood::MIN_SIZE))
	{
		Error("Broken packet for blood count.");
		return false;
	}
	bloods.resize(count2);
	for(Blood& blood : bloods)
		blood.Read(f);
	if(!f)
	{
		Error("Broken packet for bloods.");
		return false;
	}

	// objects
	f >> count;
	if(!f.Ensure(count * Object::MIN_SIZE))
	{
		Error("Broken packet for object count.");
		return false;
	}
	objects.resize(count);
	for(Object*& object : objects)
	{
		object = new Object;
		if(!object->Read(f))
		{
			Error("Broken packet for objects.");
			return false;
		}
	}

	// lights
	f >> count;
	if(!f.Ensure(count * Light::MIN_SIZE))
	{
		Error("Broken packet for light count.");
		return false;
	}
	lights.resize(count);
	for(Light& light : lights)
		light.Read(f);
	if(!f)
	{
		Error("Broken packet for lights.");
		return false;
	}

	// other
	f >> xsphere_pos;
	f >> enter_area;
	f >> exit_area;
	f >> top;
	f >> xsphere_radius;
	f >> enter_y;
	if(!f)
	{
		Error("Broken packet for other data.");
		return false;
	}

	return true;
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
