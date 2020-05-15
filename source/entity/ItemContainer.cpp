#include "Pch.h"
#include "Item.h"

#include "Content.h"
#include "ItemContainer.h"
#include "QuestManager.h"
#include "SaveState.h"

//=================================================================================================
void ItemContainer::Save(FileWriter& f)
{
	f << items.size();
	for(auto& slot : items)
	{
		assert(slot.item);
		f << slot.item->id;
		f << slot.count;
		f << slot.team_count;
		if(slot.item->id[0] == '$')
			f << slot.item->quest_id;
	}
}

//=================================================================================================
void ItemContainer::Load(FileReader& f)
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
		f >> slot.team_count;
		if(item_id[0] != '$')
			slot.item = Item::Get(item_id);
		else
		{
			int quest_id;
			f >> quest_id;
			quest_mgr->AddQuestItemRequest(&slot.item, item_id.c_str(), quest_id, &items);
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
int ItemContainer::FindQuestItem(int quest_id) const
{
	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item->IsQuest(quest_id))
			return index;
	}
	return -1;
}
