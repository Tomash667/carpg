#include "Pch.h"
#include "Trap.h"

#include "BitStreamFunc.h"
#include "GameFile.h"
#include "Net.h"
#include "SaveState.h"
#include "Unit.h"

EntityType<Trap>::Impl EntityType<Trap>::impl;

//=================================================================================================
void Trap::Save(GameWriter& f)
{
	f << id;
	f << base->type;
	f << pos;

	if(base->type == TRAP_ARROW || base->type == TRAP_POISON)
	{
		f << tile;
		f << dir;
	}
	else
		f << obj.rot.y;

	if(f.isLocal && base->type != TRAP_FIREBALL)
	{
		f << state;
		f << time;

		if(base->type == TRAP_SPEAR)
		{
			f << obj2.pos.y;
			f << hitted->size();
			for(Unit* unit : *hitted)
				f << unit->id;
		}
	}
}

//=================================================================================================
void Trap::Load(GameReader& f)
{
	TRAP_TYPE type;

	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	f >> type;
	f >> pos;
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid

	base = &BaseTrap::traps[type];
	hitted = nullptr;
	obj.pos = pos;
	obj.rot = Vec3(0, 0, 0);
	obj.scale = 1.f;
	obj.base = nullptr;
	obj.mesh = base->mesh;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		f >> tile;
		f >> dir;
	}
	else
		f >> obj.rot.y;

	if(type == TRAP_SPEAR)
	{
		obj2.pos = pos;
		obj2.rot = obj.rot;
		obj2.scale = 1.f;
		obj2.mesh = base->mesh2;
		obj2.base = nullptr;
	}

	if(f.isLocal && base->type != TRAP_FIREBALL)
	{
		f >> state;
		f >> time;

		if(base->type == TRAP_SPEAR)
		{
			f >> obj2.pos.y;
			uint count = f.Read<uint>();
			hitted = new vector<Unit*>;
			if(count)
			{
				hitted->resize(count);
				for(Unit*& unit : *hitted)
					unit = Unit::GetById(f.Read<int>());
			}
		}
	}
}

//=================================================================================================
void Trap::Write(BitStreamWriter& f)
{
	f << id;
	f.WriteCasted<byte>(base->type);
	f.WriteCasted<byte>(dir);
	f << tile;
	f << pos;
	f << obj.rot.y;

	if(net->mp_load)
	{
		f.WriteCasted<byte>(state);
		f << time;
	}
}

//=================================================================================================
bool Trap::Read(BitStreamReader& f)
{
	TRAP_TYPE type;
	f >> id;
	f.ReadCasted<byte>(type);
	f.ReadCasted<byte>(dir);
	f >> tile;
	f >> pos;
	f >> obj.rot.y;
	if(!f)
		return false;
	base = &BaseTrap::traps[type];

	state = 0;
	obj.base = nullptr;
	obj.mesh = base->mesh;
	obj.pos = pos;
	obj.scale = 1.f;
	obj.rot.x = obj.rot.z = 0;
	trigger = false;
	hitted = nullptr;

	if(net->mp_load)
	{
		f.ReadCasted<byte>(state);
		f >> time;
		if(!f)
			return false;
	}

	if(type == TRAP_ARROW || type == TRAP_POISON)
		obj.rot = Vec3(0, 0, 0);
	else if(type == TRAP_SPEAR)
	{
		obj2.base = nullptr;
		obj2.mesh = base->mesh2;
		obj2.pos = obj.pos;
		obj2.rot = obj.rot;
		obj2.scale = 1.f;
		obj2.pos.y -= 2.f;
		hitted = nullptr;
	}

	Register();
	return true;
}
