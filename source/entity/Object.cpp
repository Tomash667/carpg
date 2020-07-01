#include "Pch.h"
#include "Object.h"

#include "BitStreamFunc.h"
#include "Game.h"
#include "GameFile.h"
#include "ResourceManager.h"
#include "SaveState.h"

#include <MeshInstance.h>

//=================================================================================================
Object::~Object()
{
	delete meshInst;
}

//=================================================================================================
void Object::Save(GameWriter& f)
{
	f << pos;
	f << rot;
	f << scale;

	if(base)
		f << base->hash;
	else
	{
		f << 0;
		f << mesh->filename;
	}
	if(meshInst)
		f << meshInst->groups[0].time;
}

//=================================================================================================
void Object::Load(GameReader& f)
{
	f >> pos;
	f >> rot;
	f >> scale;

	if(LOAD_VERSION >= V_0_16)
	{
		const int hash = f.Read<int>();
		if(hash != 0)
		{
			base = BaseObject::Get(hash);
			mesh = base->mesh;
		}
		else
		{
			base = nullptr;
			mesh = res_mgr->Load<Mesh>(f.ReadString1());
		}
	}
	else
	{
		const string& base_id = f.ReadString1();
		if(!base_id.empty())
		{
			base = BaseObject::Get(base_id);
			mesh = base->mesh;
		}
		else
		{
			base = nullptr;
			mesh = res_mgr->Load<Mesh>(f.ReadString1());
		}
	}

	if(f.isLocal && mesh->IsAnimated())
		f >> time;
}

//=================================================================================================
void Object::Write(BitStreamWriter& f) const
{
	f << pos;
	f << rot;
	f << scale;
	if(base)
		f << base->hash;
	else
	{
		f << 0;
		f << mesh->filename;
	}
	if(meshInst && net->mp_load)
		f << meshInst->groups[0].time;
}

//=================================================================================================
bool Object::Read(BitStreamReader& f)
{
	f >> pos;
	f >> rot;
	f >> scale;

	const int hash = f.Read<int>();
	if(hash != 0)
	{
		base = BaseObject::Get(hash);
		mesh = base->mesh;
	}
	else
	{
		const string& mesh_id = f.ReadString1();
		if(!f)
			return false;
		mesh = res_mgr->Load<Mesh>(mesh_id);
		base = nullptr;
	}

	if(mesh->IsAnimated() && net->mp_load)
		f >> time;
	return true;
}
