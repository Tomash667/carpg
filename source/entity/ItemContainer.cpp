#include "Pch.h"
#include "Item.h"

#include "Content.h"
#include "ItemContainer.h"
#include "QuestManager.h"

//=================================================================================================
void ItemContainer::Save(GameWriter& f)
{
	f << items.size();
	for(ItemSlot& slot : items)
	{
		assert(slot.item);
		f << slot.item->id;
		f << slot.count;
		f << slot.teamCount;
		if(slot.item->id[0] == '$')
			f << slot.item->questId;
	}
}

//=================================================================================================
void ItemContainer::Load(GameReader& f)
{
	bool canSort = true;

	uint count;
	f >> count;
	if(count == 0u)
		return;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		const string& itemId = f.ReadString1();
		f >> slot.count;
		f >> slot.teamCount;
		if(itemId[0] != '$')
			slot.item = Item::Get(itemId);
		else
		{
			int questId;
			f >> questId;
			questMgr->AddQuestItemRequest(&slot.item, itemId.c_str(), questId, &items);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			canSort = false;
		}
	}

	if(canSort && !items.empty() && content.requireUpdate)
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
int ItemContainer::FindQuestItem(int questId) const
{
	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item->IsQuest(questId))
			return index;
	}
	return -1;
}
