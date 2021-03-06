#include "Pch.h"
#include "Chest.h"

#include "BitStreamFunc.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "Inventory.h"
#include "PlayerInfo.h"

#include <SoundManager.h>

const float Chest::SOUND_DIST = 1.f;
EntityType<Chest>::Impl EntityType<Chest>::impl;

//=================================================================================================
void Chest::Recreate()
{
	if(meshInst)
	{
		meshInst->ApplyPreload(base->mesh);
		if(meshInst->IsActive())
			meshInst->Play(&meshInst->mesh->anims[0], PLAY_PRIO1 | PLAY_BACK | PLAY_ONCE);
	}
	else
		meshInst = new MeshInstance(base->mesh);
}

//=================================================================================================
void Chest::Cleanup()
{
	delete meshInst;
	meshInst = nullptr;
	user = nullptr;
}

//=================================================================================================
void Chest::Save(GameWriter& f)
{
	f << id;

	ItemContainer::Save(f);

	f << base->hash;
	f << pos;
	f << rot;

	if(f.isLocal)
		MeshInstance::SaveOptional(f, meshInst);

	f << (handler ? handler->GetChestEventHandlerQuestId() : -1);
}

//=================================================================================================
void Chest::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();

	ItemContainer::Load(f);

	if(LOAD_VERSION >= V_0_16)
		base = BaseObject::Get(f.Read<int>());
	else
	{
		static BaseObject* baseChest = BaseObject::Get("chest");
		base = baseChest;
	}
	f >> pos;
	f >> rot;
	user = nullptr;

	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid

	if(f.isLocal)
	{
		if(LOAD_VERSION >= V_0_18)
			MeshInstance::LoadOptional(f, meshInst);
		else
		{
			meshInst = new MeshInstance(nullptr);
			meshInst->LoadSimple(f);
		}
	}
	else
		meshInst = nullptr;

	int handler_id = f.Read<int>();
	if(handler_id == -1)
		handler = nullptr;
	else
	{
		handler = reinterpret_cast<ChestEventHandler*>(handler_id);
		game->load_chest_handler.push_back(this);
	}
}

//=================================================================================================
void Chest::Write(BitStreamWriter& f)
{
	f << id;
	f << base->hash;
	f << pos;
	f << rot;
	if(net->mp_load)
		MeshInstance::SaveOptional(f, meshInst);
}

//=================================================================================================
bool Chest::Read(BitStreamReader& f)
{
	f >> id;
	base = BaseObject::Get(f.Read<int>());
	f >> pos;
	f >> rot;
	if(net->mp_load)
		MeshInstance::LoadOptional(f, meshInst);
	if(!f)
		return false;
	Register();
	return true;
}

//=================================================================================================
bool Chest::AddItem(const Item* item, uint count, uint team_count, bool notify)
{
	bool stacked = ItemContainer::AddItem(item, count, team_count);

	if(user && user->IsPlayer())
	{
		if(user->player->is_local)
			game_gui->inventory->BuildTmpInventory(1);
		else if(notify)
		{
			NetChangePlayer& c = Add1(user->player->player_info->changes);
			c.type = NetChangePlayer::ADD_ITEMS_CHEST;
			c.item = item;
			c.id = id;
			c.count = count;
			c.a = team_count;
		}
	}

	return stacked;
}

//=================================================================================================
void Chest::OpenClose(Unit* unit)
{
	if(unit)
	{
		// open chest by unit
		assert(!user);
		user = unit;
		meshInst->Play(&meshInst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END);
		sound_mgr->PlaySound3d(game_res->sChestOpen, GetCenter(), SOUND_DIST);
		if(Net::IsLocal() && handler)
			handler->HandleChestEvent(ChestEventHandler::Opened, this);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::USE_CHEST;
			c.id = id;
			c.count = unit->id;
		}
	}
	else
	{
		// close chest
		assert(user);
		user = nullptr;
		meshInst->Play(&meshInst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_BACK);
		sound_mgr->PlaySound3d(game_res->sChestClose, GetCenter(), SOUND_DIST);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::USE_CHEST;
			c.id = id;
			c.count = -1;
		}
	}
}
