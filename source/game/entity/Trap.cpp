#include "Pch.h"
#include "Base.h"
#include "Game.h"

//=================================================================================================
void Trap::Save(HANDLE file, bool local)
{
	WriteFile(file, &base->type, sizeof(base->type), &tmp, nullptr);
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);

	if(base->type == TRAP_ARROW || base->type == TRAP_POISON)
	{
		WriteFile(file, &tile, sizeof(tile), &tmp, nullptr);
		WriteFile(file, &dir, sizeof(dir), &tmp, nullptr);
	}
	else
		WriteFile(file, &obj.rot.y, sizeof(obj.rot.y), &tmp, nullptr);

	if(local && base->type != TRAP_FIREBALL)
	{
		WriteFile(file, &state, sizeof(state), &tmp, nullptr);
		WriteFile(file, &time, sizeof(time), &tmp, nullptr);

		if(base->type == TRAP_SPEAR)
		{
			WriteFile(file, &obj2.pos.y, sizeof(obj2.pos.y), &tmp, nullptr);
			uint count = hitted->size();
			WriteFile(file, &count, sizeof(count), &tmp, nullptr);
			if(count)
			{
				for(vector<Unit*>::iterator it = hitted->begin(), end = hitted->end(); it != end; ++it)
					WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, nullptr);
			}
		}
	}
}

//=================================================================================================
void Trap::Load(HANDLE file, bool local)
{
	TRAP_TYPE type;

	ReadFile(file, &type, sizeof(type), &tmp, nullptr);
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);

	base = &g_traps[type];
	hitted = nullptr;
	obj.pos = pos;
	obj.rot = VEC3(0,0,0);
	obj.scale = 1.f;
	obj.base = nullptr;
	obj.mesh = base->mesh;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		ReadFile(file, &tile, sizeof(tile), &tmp, nullptr);
		ReadFile(file, &dir, sizeof(dir), &tmp, nullptr);
	}
	else
		ReadFile(file, &obj.rot.y, sizeof(obj.rot.y), &tmp, nullptr);

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
		ReadFile(file, &state, sizeof(state), &tmp, nullptr);
		ReadFile(file, &time, sizeof(time), &tmp, nullptr);

		if(base->type == TRAP_SPEAR)
		{
			ReadFile(file, &obj2.pos.y, sizeof(obj2.pos.y), &tmp, nullptr);
			uint count;
			ReadFile(file, &count, sizeof(count), &tmp, nullptr);
			hitted = new vector<Unit*>;
			if(count)
			{
				hitted->resize(count);
				for(vector<Unit*>::iterator it = hitted->begin(), end = hitted->end(); it != end; ++it)
				{
					int refid;
					ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
					*it = Unit::GetByRefid(refid);
				}
			}
		}
	}
}
