#include "Pch.h"
#include "UnitHelper.h"

namespace UnitHelper
{
	inline void EnsureList(const ItemList*& lis)
	{
		if(!lis)
			lis = &ItemList::Get("base_items");
	}

	static vector<const Item*> itemsToPick;
}

const Item* UnitHelper::GetBaseWeapon(const Unit& unit, const ItemList* lis)
{
	EnsureList(lis);

	if(IsSet(unit.data->flags, F_MAGE))
	{
		for(const ItemList::Entry& e : lis->items)
		{
			const Item* item = e.item;
			if(item->type == IT_WEAPON && IsSet(item->flags, ITEM_MAGE))
				return item;
		}
	}

	WEAPON_TYPE best = unit.GetBestWeaponType();
	for(const ItemList::Entry& e : lis->items)
	{
		const Item* item = e.item;
		if(item->ToWeapon().weaponType == best)
			return item;
	}

	// never should happen, list is checked in CheckBaseItems
	return nullptr;
}

const Item* UnitHelper::GetBaseArmor(const Unit& unit, const ItemList* lis)
{
	EnsureList(lis);

	if(IsSet(unit.data->flags, F_MAGE))
	{
		for(const ItemList::Entry& e : lis->items)
		{
			const Item* item = e.item;
			if(item->type == IT_ARMOR && IsSet(item->flags, ITEM_MAGE))
				return item;
		}
	}

	ARMOR_TYPE armorType = unit.GetBestArmorType();
	for(const ItemList::Entry& e : lis->items)
	{
		const Item* item = e.item;
		if(item->type == IT_ARMOR && item->ToArmor().armorType == armorType)
			return item;
	}

	return nullptr;
}

const Item* UnitHelper::GetBaseItem(ITEM_TYPE type, const ItemList* lis)
{
	EnsureList(lis);

	for(const ItemList::Entry& e : lis->items)
	{
		const Item* item = e.item;
		if(item->type == type)
			return item;
	}

	return nullptr;
}

UnitHelper::BetterItem UnitHelper::GetBetterAmulet(const Unit& unit)
{
	static const ItemList& lis = ItemList::Get("amulets");
	const Item* amulet = unit.GetEquippedItem(SLOT_AMULET);
	float prevValue = amulet ? unit.GetItemAiValue(amulet) : 0.f;
	float bestValue = prevValue;
	itemsToPick.clear();
	for(const ItemList::Entry& e : lis.items)
	{
		const Item* item = e.item;
		if(item == amulet)
			continue;
		if(item->value > unit.gold)
			continue;
		float value = unit.GetItemAiValue(item);
		if(value > bestValue)
		{
			bestValue = value;
			itemsToPick.clear();
			itemsToPick.push_back(item);
		}
		else if(value == bestValue && value != 0 && !itemsToPick.empty())
			itemsToPick.push_back(item);
	}
	const Item* bestItem = (itemsToPick.empty() ? nullptr : RandomItem(itemsToPick));
	return { bestItem, bestValue, prevValue };
}

array<UnitHelper::BetterItem, 2> UnitHelper::GetBetterRings(const Unit& unit)
{
	const Item* rings[2] = { unit.GetEquippedItem(SLOT_RING1), unit.GetEquippedItem(SLOT_RING2) };
	float value[2] = {
		rings[0] ? unit.GetItemAiValue(rings[0]) : 0,
		rings[1] ? unit.GetItemAiValue(rings[1]) : 0
	};
	array<pair<const Item*, float>, 2> innerResult = GetBetterRingsInternal(unit, Min(value));
	array<BetterItem, 2> result;
	if(innerResult[0].first)
	{
		if(value[0] > innerResult[0].second)
		{
			result[0] = { innerResult[0].first, innerResult[0].second, value[1] };
			result[1] = { nullptr, 0.f, 0.f };
		}
		else
		{
			result[0] = { innerResult[0].first, innerResult[0].second, value[0] };
			result[1] = { innerResult[1].first, innerResult[1].second, value[1] };
		}
	}
	else
	{
		result[0].item = nullptr;
		result[1].item = nullptr;
	}
	return result;
}

array<pair<const Item*, float>, 2> UnitHelper::GetBetterRingsInternal(const Unit& unit, float minValue)
{
	static const ItemList& lis = ItemList::Get("rings");
	const Item* rings[2] = { unit.GetEquippedItem(SLOT_RING1), unit.GetEquippedItem(SLOT_RING2) };

	itemsToPick.clear();
	const Item* secondBestItem = nullptr;
	float secondBestValue = 0.f;
	for(const ItemList::Entry& e : lis.items)
	{
		const Item* item = e.item;
		if(item->value > unit.gold)
			continue;
		if(item == rings[0] && item == rings[1])
			continue;
		float value = unit.GetItemAiValue(item);
		if(value > minValue)
		{
			if(!itemsToPick.empty())
			{
				secondBestItem = RandomItem(itemsToPick);
				secondBestValue = minValue;
				itemsToPick.clear();
			}
			itemsToPick.push_back(item);
			minValue = value;
		}
		else if(value != 0 && !itemsToPick.empty())
		{
			if(value == minValue)
				itemsToPick.push_back(item);
			else if(!secondBestItem)
			{
				secondBestItem = item;
				secondBestValue = value;
			}
		}
	}

	array<pair<const Item*, float>, 2> result;
	if(itemsToPick.size() >= 2u)
	{
		result[0] = { RandomItemPop(itemsToPick), minValue };
		result[1] = { RandomItem(itemsToPick), minValue };
	}
	else if(!itemsToPick.empty())
	{
		result[0] = { itemsToPick.back(), minValue };
		result[1] = { secondBestItem, secondBestValue };
	}
	else
	{
		result[0] = { secondBestItem, secondBestValue };
		result[1] = { nullptr, 0.f };
	}
	return result;
}

int UnitHelper::CalculateChance(int value, int min, int max)
{
	if(value <= min)
		return 0;
	if(value >= max)
		return 100;
	const int dif = max - min;
	const float ratio = float(value) / dif;
	return int(ratio * 100 * dif) + min;
}
