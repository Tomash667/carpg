#include "Pch.h"
#include "GameCore.h"
#include "UnitHelper.h"

namespace UnitHelper
{
	inline void EnsureList(const ItemList*& lis)
	{
		if(!lis)
			lis = ItemList::Get("base_items").lis;
	}
}

const Item* UnitHelper::GetBaseWeapon(const Unit& unit, const ItemList* lis)
{
	EnsureList(lis);

	if(IS_SET(unit.data->flags, F_MAGE))
	{
		for(const Item* item : lis->items)
		{
			if(item->type == IT_WEAPON && IS_SET(item->flags, ITEM_MAGE))
				return item;
		}
	}

	WEAPON_TYPE best = unit.GetBestWeaponType();
	for(const Item* item : lis->items)
	{
		if(item->ToWeapon().weapon_type == best)
			return item;
	}

	// never should happen, list is checked in CheckBaseItems
	return nullptr;
}

const Item* UnitHelper::GetBaseArmor(const Unit& unit, const ItemList* lis)
{
	EnsureList(lis);

	if(IS_SET(unit.data->flags, F_MAGE))
	{
		for(const Item* item : lis->items)
		{
			if(item->type == IT_ARMOR && IS_SET(item->flags, ITEM_MAGE))
				return item;
		}
	}

	ARMOR_TYPE armor_type = unit.GetBestArmorType();
	for(const Item* item : lis->items)
	{
		if(item->type == IT_ARMOR && item->ToArmor().armor_type == armor_type)
			return item;
	}

	return nullptr;
}

const Item* UnitHelper::GetBaseItem(ITEM_TYPE type, const ItemList* lis)
{
	EnsureList(lis);

	for(const Item* item : lis->items)
	{
		if(item->type == type)
			return item;
	}

	return nullptr;
}
