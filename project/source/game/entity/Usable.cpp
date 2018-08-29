#include "Pch.h"
#include "GameCore.h"
#include "Usable.h"
#include "Unit.h"
#include "Object.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "ItemContainer.h"
#include "Game.h"

enum OLD_USABLE_ID
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

int Usable::netid_counter;

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
	if(IS_SET(base->use_flags, BaseUsable::CONTAINER))
		container->Save(f);
	if(local && !IS_SET(base->use_flags, BaseUsable::CONTAINER))
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
		case U_CHAIR:
			id = "chair";
			break;
		case U_BENCH:
			id = "bench";
			break;
		case U_ANVIL:
			id = "anvil";
			break;
		case U_CAULDRON:
			id = "cauldron";
			break;
		case U_IRON_VEIN:
			id = "iron_vein";
			break;
		case U_GOLD_VEIN:
			id = "gold_vein";
			break;
		case U_THRONE:
			id = "throne";
			break;
		case U_STOOL:
			id = "stool";
			break;
		case U_BENCH_DIR:
			id = "bench_dir";
			break;
		case U_BOOKSHELF:
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
	if(LOAD_VERSION >= V_0_2_20 && base->variants)
		f >> variant;
	else
		variant = -1;
	if(LOAD_VERSION >= V_0_6 && IS_SET(base->use_flags, BaseUsable::CONTAINER))
	{
		container = new ItemContainer;
		if(LOAD_VERSION >= V_0_6)
			container->Load(f);
		else
		{
			auto item = Game::Get().GetRandomBook();
			if(item)
				container->items.push_back({ item, 1 ,1 });
		}
	}

	if(local)
	{
		if(LOAD_VERSION >= V_0_7_1)
		{
			if(IS_SET(base->use_flags, BaseUsable::CONTAINER))
				user = nullptr;
			else
				user = Unit::GetByRefid(f.Read<int>());
		}
		else
		{
			int refid = f.Read<int>();
			if(IS_SET(base->use_flags, BaseUsable::CONTAINER))
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
	if(IS_SET(base->use_flags, BaseUsable::CONTAINER))
		container = new ItemContainer;
	return true;
}
