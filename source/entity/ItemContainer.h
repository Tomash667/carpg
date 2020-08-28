#pragma once

//-----------------------------------------------------------------------------
#include "ItemSlot.h"

//-----------------------------------------------------------------------------
struct ItemContainer
{
	vector<ItemSlot> items;

	void Save(GameWriter& f);
	void Load(GameReader& f);

	int FindItem(const Item* item) const;
	int FindQuestItem(int quest_id) const;

	bool AddItem(const Item* item, uint count, uint team_count)
	{
		return InsertItem(items, item, count, team_count);
	}
	bool AddItem(const Item* item, uint count = 1)
	{
		return AddItem(item, count, count);
	}
	void RemoveItem(int index)
	{
		items.erase(items.begin() + index);
	}
};
