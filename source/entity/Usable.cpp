#include "Pch.h"
#include "Usable.h"

#include "BitStreamFunc.h"
#include "GameFile.h"
#include "ItemContainer.h"
#include "Net.h"
#include "Object.h"
#include "SaveState.h"
#include "Unit.h"

const float Usable::SOUND_DIST = 1.5f;
EntityType<Usable>::Impl EntityType<Usable>::impl;

//=================================================================================================
void Usable::Save(GameWriter& f)
{
	f << id;
	f << base->hash;
	f << pos;
	f << rot;
	if(base->variants)
		f << variant;
	if(IsSet(base->use_flags, BaseUsable::CONTAINER))
		container->Save(f);
	if(f.isLocal && !IsSet(base->use_flags, BaseUsable::CONTAINER))
		f << user;
}

//=================================================================================================
void Usable::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	if(LOAD_VERSION >= V_0_16)
		base = BaseUsable::Get(f.Read<int>());
	else
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

	if(f.isLocal)
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
	f << base->hash;
	f << pos;
	f << rot;
	f.WriteCasted<byte>(variant);
	f << user;
}

//=================================================================================================
bool Usable::Read(BitStreamReader& f)
{
	int hash;
	f >> id;
	f >> hash;
	f >> pos;
	f >> rot;
	f.ReadCasted<byte>(variant);
	f >> user;
	if(!f)
		return false;
	base = BaseUsable::Get(hash);
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
