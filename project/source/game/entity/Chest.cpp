#include "Pch.h"
#include "GameCore.h"
#include "Chest.h"
#include "Game.h"
#include "BitStreamFunc.h"
#include "Inventory.h"
#include "GlobalGui.h"
#include "PlayerInfo.h"

int Chest::netid_counter;

//=================================================================================================
void Chest::Save(FileWriter& f, bool local)
{
	ItemContainer::Save(f);

	f << pos;
	f << rot;
	f << netid;

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
	ItemContainer::Load(f);

	f >> pos;
	f >> rot;
	f >> netid;
	user = nullptr;

	if(local)
	{
		mesh_inst = new MeshInstance(Game::Get().aChest);

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

	int refid = f.Read<int>();
	if(refid == -1)
		handler = nullptr;
	else
	{
		handler = (ChestEventHandler*)refid;
		Game::Get().load_chest_handler.push_back(this);
	}
}

//=================================================================================================
void Chest::Write(BitStreamWriter& f)
{
	f << pos;
	f << rot;
	f << netid;
}

//=================================================================================================
bool Chest::Read(BitStreamReader& f)
{
	f >> pos;
	f >> rot;
	f >> netid;
	if(!f)
		return false;
	mesh_inst = new MeshInstance(Game::Get().aChest);
	return true;
}

//=================================================================================================
bool Chest::AddItem(const Item* item, uint count, uint team_count, bool notify)
{
	bool stacked = ItemContainer::AddItem(item, count, team_count);

	if(user && user->IsPlayer())
	{
		if(user->player->is_local)
			Game::Get().gui->inventory->BuildTmpInventory(1);
		else if(notify)
		{
			NetChangePlayer& c = Add1(user->player->player_info->changes);
			c.type = NetChangePlayer::ADD_ITEMS_CHEST;
			c.item = item;
			c.id = netid;
			c.count = count;
			c.a = team_count;
		}
	}

	return stacked;
}
