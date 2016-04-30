#pragma once

//-----------------------------------------------------------------------------
// Item types
// Adding new item type require changes in:
// + TeamItems.cpp
// ? incomplete
enum ITEM_TYPE
{
	IT_WEAPON,
	IT_BOW,
	IT_SHIELD,
	IT_ARMOR,
	IT_OTHER,
	IT_CONSUMABLE,
	IT_MAX_GEN = IT_CONSUMABLE, // items generated in treasure
	IT_BOOK,
	IT_GOLD,

	// special types (not realy items)
	IT_LIST,
	IT_LEVELED_LIST,
	IT_STOCK,
	IT_BOOK_SCHEMA,
	IT_START_ITEMS,
	IT_BETTER_ITEMS,
	IT_ALIAS
};
