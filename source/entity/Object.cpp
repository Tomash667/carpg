#include "Pch.h"
#include "Object.h"

#include "BitStreamFunc.h"
#include "ResourceManager.h"
#include "SaveState.h"

//=================================================================================================
void Object::Save(FileWriter& f)
{
	f << pos;
	f << rot;
	f << scale;

	if(base)
		f << base->id;
	else
	{
		f.Write0();
		f << mesh->filename;
	}
}

//=================================================================================================
void Object::Load(FileReader& f)
{
	f >> pos;
	f >> rot;
	f >> scale;

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

//=================================================================================================
void Object::Write(BitStreamWriter& f) const
{
	f << pos;
	f << rot;
	f << scale;
	if(base)
		f << base->id;
	else
	{
		f.Write0();
		f << mesh->filename;
	}
}

//=================================================================================================
bool Object::Read(BitStreamReader& f)
{
	f >> pos;
	f >> rot;
	f >> scale;
	const string& base_id = f.ReadString1();
	if(!f)
		return false;
	if(!base_id.empty())
	{
		// use base obj
		base = BaseObject::Get(base_id);
		if(!base)
		{
			Error("Missing base object '%s'!", base_id.c_str());
			return false;
		}
		mesh = base->mesh;
	}
	else
	{
		// use mesh
		const string& mesh_id = f.ReadString1();
		if(!f)
			return false;
		mesh = res_mgr->Load<Mesh>(mesh_id);
		base = nullptr;
	}
	return true;
}
