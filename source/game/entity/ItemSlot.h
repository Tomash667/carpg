#pragma once

//-----------------------------------------------------------------------------
#include "ItemType.h"

//-----------------------------------------------------------------------------
struct Item;
struct Unit;

//-----------------------------------------------------------------------------
enum ITEM_SLOT
{
	SLOT_WEAPON,
	SLOT_BOW,
	SLOT_SHIELD,
	SLOT_ARMOR,
	SLOT_MAX,
	SLOT_INVALID
};

//-----------------------------------------------------------------------------
inline int SlotToIIndex(ITEM_SLOT s)
{
	return -s - 1;
}
inline ITEM_SLOT IIndexToSlot(int i_index)
{
	ITEM_SLOT s = ITEM_SLOT(-i_index - 1);
	assert(s >= SLOT_WEAPON && s < SLOT_MAX);
	return s;
}

//-----------------------------------------------------------------------------
inline ITEM_SLOT ItemTypeToSlot(ITEM_TYPE type)
{
	switch(type)
	{
	case IT_WEAPON:
		return SLOT_WEAPON;
	case IT_BOW:
		return SLOT_BOW;
	case IT_SHIELD:
		return SLOT_SHIELD;
	case IT_ARMOR:
		return SLOT_ARMOR;
	default:
		return SLOT_INVALID;
	}
}

//-----------------------------------------------------------------------------
inline bool IsValid(ITEM_SLOT slot)
{
	return slot >= IT_WEAPON && slot < SLOT_MAX;
}

//-----------------------------------------------------------------------------
struct ItemSlot
{
	const Item* item;
	uint count, team_count;

	inline void operator = (const ItemSlot& slot)
	{
		item = slot.item;
		count = slot.count;
		team_count = slot.team_count;
	}

	inline void Set(const Item* _item, uint _count, uint _team_count=0)
	{
		item = _item;
		count = _count;
		team_count = _team_count;
	}
};

//-----------------------------------------------------------------------------
// Sortuje przedmioty wed³ug kolejnoœci ITEM_TYPE i ceny
void SortItems(vector<ItemSlot>& items);

void GetItemString(string& str, const Item* item, Unit* unit, uint count=1);

bool InsertItemStackable(vector<ItemSlot>& items, ItemSlot& slot);
void InsertItemNotStackable(vector<ItemSlot>& items, ItemSlot& slot);

// dodaje przedmiot do ekwipunku, sprawdza czy siê stackuje, zwraca true jeœli siê zestackowa³
bool InsertItem(vector<ItemSlot>& items, ItemSlot& slot);
inline bool InsertItem(vector<ItemSlot>& items, const Item* item, uint count, uint team_count)
{
	ItemSlot slot;
	slot.Set(item, count, team_count);
	return InsertItem(items, slot);
}

// add item without sorting (assume that vector is unsorted), should be stackable if count > 1
void InsertItemBare(vector<ItemSlot>& items, const Item* item, uint count, uint team_count);
inline void InsertItemBare(vector<ItemSlot>& items, const Item* item, uint count=1, bool is_team=true)
{
	InsertItemBare(items, item, count, is_team ? count : 0);
}

void SetItemStatsText();

inline bool IsEmpty(const ItemSlot& slot)
{
	return slot.item == nullptr;
}

inline void RemoveNullItems(vector<ItemSlot>& items)
{
	RemoveElements(items, IsEmpty);
}
