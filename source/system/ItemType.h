#pragma once

//-----------------------------------------------------------------------------
/* Item types - Adding new item type require changes in:
ItemLoader.cpp
UnitLoader.cpp - to allow selling
If equippable
	Item.IsWearable
	ItemSlot.h - Add new slots in ItemSlot.h (ItemTypeToSlot)
	Unit.UpdateInventory, IsBetterItem, CanWear, Save/Load, Read/Write for visible slots
	Team.CheckTeamItemShares, BuyTeamItems
	If item have multiple slots:
		Unit.AddItemAndEquipIfNone
		Unit.HaveItemEquipped
		Inventory.cpp
	UnitStats defaultPriorities
If need special texts in inventory:
	ItemSlot.GetItemString
If require special ordering:
	Item.cpp:ItemCmp
If can be generated:
	ItemHelper.GenerateTreasure
Game2.cpp:GetItemSound
*/
enum ITEM_TYPE
{
	IT_NONE = -1,

	IT_WEAPON,
	IT_BOW,
	IT_SHIELD,
	IT_ARMOR,
	IT_AMULET,
	IT_RING,
	IT_OTHER,
	IT_MAX_WEARABLE = IT_OTHER, // used in Team.cpp priorities
	IT_CONSUMABLE,
	IT_BOOK,
	IT_GOLD,
	IT_MAX_GEN = IT_GOLD, // items generated in treasure

	// special types (not really items)
	IT_LIST,
	IT_STOCK,
	IT_BOOK_SCHEME,
	IT_START_ITEMS,
	IT_BETTER_ITEMS,
	IT_RECIPE,
	IT_ALIAS,
	IT_RECIPE_ALIAS,

	IT_MAX
};
