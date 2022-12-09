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
	int FindQuestItem(int questId) const;

	bool AddItem(const Item* item, uint count, uint teamCount)
	{
		return InsertItem(items, item, count, teamCount);
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
