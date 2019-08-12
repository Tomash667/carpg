#include "Pch.h"
#include "GameCore.h"
#include "Usable.h"
#include "Unit.h"
#include "Object.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "ItemContainer.h"
#include "Net.h"

// pre V_0_6_2 compatibility
namespace old
{
	enum USABLE_ID
	{
		U_CHAIR,
		U_BENCH,
		U_ANVIL,
		U_CAULDRON,
		U_IRON_VEIN,
		U_GOLD_VEIN,
		U_THRONE,
		U_STOOL,
		U_BENCH_DIR,
		U_BOOKSHELF,
		U_MAX
	};
}

vector<Usable*> Usable::refid_table;
vector<UsableRequest> Usable::refid_request;
int Usable::netid_counter;
const float Usable::SOUND_DIST = 1.5f;

//=================================================================================================
Mesh* Usable::GetMesh() const
{
	if(base->variants)
	{
		assert(InRange(variant, 0, (int)base->variants->entries.size()));
		return base->variants->entries[variant].mesh;
	}
	else
		return base->mesh;
}

//=================================================================================================
void Usable::Save(FileWriter& f, bool local)
{
	f << base->id;
	f << pos;
	f << rot;
	f << netid;
	if(base->variants)
		f << variant;
	if(IsSet(base->use_flags, BaseUsable::CONTAINER))
		container->Save(f);
	if(local && !IsSet(base->use_flags, BaseUsable::CONTAINER))
		f << (user ? user->refid : -1);
}

//=================================================================================================
void Usable::Load(FileReader& f, bool local)
{
	if(LOAD_VERSION < V_0_6_2)
	{
		int type = f.Read<int>();
		cstring id;
		switch(type)
		{
		default:
		case old::U_CHAIR:
			id = "chair";
			break;
		case old::U_BENCH:
			id = "bench";
			break;
		case old::U_ANVIL:
			id = "anvil";
			break;
		case old::U_CAULDRON:
			id = "cauldron";
			break;
		case old::U_IRON_VEIN:
			id = "iron_vein";
			break;
		case old::U_GOLD_VEIN:
			id = "gold_vein";
			break;
		case old::U_THRONE:
			id = "throne";
			break;
		case old::U_STOOL:
			id = "stool";
			break;
		case old::U_BENCH_DIR:
			id = "bench_dir";
			break;
		case old::U_BOOKSHELF:
			id = "bookshelf";
			break;
		}
		base = BaseUsable::Get(id);
	}
	else
		base = BaseUsable::Get(f.ReadString1());
	f >> pos;
	f >> rot;
	f >> netid;
	if(base->variants)
		f >> variant;
	if(LOAD_VERSION >= V_0_6 && IsSet(base->use_flags, BaseUsable::CONTAINER))
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
				user = Unit::GetByRefid(f.Read<int>());
		}
		else
		{
			int refid = f.Read<int>();
			if(IsSet(base->use_flags, BaseUsable::CONTAINER))
				user = nullptr;
			else
				user = Unit::GetByRefid(refid);
		}
	}
	else
		user = nullptr;
}

//=================================================================================================
void Usable::Write(BitStreamWriter& f) const
{
	f << netid;
	f << base->id;
	f << pos;
	f << rot;
	f.WriteCasted<byte>(variant);
}

//=================================================================================================
bool Usable::Read(BitStreamReader& f)
{
	f >> netid;
	const string& base_id = f.ReadString1();
	f >> pos;
	f >> rot;
	f.ReadCasted<byte>(variant);
	if(!f)
		return false;
	user = nullptr;
	base = BaseUsable::TryGet(base_id);
	if(!base)
	{
		Error("Invalid usable type '%s'.", base_id.c_str());
		return false;
	}
	if(IsSet(base->use_flags, BaseUsable::CONTAINER))
		container = new ItemContainer;
	return true;
}
