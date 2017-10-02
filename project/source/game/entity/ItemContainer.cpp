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
		ReadString1(file);
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

	if(can_sort && LOAD_VERSION < V_0_2_20 && !items.empty())
		SortItems(items);
}

//=================================================================================================
int ItemContainer::FindItem(const Item* item) const
{
	assert(item);
	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item == item)
			return index;
	}
	return -1;
}

//=================================================================================================
int ItemContainer::FindQuestItem(int quest_refid) const
{
	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item->IsQuest(quest_refid))
			return index;
	}
	return -1;
}
