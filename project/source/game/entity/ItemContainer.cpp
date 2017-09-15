#include "Pch.h"
#include "Core.h"
#include "Item.h"
#include "ItemContainer.h"
#include "QuestManager.h"
#include "SaveState.h"

//=================================================================================================
void ItemContainer::Save(HANDLE file)
{
	uint count = items.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);

	for(auto& slot : items)
	{
		assert(slot.item);
		WriteString1(file, slot.item->id);
		WriteFile(file, &slot.count, sizeof(slot.count), &tmp, nullptr);
		WriteFile(file, &slot.team_count, sizeof(slot.team_count), &tmp, nullptr);
		if(slot.item->id[0] == '$')
			WriteFile(file, &slot.item->refid, sizeof(int), &tmp, nullptr);
	}
}

//=================================================================================================
void ItemContainer::Load(HANDLE file)
{
	bool can_sort = true;

	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	if(count == 0u)
		return;

	items.resize(count);
	for(auto& slot : items)
	{
		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		if(len)
		{
			ReadFile(file, BUF, len, &tmp, nullptr);
			BUF[len] = 0;
			ReadFile(file, &slot.count, sizeof(slot.count), &tmp, nullptr);
			ReadFile(file, &slot.team_count, sizeof(slot.team_count), &tmp, nullptr);
			if(BUF[0] != '$')
				slot.item = ::FindItem(BUF);
			else
			{
				int quest_refid;
				ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, nullptr);
				QuestManager::Get().AddQuestItemRequest(&slot.item, BUF, quest_refid, &items);
				slot.item = QUEST_ITEM_PLACEHOLDER;
				can_sort = false;
			}
		}
		else
		{
			assert(LOAD_VERSION < V_0_2_10);
			slot.item = nullptr;
			slot.count = 0;
		}
	}

	if(can_sort && LOAD_VERSION < V_0_2_20 && !items.empty())
	{
		if(LOAD_VERSION < V_0_2_10)
			RemoveNullItems(items);
		SortItems(items);
	}
}
