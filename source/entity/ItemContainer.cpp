#include "Pch.h"
#include "Item.h"

#include "Content.h"
#include "ItemContainer.h"
#include "QuestManager.h"

//=================================================================================================
void ItemContainer::Save(GameWriter& f)
{
	f << items.size();
	for(auto& slot : items)
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
	bool can_sort = true;

	uint count;
	f >> count;
	if(count == 0u)
		return;

	items.resize(count);
	for(auto& slot : items)
	{
		const string& item_id = f.ReadString1();
		f >> slot.count;
		f >> slot.teamCount;
		if(item_id[0] != '$')
			slot.item = Item::Get(item_id);
		else
		{
			int questId;
			f >> questId;
			questMgr->AddQuestItemRequest(&slot.item, item_id.c_str(), questId, &items);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}

	if(can_sort && !items.empty() && content.require_update)
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
