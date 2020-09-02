#include "Pch.h"
#include "Object.h"

#include "BitStreamFunc.h"
#include "Game.h"
#include "SceneNodeHelper.h"

#include <ResourceManager.h>

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
	if(f.isLocal)
		SceneNodeHelper::Save(node, f);
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
			mesh = res_mgr->Get<Mesh>(f.ReadString1());
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
			mesh = res_mgr->Get<Mesh>(f.ReadString1());
		}
	}

	if(f.isLocal)
	{
		if(LOAD_VERSION >= V_DEV)
			node = SceneNodeHelper::Load(f);
		else if(mesh->IsAnimated())
			f.Skip<float>(); // old time
	}
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
	if(net->mp_load)
		SceneNodeHelper::Save(node, f);
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
		mesh = res_mgr->Get<Mesh>(mesh_id);
		base = nullptr;
	}

	if(net->mp_load)
		node = SceneNodeHelper::Load(f);
	return true;
}
