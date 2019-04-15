#pragma once

//-----------------------------------------------------------------------------
#include "Unit.h"

//-----------------------------------------------------------------------------
namespace UnitHelper
{
	struct BetterItem
	{
		const Item* item;
		float value;
		float prev_value;
	};

	const Item* GetBaseWeapon(const Unit& unit, const ItemList* lis = nullptr);
	const Item* GetBaseArmor(const Unit& unit, const ItemList* lis = nullptr);
	const Item* GetBaseItem(ITEM_TYPE type, const ItemList* lis = nullptr);
	BetterItem GetBetterAmulet(const Unit& unit);
	array<BetterItem, 2> GetBetterRings(const Unit& unit);
	array<pair<const Item*, float>, 2> GetBetterRingsInternal(const Unit& unit, float min_value);
}
