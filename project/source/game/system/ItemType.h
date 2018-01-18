#pragma once

//-----------------------------------------------------------------------------
// Item types
// Adding new item type require changes in:
// + TeamItems.cpp
// + Game2.cpp allow buy merchant_buy
// + Worldmap.cpp GenerateStockItems
// ? incomplete
enum ITEM_TYPE
{
	IT_WEAPON,
	IT_BOW,
	IT_SHIELD,
	IT_ARMOR,
	IT_OTHER,
	IT_CONSUMABLE,
	IT_BOOK,
	IT_MAX_GEN = IT_BOOK, // items generated in treasure
	IT_GOLD,

	// special types (not realy items)
	IT_LIST,
	IT_LEVELED_LIST,
	IT_STOCK,
	IT_BOOK_SCHEME,
	IT_START_ITEMS,
	IT_BETTER_ITEMS,
	IT_ALIAS,

	IT_MAX
};
