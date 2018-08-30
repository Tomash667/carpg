#include "Pch.h"
#include "GameCore.h"
#include "Trap.h"
#include "Game.h"

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
