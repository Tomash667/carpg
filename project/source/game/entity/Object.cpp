#include "Pch.h"
#include "Core.h"
#include "Object.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "BitStreamFunc.h"

//=================================================================================================
void Object::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &scale, sizeof(scale), &tmp, nullptr);

	if(base)
		WriteString1(file, base->id);
	else
	{
		byte len = 0;
		WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		len = (byte)strlen(mesh->filename);
		WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		WriteFile(file, mesh->filename, len, &tmp, nullptr);
	}
}

//=================================================================================================
void Object::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &scale, sizeof(scale), &tmp, nullptr);

	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);

	if(len)
	{
		ReadFile(file, BUF, len, &tmp, nullptr);
		BUF[len] = 0;
		if(LOAD_VERSION >= V_0_2_20)
			base = BaseObject::Get(BUF);
		else
		{
			if(strcmp(BUF, "tombstone") == 0)
				base = BaseObject::Get("tombstone_1");
			else
				base = BaseObject::Get(BUF);
		}
		mesh = base->mesh;
	}
	else
	{
		base = nullptr;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		ReadFile(file, BUF, len, &tmp, nullptr);
		BUF[len] = 0;
		if(LOAD_VERSION >= V_0_3)
			mesh = ResourceManager::Get<Mesh>().GetLoaded(BUF);
		else
		{
			if(strcmp(BUF, "mur.qmsh") == 0 || strcmp(BUF, "mur2.qmsh") == 0 || strcmp(BUF, "brama.qmsh") == 0)
			{
				base = BaseObject::Get("to_remove");
				mesh = base->mesh;
			}
			else
				mesh = ResourceManager::Get<Mesh>().GetLoaded(BUF);
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
