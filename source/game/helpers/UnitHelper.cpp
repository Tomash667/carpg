#include "Pch.h"
#include "Base.h"
#include "UnitHelper.h"

namespace UnitHelper
{
	inline void EnsureList(const ItemList*& lis)
	{
		if(!lis)
			lis = FindItemList("base_items").lis;
	}
}

const Item* UnitHelper::GetBaseWeapon(const Unit& unit, const ItemList* lis)
{
	EnsureList(lis);

	if(IS_SET(unit.data->flags, F_MAGE))
	{
		for(const Item* item : lis->items)
		{
			if(item->type == IT_WEAPON && (item->flags, ITEM_MAGE))
				return item;
		}
	}

	Skill best_skill = unit.GetBestWeaponSkill();
	for(const Item* item : lis->items)
	{
		if(item->ToWeapon().GetInfo().skill == best_skill)
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

	Skill best_skill = unit.GetBestArmorSkill();
	for(const Item* item : lis->items)
	{
		if(item->type == IT_ARMOR && item->ToArmor().skill == best_skill)
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
