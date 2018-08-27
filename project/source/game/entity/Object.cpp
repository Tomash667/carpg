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
void Object::Write(BitStream& stream) const
{
	stream.Write(pos);
	stream.Write(rot);
	stream.Write(scale);
	if(base)
		WriteString1(stream, base->id);
	else
	{
		stream.Write<byte>(0);
		WriteString1(stream, mesh->filename);
	}
}

//=================================================================================================
bool Object::Read(BitStream& stream)
{
	if(!stream.Read(pos)
		|| !stream.Read(rot)
		|| !stream.Read(scale)
		|| !ReadString1(stream))
		return false;
	if(BUF[0])
	{
		// use base obj
		base = BaseObject::Get(BUF);
		if(!base)
		{
			Error("Missing base object '%s'!", BUF);
			return false;
		}
		mesh = base->mesh;
	}
	else
	{
		// use mesh
		if(!ReadString1(stream))
			return false;
		mesh = ResourceManager::Get<Mesh>().GetLoaded(BUF);
		base = nullptr;
	}
	return true;
}
