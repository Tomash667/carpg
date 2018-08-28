#include "Pch.h"
#include "GameCore.h"
#include "Object.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "BitStreamFunc.h"

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
		if(LOAD_VERSION >= V_0_2_20)
			base = BaseObject::Get(base_id);
		else
		{
			if(base_id == "tombstone")
				base = BaseObject::Get("tombstone_1");
			else
				base = BaseObject::Get(base_id);
		}
		mesh = base->mesh;
	}
	else
	{
		base = nullptr;
		const string& mesh_id = f.ReadString1();
		if(LOAD_VERSION >= V_0_3)
			mesh = ResourceManager::Get<Mesh>().GetLoaded(mesh_id);
		else
		{
			if(mesh_id == "mur.qmsh" || mesh_id == "mur2.qmsh" || mesh_id == "brama.qmsh")
			{
				base = BaseObject::Get("to_remove");
				mesh = base->mesh;
			}
			else
				mesh = ResourceManager::Get<Mesh>().GetLoaded(mesh_id);
		}
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
		mesh = ResourceManager::Get<Mesh>().GetLoaded(mesh_id);
		base = nullptr;
	}
	return true;
}
