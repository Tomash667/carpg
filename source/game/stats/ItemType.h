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
	IT_CONSUMEABLE,
	IT_BOOK,
	IT_GOLD,

	IT_MAX_GEN = IT_BOOK,
	IT_LIST,
	IT_LEVELED_LIST,
	IT_STOCK,
	IT_BOOK_SCHEMA
};
