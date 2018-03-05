#pragma once

//-----------------------------------------------------------------------------
/* Item types - Adding new item type require changes in:
If equippable
	Add new slots in ItemSlot.h
	Unit.AddItemAndEquipIfNone, HaveItem, IsBetterItem
	Game2.cpp:UpdateUnitInventory
	Game2.cpp:IsBetterItem
	TeamItems.cpp:array item_type_prio, BuyTeamItems
If need special texts in inventory:
	ItemSlot.GetItemString
If require special ordering:
	Item.cpp:ItemCmp
If can be generated:
	Game2.cpp:GenerateTreasure
Item.cpp:LoadItem
Game2.cpp:GetItemSound, merchant_buy etc
*/
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
