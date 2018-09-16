#include "Pch.h"
#include "GameCore.h"
#include "ItemHelper.h"
#include "Chest.h"
#include "Stock.h"

int wartosc_skarbu[] = {
	10, //0
	50, //1
	100, //2
	200, //3
	350, //4
	500, //5
	700, //6
	900, //7
	1200, //8
	1500, //9
	1900, //10
	2400, //11
	2900, //12
	3400, //13
	4000, //14
	4600, //15
	5300, //16
	6000, //17
	6800, //18
	7600, //19
	8500 //20
};

template<typename T>
void InsertRandomItem(vector<ItemSlot>& container, vector<T*>& items, int price_limit, int exclude_flags, uint count = 1)
{
	for(int i = 0; i < 100; ++i)
	{
		T* item = items[Rand() % items.size()];
		if(item->value > price_limit || IS_SET(item->flags, exclude_flags))
			continue;
		InsertItemBare(container, item, count);
		return;
	}
}

//=================================================================================================
void ItemHelper::GenerateTreasure(int level, int _count, vector<ItemSlot>& items, int& gold, bool extra)
{
	assert(InRange(level, 1, 20));

	int value = Random(wartosc_skarbu[level - 1], wartosc_skarbu[level]);

	items.clear();

	const Item* item;
	uint count;

	for(int tries = 0; tries < _count; ++tries)
	{
		switch(Rand() % IT_MAX_GEN)
		{
		case IT_WEAPON:
			item = Weapon::weapons[Rand() % Weapon::weapons.size()];
			count = 1;
			break;
		case IT_ARMOR:
			item = Armor::armors[Rand() % Armor::armors.size()];
			count = 1;
			break;
		case IT_BOW:
			item = Bow::bows[Rand() % Bow::bows.size()];
			count = 1;
			break;
		case IT_SHIELD:
			item = Shield::shields[Rand() % Shield::shields.size()];
			count = 1;
			break;
		case IT_CONSUMABLE:
			item = Consumable::consumables[Rand() % Consumable::consumables.size()];
			count = Random(2, 5);
			break;
		case IT_OTHER:
			item = OtherItem::others[Rand() % OtherItem::others.size()];
			count = Random(1, 4);
			break;
		default:
			assert(0);
			item = nullptr;
			break;
		}

		if(!item->CanBeGenerated())
			continue;

		int cost = item->value * count;
		if(cost > value)
			continue;

		value -= cost;

		InsertItemBare(items, item, count, count);
	}

	if(extra)
		InsertItemBare(items, ItemList::GetItem("treasure"));

	gold = value + level * 5;
}

//=================================================================================================
void ItemHelper::SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count)
{
	assert(gold >= 0 && count > 0 && chests);

	while(!items.empty())
	{
		for(int i = 0; i < count && !items.empty(); ++i)
		{
			chests[i]->items.push_back(items.back());
			items.pop_back();
		}
	}

	int ile = gold / count - 1;
	if(ile < 0)
		ile = 0;
	gold -= ile * count;

	for(int i = 0; i < count; ++i)
	{
		if(i == count - 1)
			ile += gold;
		ItemSlot& slot = Add1(chests[i]->items);
		slot.Set(Item::gold, ile, ile);
		SortItems(chests[i]->items);
	}
}

//=================================================================================================
void ItemHelper::GenerateMerchantItems(vector<ItemSlot>& items, int price_limit)
{
	items.clear();
	InsertItemBare(items, Item::Get("p_nreg"), Random(5, 10));
	InsertItemBare(items, Item::Get("p_hp"), Random(5, 10));
	for(int i = 0, count = Random(15, 20); i < count; ++i)
	{
		switch(Rand() % 6)
		{
		case IT_WEAPON:
			InsertRandomItem(items, Weapon::weapons, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_BOW:
			InsertRandomItem(items, Bow::bows, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_SHIELD:
			InsertRandomItem(items, Shield::shields, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_ARMOR:
			InsertRandomItem(items, Armor::armors, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_CONSUMABLE:
			InsertRandomItem(items, Consumable::consumables, price_limit / 5, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT, Random(2, 5));
			break;
		case IT_OTHER:
			InsertRandomItem(items, OtherItem::others, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		}
	}
	SortItems(items);
}

//=================================================================================================
void ItemHelper::GenerateBlacksmithItems(vector<ItemSlot>& items, int price_limit, int count_mod, bool is_city)
{
	items.clear();
	for(int i = 0, count = Random(12, 18) + count_mod; i < count; ++i)
	{
		switch(Rand() % 4)
		{
		case IT_WEAPON:
			InsertRandomItem(items, Weapon::weapons, price_limit, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
			break;
		case IT_BOW:
			InsertRandomItem(items, Bow::bows, price_limit, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
			break;
		case IT_SHIELD:
			InsertRandomItem(items, Shield::shields, price_limit, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
			break;
		case IT_ARMOR:
			InsertRandomItem(items, Armor::armors, price_limit, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
			break;
		}
	}
	// basic equipment
	Stock::Get("blacksmith")->Parse(5, is_city, items);
	SortItems(items);
}

//=================================================================================================
void ItemHelper::GenerateAlchemistItems(vector<ItemSlot>& items, int count_mod)
{
	items.clear();
	for(int i = 0, count = Random(8, 12) + count_mod; i < count; ++i)
		InsertRandomItem(items, Consumable::consumables, 99999, ITEM_NOT_SHOP | ITEM_NOT_ALCHEMIST, Random(3, 6));
	SortItems(items);
}

//=================================================================================================
void ItemHelper::GenerateInnkeeperItems(vector<ItemSlot>& items, int count_mod, bool is_city)
{
	items.clear();
	Stock::Get("innkeeper")->Parse(5, is_city, items);
	const ItemList* lis2 = ItemList::Get("normal_food").lis;
	for(uint i = 0, count = Random(10, 20) + count_mod; i < count; ++i)
		InsertItemBare(items, lis2->Get());
	SortItems(items);
}

//=================================================================================================
void ItemHelper::GenerateFoodSellerItems(vector<ItemSlot>& items, bool is_city)
{
	items.clear();
	const ItemList* lis = ItemList::Get("food_and_drink").lis;
	for(const Item* item : lis->items)
	{
		uint value = Random(50, 100);
		if(!is_city)
			value /= 2;
		InsertItemBare(items, item, value / item->value);
	}
	if(Rand() % 4 == 0)
		InsertItemBare(items, Item::Get("frying_pan"));
	if(Rand() % 4 == 0)
		InsertItemBare(items, Item::Get("ladle"));
	SortItems(items);
}
