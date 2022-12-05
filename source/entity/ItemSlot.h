#pragma once

//-----------------------------------------------------------------------------
#include "ItemType.h"

//-----------------------------------------------------------------------------
enum ITEM_SLOT
{
	SLOT_WEAPON,
	SLOT_BOW,
	SLOT_SHIELD,
	SLOT_ARMOR,
	SLOT_AMULET,
	SLOT_MAX_VISIBLE = SLOT_AMULET,
	SLOT_RING1,
	SLOT_RING2,
	SLOT_MAX,
	SLOT_INVALID
};

//-----------------------------------------------------------------------------
inline constexpr int SlotToIIndex(ITEM_SLOT s)
{
	return -s - 1;
}
inline constexpr ITEM_SLOT IIndexToSlot(int iIndex)
{
	ITEM_SLOT s = ITEM_SLOT(-iIndex - 1);
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
	case IT_AMULET:
		return SLOT_AMULET;
	case IT_RING:
		return SLOT_RING1;
	default:
		return SLOT_INVALID;
	}
}

//-----------------------------------------------------------------------------
inline bool IsValid(ITEM_SLOT slot)
{
	return slot >= SLOT_WEAPON && slot < SLOT_MAX;
}
inline bool IsVisible(ITEM_SLOT slot)
{
	return slot < SLOT_MAX_VISIBLE;
}

//-----------------------------------------------------------------------------
struct ItemSlotInfo
{
	cstring id;

	static ItemSlotInfo slots[SLOT_MAX];
	static ITEM_SLOT Find(const string& id);
};

//-----------------------------------------------------------------------------
struct ItemSlot
{
	const Item* item;
	uint count, teamCount;

	void operator = (const ItemSlot& slot)
	{
		item = slot.item;
		count = slot.count;
		teamCount = slot.teamCount;
	}

	void Set(const Item* item, uint count, uint teamCount = 0)
	{
		this->item = item;
		this->count = count;
		this->teamCount = teamCount;
	}
};

//-----------------------------------------------------------------------------
// Sortuje przedmioty wed³ug kolejnoœci ITEM_TYPE i ceny
void SortItems(vector<ItemSlot>& items);

void GetItemString(string& str, const Item* item, Unit* unit, uint count = 1);

bool InsertItemStackable(vector<ItemSlot>& items, ItemSlot& slot);
void InsertItemNotStackable(vector<ItemSlot>& items, ItemSlot& slot);

// dodaje przedmiot do ekwipunku, sprawdza czy siê stackuje, zwraca true jeœli siê zestackowa³
bool InsertItem(vector<ItemSlot>& items, ItemSlot& slot);
inline bool InsertItem(vector<ItemSlot>& items, const Item* item, uint count, uint teamCount)
{
	ItemSlot slot;
	slot.Set(item, count, teamCount);
	return InsertItem(items, slot);
}

// add item without sorting (assume that vector is unsorted)
void InsertItemBare(vector<ItemSlot>& items, const Item* item, uint count, uint teamCount);
inline void InsertItemBare(vector<ItemSlot>& items, const Item* item, uint count = 1, bool isTeam = true)
{
	InsertItemBare(items, item, count, isTeam ? count : 0);
}

void SetItemStatsText();

inline void RemoveNullItems(vector<ItemSlot>& items)
{
	RemoveElements(items, [](ItemSlot& slot) { return slot.item == nullptr; });
}

int FindItemIndex(const vector<ItemSlot>& items, int index, const Item* item, bool isTeam);
