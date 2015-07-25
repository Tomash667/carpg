#include "Pch.h"
#include "Base.h"
#include "Game.h"

//=================================================================================================
void Trap::Save(HANDLE file, bool local)
{
	WriteFile(file, &base->type, sizeof(base->type), &tmp, NULL);
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &netid, sizeof(netid), &tmp, NULL);

	if(base->type == TRAP_ARROW || base->type == TRAP_POISON)
	{
		WriteFile(file, &tile, sizeof(tile), &tmp, NULL);
		WriteFile(file, &dir, sizeof(dir), &tmp, NULL);
	}
	else
		WriteFile(file, &obj.rot.y, sizeof(obj.rot.y), &tmp, NULL);

	if(local && base->type != TRAP_FIREBALL)
	{
		WriteFile(file, &state, sizeof(state), &tmp, NULL);
		WriteFile(file, &time, sizeof(time), &tmp, NULL);

		if(base->type == TRAP_SPEAR)
		{
			WriteFile(file, &obj2.pos.y, sizeof(obj2.pos.y), &tmp, NULL);
			uint count = hitted->size();
			WriteFile(file, &count, sizeof(count), &tmp, NULL);
			if(count)
			{
				for(vector<Unit*>::iterator it = hitted->begin(), end = hitted->end(); it != end; ++it)
					WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, NULL);
			}
		}
	}
}

//=================================================================================================
void Trap::Load(HANDLE file, bool local)
{
	TRAP_TYPE type;

	ReadFile(file, &type, sizeof(type), &tmp, NULL);
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &netid, sizeof(netid), &tmp, NULL);

	base = &g_traps[type];
	hitted = NULL;
	obj.pos = pos;
	obj.rot = VEC3(0,0,0);
	obj.scale = 1.f;
	obj.base = NULL;
	obj.mesh = base->mesh;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		ReadFile(file, &tile, sizeof(tile), &tmp, NULL);
		ReadFile(file, &dir, sizeof(dir), &tmp, NULL);
	}
	else
		ReadFile(file, &obj.rot.y, sizeof(obj.rot.y), &tmp, NULL);

	if(type == TRAP_SPEAR)
	{
		obj2.pos = pos;
		obj2.rot = obj.rot;
		obj2.scale = 1.f;
		obj2.mesh = base->mesh;
		obj2.base = NULL;
	}
	else if(type == TRAP_FIREBALL)
		obj.base = &Game::Get().obj_alpha;

	if(local && base->type != TRAP_FIREBALL)
	{
		ReadFile(file, &state, sizeof(state), &tmp, NULL);
		ReadFile(file, &time, sizeof(time), &tmp, NULL);

		if(base->type == TRAP_SPEAR)
		{
			ReadFile(file, &obj2.pos.y, sizeof(obj2.pos.y), &tmp, NULL);
			uint count;
			ReadFile(file, &count, sizeof(count), &tmp, NULL);
			hitted = new vector<Unit*>;
			if(count)
			{
				hitted->resize(count);
				for(vector<Unit*>::iterator it = hitted->begin(), end = hitted->end(); it != end; ++it)
				{
					int refid;
					ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
					*it = Unit::GetByRefid(refid);
				}
			}
		}
	}
}
