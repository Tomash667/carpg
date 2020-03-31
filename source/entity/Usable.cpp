#include "Pch.h"
#include "Usable.h"
#include "Unit.h"
#include "Object.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "ItemContainer.h"
#include "Net.h"

const float Usable::SOUND_DIST = 1.5f;
EntityType<Usable>::Impl EntityType<Usable>::impl;

//=================================================================================================
void Usable::Save(FileWriter& f, bool local)
{
	f << id;
	f << base->id;
	f << pos;
	f << rot;
	if(base->variants)
		f << variant;
	if(IsSet(base->use_flags, BaseUsable::CONTAINER))
		container->Save(f);
	if(local && !IsSet(base->use_flags, BaseUsable::CONTAINER))
		f << user;
}

//=================================================================================================
void Usable::Load(FileReader& f, bool local)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	base = BaseUsable::Get(f.ReadString1());
	f >> pos;
	f >> rot;
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid
	if(base->variants)
		f >> variant;
	if(IsSet(base->use_flags, BaseUsable::CONTAINER))
	{
		container = new ItemContainer;
		container->Load(f);
	}

	if(local)
	{
		if(LOAD_VERSION >= V_0_7_1)
		{
			if(IsSet(base->use_flags, BaseUsable::CONTAINER))
				user = nullptr;
			else
				f >> user;
		}
		else
		{
			f >> user;
			if(IsSet(base->use_flags, BaseUsable::CONTAINER))
				user = nullptr;
		}
	}
	else
		user = nullptr;
}

//=================================================================================================
void Usable::Write(BitStreamWriter& f) const
{
	f << id;
	f << base->id;
	f << pos;
	f << rot;
	f.WriteCasted<byte>(variant);
	f << user;
}

//=================================================================================================
bool Usable::Read(BitStreamReader& f)
{
	f >> id;
	const string& base_id = f.ReadString1();
	f >> pos;
	f >> rot;
	f.ReadCasted<byte>(variant);
	f >> user;
	if(!f)
		return false;
	base = BaseUsable::TryGet(base_id);
	if(!base)
	{
		Error("Invalid usable type '%s'.", base_id.c_str());
		return false;
	}
	Register();
	if(IsSet(base->use_flags, BaseUsable::CONTAINER))
		container = new ItemContainer;
	return true;
}

//=================================================================================================
Mesh* Usable::GetMesh() const
{
	if(base->variants)
	{
		assert(InRange(variant, 0, (int)base->variants->meshes.size()));
		return base->variants->meshes[variant];
	}
	else
		return base->mesh;
}

//=================================================================================================
Vec3 Usable::GetCenter() const
{
	return pos + base->mesh->head.bbox.Midpoint();
}
