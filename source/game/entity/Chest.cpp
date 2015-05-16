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
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);

	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item)
		{
			byte len = (byte)strlen(it->item->id);
			WriteFile(file, &len, sizeof(len), &tmp, NULL);
			WriteFile(file, it->item->id, len, &tmp, NULL);
			WriteFile(file, &it->count, sizeof(it->count), &tmp, NULL);
			WriteFile(file, &it->team_count, sizeof(it->team_count), &tmp, NULL);
			if(it->item->id[0] == '$')
				WriteFile(file, &it->item->refid, sizeof(int), &tmp, NULL);
		}
		else
		{
			byte zero = 0;
			WriteFile(file, &zero, sizeof(zero), &tmp, NULL);
		}
	}

	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &rot, sizeof(rot), &tmp, NULL);
	WriteFile(file, &netid, sizeof(netid), &tmp, NULL);

	if(local)
	{
		if(ani->groups[0].IsPlaying())
		{
			WriteFile(file, &ani->groups[0].state, sizeof(ani->groups[0].state), &tmp, NULL);
			WriteFile(file, &ani->groups[0].time, sizeof(ani->groups[0].time), &tmp, NULL);
			WriteFile(file, &ani->groups[0].blend_time, sizeof(ani->groups[0].blend_time), &tmp, NULL);
		}
		else
		{
			int b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, NULL);
		}
	}

	int chest_event_handler_quest_refid = (handler ? handler->GetChestEventHandlerQuestRefid() : -1);
	WriteFile(file, &chest_event_handler_quest_refid, sizeof(chest_event_handler_quest_refid), &tmp, NULL);
}

//=================================================================================================
void Chest::Load(HANDLE file, bool local)
{
	bool can_sort = true;

	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	if(ile)
	{
		items.resize(ile);
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			byte len;
			ReadFile(file, &len, sizeof(len), &tmp, NULL);
			if(len)
			{
				ReadFile(file, BUF, len, &tmp, NULL);
				BUF[len] = 0;
				ReadFile(file, &it->count, sizeof(it->count), &tmp, NULL);
				ReadFile(file, &it->team_count, sizeof(it->team_count), &tmp, NULL);
				if(BUF[0] != '$')
				{
					if(strcmp(BUF, "gold") == 0)
						it->item = &Game::Get().gold_item;
					else
						it->item = ::FindItem(BUF);
				}
				else
				{
					int quest_refid;
					ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, NULL);
					Game::Get().AddQuestItemRequest(&it->item, BUF, quest_refid, &items);
					it->item = QUEST_ITEM_PLACEHOLDER;
					can_sort = false;
				}
			}
			else
			{
				assert(LOAD_VERSION < V_0_2_10);
				it->item = NULL;
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

	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &rot, sizeof(rot), &tmp, NULL);
	ReadFile(file, &netid, sizeof(netid), &tmp, NULL);
	looted = false;

	if(local)
	{
		int b;
		ReadFile(file, &b, sizeof(b), &tmp, NULL);

		ani = new AnimeshInstance(Game::Get().aSkrzynia);

		if(b != 0)
		{
			ani->groups[0].anim = &ani->ani->anims[0];
			ani->groups[0].state = b;
			ReadFile(file, &ani->groups[0].time, sizeof(ani->groups[0].time), &tmp, NULL);
			ReadFile(file, &ani->groups[0].blend_time, sizeof(ani->groups[0].blend_time), &tmp, NULL);
			ani->groups[0].used_group = 0;
		}
	}
	else
		ani = NULL;

	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	if(refid == -1)
		handler = NULL;
	else
	{
		handler = (ChestEventHandler*)refid;
		Game::Get().load_chest_handler.push_back(this);
	}
}
