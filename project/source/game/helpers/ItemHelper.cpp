#include "Pch.h"
#include "GameCore.h"
#include "ItemHelper.h"
#include "Chest.h"

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
