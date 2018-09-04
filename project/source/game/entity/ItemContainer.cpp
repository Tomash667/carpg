#include "Pch.h"
#include "GameCore.h"
#include "Item.h"
#include "ItemContainer.h"
#include "QuestManager.h"
#include "SaveState.h"
#include "Content.h"

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
			f << slot.item->refid;
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
			int quest_refid;
			f >> quest_refid;
			QM.AddQuestItemRequest(&slot.item, item_id.c_str(), quest_refid, &items);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}

	if(can_sort && !items.empty() && (LOAD_VERSION < V_0_2_20 || content::require_update))
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
