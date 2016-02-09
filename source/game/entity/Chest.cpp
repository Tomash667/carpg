// skrzynia
#include "Pch.h"
#include "Base.h"
#include "Chest.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
void Chest::Save(HANDLE file, bool local)
{
	while(!items.empty() && !items.back().item)
		items.pop_back();

	uint ile = items.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);

	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item)
		{
			WriteString1(file, it->item->id);
			WriteFile(file, &it->count, sizeof(it->count), &tmp, nullptr);
			WriteFile(file, &it->team_count, sizeof(it->team_count), &tmp, nullptr);
			if(it->item->id[0] == '$')
				WriteFile(file, &it->item->refid, sizeof(int), &tmp, nullptr);
		}
		else
		{
			byte zero = 0;
			WriteFile(file, &zero, sizeof(zero), &tmp, nullptr);
		}
	}

	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);

	if(local)
	{
		AnimeshInstance::Group& group = ani->groups[0];
		if(group.IsPlaying())
		{
			WriteFile(file, &group.state, sizeof(group.state), &tmp, nullptr);
			WriteFile(file, &group.time, sizeof(group.time), &tmp, nullptr);
			WriteFile(file, &group.blend_time, sizeof(group.blend_time), &tmp, nullptr);
		}
		else
		{
			int b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, nullptr);
		}
	}

	int chest_event_handler_quest_refid = (handler ? handler->GetChestEventHandlerQuestRefid() : -1);
	WriteFile(file, &chest_event_handler_quest_refid, sizeof(chest_event_handler_quest_refid), &tmp, nullptr);
}

//=================================================================================================
void Chest::Load(HANDLE file, bool local)
{
	bool can_sort = true;

	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	if(ile)
	{
		items.resize(ile);
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			byte len;
			ReadFile(file, &len, sizeof(len), &tmp, nullptr);
			if(len)
			{
				ReadFile(file, BUF, len, &tmp, nullptr);
				BUF[len] = 0;
				ReadFile(file, &it->count, sizeof(it->count), &tmp, nullptr);
				ReadFile(file, &it->team_count, sizeof(it->team_count), &tmp, nullptr);
				if(BUF[0] != '$')
					it->item = ::FindItem(BUF);
				else
				{
					int quest_refid;
					ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, nullptr);
					Game::Get().AddQuestItemRequest(&it->item, BUF, quest_refid, &items);
					it->item = QUEST_ITEM_PLACEHOLDER;
					can_sort = false;
				}
			}
			else
			{
				assert(LOAD_VERSION < V_0_2_10);
				it->item = nullptr;
				it->count = 0;
			}
		}
	}

	if(can_sort && LOAD_VERSION < V_0_2_20 && !items.empty())
	{
		if(LOAD_VERSION < V_0_2_10)
			RemoveNullItems(items);
		SortItems(items);
	}

	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);
	looted = false;

	if(local)
	{
		int b;
		ReadFile(file, &b, sizeof(b), &tmp, nullptr);

		ani = new AnimeshInstance(Game::Get().aSkrzynia);

		if(b != 0)
		{
			AnimeshInstance::Group& group = ani->groups[0];
			group.anim = &ani->ani->anims[0];
			group.state = b;
			ReadFile(file, &group.time, sizeof(group.time), &tmp, nullptr);
			ReadFile(file, &group.blend_time, sizeof(group.blend_time), &tmp, nullptr);
			group.used_group = 0;
		}
	}
	else
		ani = nullptr;

	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	if(refid == -1)
		handler = nullptr;
	else
	{
		handler = (ChestEventHandler*)refid;
		Game::Get().load_chest_handler.push_back(this);
	}
}
