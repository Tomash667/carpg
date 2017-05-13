#pragma once

#include "Unit.h"

namespace UnitHelper
{
	const Item* GetBaseWeapon(const Unit& unit, const ItemList* lis = nullptr);
	const Item* GetBaseArmor(const Unit& unit, const ItemList* lis = nullptr);
	const Item* GetBaseItem(ITEM_TYPE type, const ItemList* lis = nullptr);
	inline const Item* GetBaseBow(const ItemList* lis = nullptr) { return GetBaseItem(IT_BOW, lis); }
	inline const Item* GetBaseShield(const ItemList* lis = nullptr) { return GetBaseItem(IT_SHIELD, lis); }
}
