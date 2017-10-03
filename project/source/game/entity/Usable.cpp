#include "Pch.h"
#include "Core.h"
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
void Usable::Save(HANDLE file, bool local)
{
	WriteString1(file, base->id);
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);
	if(base->variants)
		WriteFile(file, &variant, sizeof(variant), &tmp, nullptr);
	if(IS_SET(base->use_flags, BaseUsable::CONTAINER))
		container->Save(file);

	if(local)
	{
		int refid = (user ? user->refid : -1);
		WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	}
}

//=================================================================================================
void Usable::Load(HANDLE file, bool local)
{
	if(LOAD_VERSION < V_FEATURE)
	{
		int type;
		ReadFile(file, &type, sizeof(type), &tmp, nullptr);
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
	{
		ReadString1(file);
		base = BaseUsable::Get(BUF);
	}
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_2_20 && base->variants)
		ReadFile(file, &variant, sizeof(variant), &tmp, nullptr);
	else
		variant = -1;
	if(LOAD_VERSION >= V_0_6 && IS_SET(base->use_flags, BaseUsable::CONTAINER))
	{
		container = new ItemContainer;
		if(LOAD_VERSION >= V_0_6)
			container->Load(file);
		else
		{
			auto item = Game::Get().GetRandomBook();
			if(item)
				container->items.push_back({ item, 1 ,1 });
		}
	}

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
	WriteString1(stream, base->id);
	stream.Write(pos);
	stream.Write(rot);
	stream.WriteCasted<byte>(variant);
}

//=================================================================================================
bool Usable::Read(BitStream& stream)
{
	if(!stream.Read(netid)
		|| !ReadString1(stream)
		|| !stream.Read(pos)
		|| !stream.Read(rot)
		|| !stream.ReadCasted<byte>(variant))
		return false;
	user = nullptr;
	base = BaseUsable::TryGet(BUF);
	if(!base)
	{
		Error("Invalid usable type '%s'.", BUF);
		return false;
	}
	if(IS_SET(base->use_flags, BaseUsable::CONTAINER))
		container = new ItemContainer;
	return true;
}
