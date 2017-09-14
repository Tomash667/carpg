#include "Pch.h"
#include "Core.h"
#include "Usable.h"
#include "Unit.h"
#include "Object.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

//=================================================================================================
Mesh* Usable::GetMesh() const
{
	BaseObject* base_obj = GetBaseObject();
	if(IS_SET(base_obj->flags2, OBJ2_VARIANT))
	{
		assert(InRange(variant, 0, (int)base_obj->variant->count));
		return base_obj->variant->entries[variant].mesh;
	}
	else
		return base_obj->mesh;
}

//=================================================================================================
void Usable::Save(HANDLE file, bool local)
{
	WriteFile(file, &type, sizeof(type), &tmp, nullptr);
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);
	BaseObject* base_obj = GetBaseObject();
	if(IS_SET(base_obj->flags2, OBJ2_VARIANT))
		WriteFile(file, &variant, sizeof(variant), &tmp, nullptr);

	if(local)
	{
		int refid = (user ? user->refid : -1);
		WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	}
}

//=================================================================================================
void Usable::Load(HANDLE file, bool local)
{
	ReadFile(file, &type, sizeof(type), &tmp, nullptr);
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_2_20 && IS_SET(GetBaseObject()->flags2, OBJ2_VARIANT))
		ReadFile(file, &variant, sizeof(variant), &tmp, nullptr);
	else
		variant = -1;

	if(local)
	{
		int refid;
		ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
		user = Unit::GetByRefid(refid);
	}
	else
		user = nullptr;
}

//=================================================================================================
void Usable::Write(BitStream& stream) const
{
	stream.Write(netid);
	stream.Write(pos);
	stream.Write(rot);
	stream.WriteCasted<byte>(type);
	stream.WriteCasted<byte>(variant);
}

//=================================================================================================
bool Usable::Read(BitStream& stream)
{
	if(!stream.Read(netid)
		|| !stream.Read(pos)
		|| !stream.Read(rot)
		|| !stream.ReadCasted<byte>(type)
		|| !stream.ReadCasted<byte>(variant))
		return false;
	user = nullptr;
	if(type >= U_MAX)
	{
		Error("Invalid usable type %d.", type);
		return false;
	}
	return true;
}
