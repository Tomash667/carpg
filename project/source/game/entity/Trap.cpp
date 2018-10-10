#include "Pch.h"
#include "GameCore.h"
#include "Trap.h"
#include "Game.h"
#include "BitStreamFunc.h"

int Trap::netid_counter;

//=================================================================================================
void Trap::Save(FileWriter& f, bool local)
{
	f << base->type;
	f << pos;
	f << netid;

	if(base->type == TRAP_ARROW || base->type == TRAP_POISON)
	{
		f << tile;
		f << dir;
	}
	else
		f << obj.rot.y;

	if(local && base->type != TRAP_FIREBALL)
	{
		f << state;
		f << time;

		if(base->type == TRAP_SPEAR)
		{
			f << obj2.pos.y;
			f << hitted->size();
			for(Unit* unit : *hitted)
				f << unit->refid;
		}
	}
}

//=================================================================================================
void Trap::Load(FileReader& f, bool local)
{
	TRAP_TYPE type;

	f >> type;
	f >> pos;
	f >> netid;

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
	else if(type == TRAP_FIREBALL)
		obj.base = &Game::Get().obj_alpha;

	if(local && base->type != TRAP_FIREBALL)
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
					unit = Unit::GetByRefid(f.Read<int>());
			}
		}
	}
}

//=================================================================================================
void Trap::Write(BitStreamWriter& f)
{
	f.WriteCasted<byte>(base->type);
	f.WriteCasted<byte>(dir);
	f << netid;
	f << tile;
	f << pos;
	f << obj.rot.y;

	if(N.mp_load)
	{
		f.WriteCasted<byte>(state);
		f << time;
	}
}

//=================================================================================================
bool Trap::Read(BitStreamReader& f)
{
	TRAP_TYPE type;
	f.ReadCasted<byte>(type);
	f.ReadCasted<byte>(dir);
	f >> netid;
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

	if(N.mp_load)
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
	else
		obj.base = &Game::Get().obj_alpha;

	return true;
}
