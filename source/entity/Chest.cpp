#include "Pch.h"
#include "GameCore.h"
#include "Chest.h"
#include "Game.h"
#include "BitStreamFunc.h"
#include "Inventory.h"
#include "GameGui.h"
#include "PlayerInfo.h"
#include "SoundManager.h"
#include "SaveState.h"
#include "GameResources.h"

const float Chest::SOUND_DIST = 1.f;
EntityType<Chest>::Impl EntityType<Chest>::impl;

//=================================================================================================
void Chest::Save(FileWriter& f, bool local)
{
	f << id;

	ItemContainer::Save(f);

	f << pos;
	f << rot;

	if(local)
	{
		MeshInstance::Group& group = mesh_inst->groups[0];
		if(group.IsPlaying())
		{
			f << group.state;
			f << group.time;
			f << group.blend_time;
		}
		else
			f << 0;
	}

	f << (handler ? handler->GetChestEventHandlerQuestRefid() : -1);
}

//=================================================================================================
void Chest::Load(FileReader& f, bool local)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();

	ItemContainer::Load(f);

	f >> pos;
	f >> rot;
	user = nullptr;

	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid

	if(local)
	{
		mesh_inst = new MeshInstance(game_res->aChest);

		int state = f.Read<int>();
		if(state != 0)
		{
			MeshInstance::Group& group = mesh_inst->groups[0];
			group.anim = &mesh_inst->mesh->anims[0];
			group.state = state;
			group.used_group = 0;
			f >> group.time;
			f >> group.blend_time;
		}
	}
	else
		mesh_inst = nullptr;

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
	f << pos;
	f << rot;
}

//=================================================================================================
bool Chest::Read(BitStreamReader& f)
{
	f >> id;
	f >> pos;
	f >> rot;
	if(!f)
		return false;
	Register();
	mesh_inst = new MeshInstance(game_res->aChest);
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
		mesh_inst->Play(&mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END, 0);
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
		mesh_inst->Play(&mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
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
