#include "Pch.h"
#include "Base.h"
#include "Useable.h"
#include "Unit.h"
#include "Object.h"
#include "SaveState.h"

//=================================================================================================
Animesh* Useable::GetMesh() const
{
	Obj* base_obj = GetBaseObj();
	if(IS_SET(base_obj->flagi2, OBJ2_VARIANT))
	{
		assert(in_range(variant, 0, (int)base_obj->variant->count));
		return base_obj->variant->entries[variant].mesh;
	}
	else
		return base_obj->ani;
}

//=================================================================================================
void Useable::Save(HANDLE file, bool local)
{
	WriteFile(file, &type, sizeof(type), &tmp, NULL);
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &rot, sizeof(rot), &tmp, NULL);
	WriteFile(file, &netid, sizeof(netid), &tmp, NULL);
	Obj* base_obj = GetBaseObj();
	if(IS_SET(base_obj->flagi2, OBJ2_VARIANT))
		WriteFile(file, &variant, sizeof(variant), &tmp, NULL);

	if(local)
	{
		int refid = (user ? user->refid : -1);
		WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	}
}

//=================================================================================================
void Useable::Load(HANDLE file, bool local)
{
	ReadFile(file, &type, sizeof(type), &tmp, NULL);
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &rot, sizeof(rot), &tmp, NULL);
	ReadFile(file, &netid, sizeof(netid), &tmp, NULL);
	if(LOAD_VERSION >= V_0_2_20 && IS_SET(GetBaseObj()->flagi2, OBJ2_VARIANT))
		ReadFile(file, &variant, sizeof(variant), &tmp, NULL);
	else
		variant = -1;

	if(local)
	{
		int refid;
		ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
		user = Unit::GetByRefid(refid);
	}
	else
		user = NULL;
}
