#include "Pch.h"
#include "UnitHelper.h"

namespace UnitHelper
{
	inline void EnsureList(const ItemList*& lis)
	{
		if(!lis)
			lis = &ItemList::Get("base_items");
	}

	static vector<const Item*> items_to_pick;
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
		if(item->ToWeapon().weapon_type == best)
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

	ARMOR_TYPE armor_type = unit.GetBestArmorType();
	for(const ItemList::Entry& e : lis->items)
	{
		const Item* item = e.item;
		if(item->type == IT_ARMOR && item->ToArmor().armor_type == armor_type)
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
	const Item* amulet = unit.slots[SLOT_AMULET];
	float prev_value = amulet ? unit.GetItemAiValue(amulet) : 0.f;
	float best_value = prev_value;
	items_to_pick.clear();
	for(const ItemList::Entry& e : lis.items)
	{
		const Item* item = e.item;
		if(item == amulet)
			continue;
		if(item->value > unit.gold)
			continue;
		float value = unit.GetItemAiValue(item);
		if(value > best_value)
		{
			best_value = value;
			items_to_pick.clear();
			items_to_pick.push_back(item);
		}
		else if(value == best_value && value != 0 && !items_to_pick.empty())
			items_to_pick.push_back(item);
	}
	const Item* best_item = (items_to_pick.empty() ? nullptr : RandomItem(items_to_pick));
	return { best_item, best_value, prev_value };
}

array<UnitHelper::BetterItem, 2> UnitHelper::GetBetterRings(const Unit& unit)
{
	const Item* rings[2] = { unit.slots[SLOT_RING1], unit.slots[SLOT_RING2] };
	float value[2] = {
		rings[0] ? unit.GetItemAiValue(rings[0]) : 0,
		rings[1] ? unit.GetItemAiValue(rings[1]) : 0
	};
	array<pair<const Item*, float>, 2> inner_result = GetBetterRingsInternal(unit, Min(value));
	array<BetterItem, 2> result;
	if(inner_result[0].first)
	{
		if(value[0] > inner_result[0].second)
		{
			result[0] = { inner_result[0].first, inner_result[0].second, value[1] };
			result[1] = { nullptr, 0.f, 0.f };
		}
		else
		{
			result[0] = { inner_result[0].first, inner_result[0].second, value[0] };
			result[1] = { inner_result[1].first, inner_result[1].second, value[1] };
		}
	}
	else
	{
		result[0].item = nullptr;
		result[1].item = nullptr;
	}
	return result;
}

array<pair<const Item*, float>, 2> UnitHelper::GetBetterRingsInternal(const Unit& unit, float min_value)
{
	static const ItemList& lis = ItemList::Get("rings");
	const Item* rings[2] = { unit.slots[SLOT_RING1], unit.slots[SLOT_RING2] };

	items_to_pick.clear();
	const Item* second_best_item = nullptr;
	float second_best_value = 0.f;
	for(const ItemList::Entry& e : lis.items)
	{
		const Item* item = e.item;
		if(item->value > unit.gold)
			continue;
		if(item == rings[0] && item == rings[1])
			continue;
		float value = unit.GetItemAiValue(item);
		if(value > min_value)
		{
			if(!items_to_pick.empty())
			{
				second_best_item = RandomItem(items_to_pick);
				second_best_value = min_value;
				items_to_pick.clear();
			}
			items_to_pick.push_back(item);
			min_value = value;
		}
		else if(value != 0 && !items_to_pick.empty())
		{
			if(value == min_value)
				items_to_pick.push_back(item);
			else if(!second_best_item)
			{
				second_best_item = item;
				second_best_value = value;
			}
		}
	}

	array<pair<const Item*, float>, 2> result;
	if(items_to_pick.size() >= 2u)
	{
		result[0] = { RandomItemPop(items_to_pick), min_value };
		result[1] = { RandomItem(items_to_pick), min_value };
	}
	else if(!items_to_pick.empty())
	{
		result[0] = { items_to_pick.back(), min_value };
		result[1] = { second_best_item, second_best_value };
	}
	else
	{
		result[0] = { second_best_item, second_best_value };
		result[1] = { nullptr, 0.f };
	}
	return result;
}
