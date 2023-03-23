#include "Pch.h"
#include "ItemHelper.h"

#include "Chest.h"
#include "Stock.h"
#include "Unit.h"

int treasureValue[] = {
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
	2000, //10
	2500, //11
	3000, //12
	3500, //13
	4000, //14
	5000, //15
	6000, //16
	7000, //17
	8000, //18
	9000, //19
	10000 //20
};

const float priceModBuy[] = { 1.75f, 1.25f, 1.f, 0.9f, 0.775f };
const float priceModBuyV[] = { 1.75f, 1.25f, 1.f, 0.95f, 0.925f };
const float priceModSell[] = { 0.25f, 0.5f, 0.6f, 0.7f, 0.75f };
const float priceModSellV[] = { 0.5f, 0.65f, 0.75f, 0.85f, 0.9f };

template<typename T>
void InsertRandomItem(vector<ItemSlot>& container, vector<T*>& items, int priceLimit, int excludeFlags, uint count)
{
	for(int i = 0; i < 100; ++i)
	{
		T* item = items[Rand() % items.size()];
		if(item->value > priceLimit || IsSet(item->flags, excludeFlags))
			continue;
		InsertItemBare(container, item, count);
		return;
	}
}

//=================================================================================================
void ItemHelper::GenerateTreasure(int level, int _count, vector<ItemSlot>& items, int& gold, bool extra)
{
	assert(InRange(level, 1, 20));

	int value = Random(treasureValue[level - 1], treasureValue[level]) * _count / 10;

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
		case IT_BOW:
			item = Bow::bows[Rand() % Bow::bows.size()];
			count = 1;
			break;
		case IT_SHIELD:
			item = Shield::shields[Rand() % Shield::shields.size()];
			count = 1;
			break;
		case IT_ARMOR:
			item = Armor::armors[Rand() % Armor::armors.size()];
			count = 1;
			break;
		case IT_AMULET:
			item = Amulet::amulets[Rand() % Amulet::amulets.size()];
			count = 1;
			break;
		case IT_RING:
			item = Ring::rings[Rand() % Ring::rings.size()];
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
		case IT_BOOK:
			item = Book::books[Rand() % Book::books.size()];
			count = 1;
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

	gold = value + Random(5 * level, 10 * level);
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

	int dividedCount = gold / count - 1;
	if(dividedCount < 0)
		dividedCount = 0;
	gold -= dividedCount * count;

	for(int i = 0; i < count; ++i)
	{
		if(i == count - 1)
			dividedCount += gold;
		ItemSlot& slot = Add1(chests[i]->items);
		slot.Set(Item::gold, dividedCount, dividedCount);
		SortItems(chests[i]->items);
	}
}

//=================================================================================================
void ItemHelper::GenerateTreasure(vector<Chest*>& chests, int level, int count)
{
	if(chests.empty())
		return;

	static vector<ItemSlot> items;
	int gold;
	GenerateTreasure(level, 10 * count, items, gold, false);
	SplitTreasure(items, gold, chests.data(), chests.size());
}

//=================================================================================================
int ItemHelper::GetItemPrice(const Item* item, Unit& unit, bool buy)
{
	assert(item);

	int persuasion = unit.Get(SkillId::PERSUASION) + unit.Get(AttributeId::CHA) - 50;
	if(unit.IsPlayer())
	{
		if(unit.player->HavePerk(Perk::asocial))
			persuasion -= 20;
		if(unit.player->action == PlayerAction::Trade
			&& unit.player->actionUnit->data->id == "alchemist" && unit.player->HavePerk(Perk::alchemistApprentice))
			persuasion += 20;
	}

	const float* modTable;
	if(item->type == IT_OTHER && item->ToOther().subtype == OtherItem::Subtype::Valuable)
	{
		if(buy)
			modTable = priceModBuyV;
		else
			modTable = priceModSellV;
	}
	else
	{
		if(buy)
			modTable = priceModBuy;
		else
			modTable = priceModSell;
	}

	float mod;
	if(persuasion <= -25)
		mod = modTable[0];
	else if(persuasion <= 0)
		mod = Lerp(modTable[0], modTable[1], float(persuasion + 25) / 25);
	else if(persuasion <= 15)
		mod = Lerp(modTable[1], modTable[2], float(persuasion) / 15);
	else if(persuasion <= 50)
		mod = Lerp(modTable[2], modTable[3], float(persuasion - 15) / 30);
	else if(persuasion <= 150)
		mod = Lerp(modTable[3], modTable[4], float(persuasion - 50) / 100);
	else
		mod = modTable[4];

	int price = int(mod * item->value);
	if(price == 0 && buy)
		price = 1;

	return price;
}

//=================================================================================================
// zwraca losowy przedmiot o maksymalnej cenie, ta funkcja jest powolna!
// mo¿e zwróciæ questowy przedmiot jeœli bêdzie wystarczaj¹co tani, lub unikat!
const Item* ItemHelper::GetRandomItem(int maxValue)
{
	int type = Rand() % 6;

	LocalVector<const Item*> items;

	switch(type)
	{
	case 0:
		for(Weapon* w : Weapon::weapons)
		{
			if(w->value <= maxValue && w->CanBeGenerated())
				items->push_back(w);
		}
		break;
	case 1:
		for(Bow* b : Bow::bows)
		{
			if(b->value <= maxValue && b->CanBeGenerated())
				items->push_back(b);
		}
		break;
	case 2:
		for(Shield* s : Shield::shields)
		{
			if(s->value <= maxValue && s->CanBeGenerated())
				items->push_back(s);
		}
		break;
	case 3:
		for(Armor* a : Armor::armors)
		{
			if(a->value <= maxValue && a->CanBeGenerated())
				items->push_back(a);
		}
		break;
	case 4:
		for(Consumable* c : Consumable::consumables)
		{
			if(c->value <= maxValue && c->CanBeGenerated())
				items->push_back(c);
		}
		break;
	case 5:
		for(OtherItem* o : OtherItem::others)
		{
			if(o->value <= maxValue && o->CanBeGenerated())
				items->push_back(o);
		}
		break;
	}

	return items->at(Rand() % items->size());
}

//=================================================================================================
const Item* ItemHelper::GetBetterItem(const Item* item)
{
	assert(item);

	auto it = betterItems.find(item);
	if(it != betterItems.end())
		return it->second;

	return nullptr;
}

//=================================================================================================
void ItemHelper::SkipStock(GameReader& f)
{
	uint count;
	f >> count;
	if(count == 0)
		return;

	for(uint i = 0; i < count; ++i)
	{
		const string& itemId = f.ReadString1();
		f.Skip<uint>(); // count
		if(itemId[0] == '$')
			f.Skip<int>(); // questId
	}
}

//=================================================================================================
void ItemHelper::AddRandomItem(vector<ItemSlot>& items, ITEM_TYPE type, int priceLimit, int flags, uint count)
{
	switch(type)
	{
	case IT_WEAPON:
		InsertRandomItem(items, Weapon::weapons, priceLimit, flags, count);
		break;
	case IT_BOW:
		InsertRandomItem(items, Bow::bows, priceLimit, flags, count);
		break;
	case IT_SHIELD:
		InsertRandomItem(items, Shield::shields, priceLimit, flags, count);
		break;
	case IT_ARMOR:
		InsertRandomItem(items, Armor::armors, priceLimit, flags, count);
		break;
	case IT_OTHER:
		InsertRandomItem(items, OtherItem::others, priceLimit, flags, count);
		break;
	case IT_CONSUMABLE:
		InsertRandomItem(items, Consumable::consumables, priceLimit, flags, count);
		break;
	case IT_BOOK:
		InsertRandomItem(items, Book::books, priceLimit, flags, count);
		break;
	}
}

//=================================================================================================
int ItemHelper::CalculateReward(int level, const Int2& levelRange, const Int2& priceRange)
{
	level = levelRange.Clamp(level);
	float t = float(level - levelRange.x) / float(levelRange.y - levelRange.x);
	return priceRange.Lerp(t);
}

//=================================================================================================
int ItemHelper::GetRestCost(int days)
{
	assert(InRange(days, 1, 60));

	const pair<int, int> prices[] = {
		{ 1, 5 },
		{ 5, 20 },
		{ 10, 35 },
		{ 30, 100 },
		{ 60, 200 }
	};

	for(uint i = 0; i < countof(prices); ++i)
	{
		if(prices[i].first == days)
			return prices[i].second;
		else if(prices[i].first > days)
		{
			float t = float(days - prices[i - 1].first) / (prices[i].first - prices[i - 1].first);
			return Lerp(prices[i - 1].second, prices[i].second, t);
		}
	}

	return prices[countof(prices) - 1].second;
}
