// przedmiot
#include "Pch.h"
#include "Core.h"
#include "Item.h"
#include "Crc.h"
#include "ResourceManager.h"

extern string g_system_dir;
ItemsMap Item::items;
std::map<string, Item*> item_aliases;
vector<ItemList*> ItemList::lists;
vector<LeveledItemList*> LeveledItemList::lists;
vector<Weapon*> Weapon::weapons;
vector<Bow*> Bow::bows;
vector<Shield*> Shield::shields;
vector<Armor*> Armor::armors;
vector<Consumable*> Consumable::consumables;
vector<OtherItem*> OtherItem::others;
vector<OtherItem*> OtherItem::artifacts;
vector<BookScheme*> BookScheme::book_schemes;
vector<Book*> Book::books;
vector<StartItem> StartItem::start_items;
std::map<const Item*, Item*> better_items;

//-----------------------------------------------------------------------------
// adding new types here will require changes in CreatedCharacter::GetStartingItems
WeaponTypeInfo WeaponTypeInfo::info[] = {
	nullptr, 0.25f, 0.78f, 0.4f, 1.1f, 0.002f, Skill::SHORT_BLADE, 40.f, // WT_SHORT
	nullptr, 0.7f, 0.3f, 0.33f, 1.f, 0.0015f, Skill::LONG_BLADE, 50.f, // WT_LONG
	nullptr, 0.85f, 0.15f, 0.29f, 0.9f, 0.00075f, Skill::BLUNT, 60.f, // WT_MACE
	nullptr, 0.8f, 0.2f, 0.31f, 0.95f, 0.001f, Skill::AXE, 60.f, // WT_AXE
};

vector<const Item*> items_to_add;

//=================================================================================================
Item& Item::operator = (const Item& i)
{
	assert(type == i.type);
	mesh_id = i.mesh_id;
	weight = i.weight;
	value = i.value;
	flags = i.flags;
	switch(type)
	{
	case IT_WEAPON:
		{
			auto& w = ToWeapon();
			auto& w2 = i.ToWeapon();
			w.dmg = w2.dmg;
			w.dmg_type = w2.dmg_type;
			w.req_str = w2.req_str;
			w.weapon_type = w2.weapon_type;
			w.material = w2.material;
		}
		break;
	case IT_BOW:
		{
			auto& b = ToBow();
			auto& b2 = i.ToBow();
			b.dmg = b2.dmg;
			b.req_str = b2.req_str;
			b.speed = b2.speed;
		}
		break;
	case IT_SHIELD:
		{
			auto& s = ToShield();
			auto& s2 = i.ToShield();
			s.def = s2.def;
			s.req_str = s2.req_str;
			s.material = s2.material;
		}
		break;
	case IT_ARMOR:
		{
			auto& a = ToArmor();
			auto& a2 = i.ToArmor();
			a.def = a2.def;
			a.req_str = a2.req_str;
			a.mobility = a2.mobility;
			a.material = a2.material;
			a.skill = a2.skill;
			a.armor_type = a2.armor_type;
			a.tex_override = a2.tex_override;
		}
		break;
	case IT_OTHER:
		{
			auto& o = ToOther();
			auto& o2 = i.ToOther();
			o.other_type = o2.other_type;
		}
		break;
	case IT_CONSUMABLE:
		{
			auto& c = ToConsumable();
			auto& c2 = i.ToConsumable();
			c.effect = c2.effect;
			c.power = c2.power;
			c.time = c2.time;
			c.cons_type = c2.cons_type;
		}
		break;
	case IT_BOOK:
		{
			auto& b = ToBook();
			auto& b2 = i.ToBook();
			b.scheme = b2.scheme;
			b.runic = b2.runic;
		}
		break;
	case IT_GOLD:
		break;
	default:
		assert(0);
		break;
	}
	return *this;
}

//=================================================================================================
void ItemList::Get(int count, const Item** result) const
{
	assert(count > 0 && result);

	items_to_add = items;

	int index = 0;
	for(; index < count && !items_to_add.empty(); ++index)
	{
		int items_index = Rand() % items_to_add.size();
		result[index] = items_to_add[items_index];
		RemoveElementIndex(items_to_add, items_index);
	}

	for(; index < count; ++index)
		result[index] = nullptr;

	items_to_add.clear();
}

//=================================================================================================
const Item* LeveledItemList::Get(int level) const
{
	int best_lvl = -1;

	for(const LeveledItemList::Entry& ie : items)
	{
		if(ie.level <= level && ie.level >= best_lvl)
		{
			if(ie.level > best_lvl)
			{
				items_to_add.clear();
				best_lvl = ie.level;
			}
			items_to_add.push_back(ie.item);
		}
	}

	if(!items_to_add.empty())
	{
		const Item* best = items_to_add[Rand() % items_to_add.size()];
		items_to_add.clear();
		return best;
	}

	return nullptr;
}

//=================================================================================================
bool ItemCmp(const Item* a, const Item* b)
{
	assert(a && b);
	if(a->type == b->type)
	{
		if(a->type == IT_WEAPON)
		{
			WEAPON_TYPE w1 = a->ToWeapon().weapon_type,
				w2 = b->ToWeapon().weapon_type;
			if(w1 != w2)
				return w1 < w2;
		}
		else if(a->type == IT_ARMOR)
		{
			ArmorUnitType a1 = a->ToArmor().armor_type,
				a2 = b->ToArmor().armor_type;
			if(a1 != a2)
				return a1 < a2;
			Skill s1 = a->ToArmor().skill,
				s2 = b->ToArmor().skill;
			if(s1 != s2)
				return s1 < s2;
		}
		else if(a->type == IT_CONSUMABLE)
		{
			ConsumableType c1 = a->ToConsumable().cons_type,
				c2 = b->ToConsumable().cons_type;
			if(c1 != c2)
				return c1 > c2;
		}
		else if(a->type == IT_OTHER)
		{
			OtherType o1 = a->ToOther().other_type,
				o2 = b->ToOther().other_type;
			if(o1 != o2)
				return o1 > o2;
		}
		if(a->value != b->value)
			return a->value < b->value;
		else
			return strcoll(a->name.c_str(), b->name.c_str()) < 0;
	}
	else
		return a->type < b->type;
}

//=================================================================================================
void CreateItemCopy(Item& item, const Item* base_item)
{
	switch(base_item->type)
	{
	case IT_OTHER:
		{
			OtherItem& o = (OtherItem&)item;
			const OtherItem& o2 = base_item->ToOther();
			o.mesh = o2.mesh;
			o.desc = o2.desc;
			o.flags = o2.flags;
			o.id = o2.id;
			o.mesh_id = o2.mesh_id;
			o.name = o2.name;
			o.other_type = o2.other_type;
			o.refid = o2.refid;
			o.tex = o2.tex;
			o.type = o2.type;
			o.value = o2.value;
			o.weight = o2.weight;
			o.state = o2.state;
			o.icon = o2.icon;
		}
		break;
	default:
		assert(0); // not implemented
		break;
	}
}

//=================================================================================================
Item* CreateItemCopy(const Item* item)
{
	switch(item->type)
	{
	case IT_OTHER:
		{
			OtherItem* o = new OtherItem;
			CreateItemCopy(*o, item);
			return o;
		}
		break;
	case IT_WEAPON:
	case IT_BOW:
	case IT_SHIELD:
	case IT_ARMOR:
	case IT_CONSUMABLE:
	case IT_GOLD:
	case IT_BOOK:
	default:
		// not implemented yet, YAGNI!
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Item::Validate(uint& err)
{
	for(auto it : Item::items)
	{
		const Item& item = *it.second;

		if(item.name.empty())
		{
			++err;
			Error("Test: Missing item '%s' name.", item.id.c_str());
		}

		if(item.type == IT_BOOK && item.ToBook().text.empty())
		{
			++err;
			Error("Test: Missing book '%s' text.", item.id.c_str());
		}

		if(item.mesh_id.empty())
		{
			++err;
			Error("Test: Missing item '%s' mesh/texture.", item.id.c_str());
		}
	}
}

//=================================================================================================
const Item* StartItem::GetStartItem(Skill skill, int value)
{
	auto it = std::lower_bound(StartItem::start_items.begin(), StartItem::start_items.end(), StartItem(skill),
		[](const StartItem& si1, const StartItem& si2) { return si1.skill > si2.skill; });
	if(it == StartItem::start_items.end())
		return nullptr;
	const Item* best = nullptr;
	int best_value = -2;
	while(true)
	{
		if(it->value == HEIRLOOM)
		{
			if(value == HEIRLOOM)
				return it->item;
		}
		else if(it->value > best_value && it->value <= value)
		{
			best = it->item;
			best_value = it->value;
		}
		++it;
		if(it == StartItem::start_items.end() || it->skill != skill)
			break;
	}
	return best;
}

//=================================================================================================
Item* Item::TryGet(const AnyString& id)
{
	// search item
	auto it = items.find(id);
	if(it != Item::items.end())
		return it->second;

	// search item by old id
	auto it2 = item_aliases.find(id.s);
	if(it2 != item_aliases.end())
		return it2->second;

	return nullptr;
}

//=================================================================================================
BookScheme* BookScheme::TryGet(const AnyString& id)
{
	for(auto scheme : book_schemes)
	{
		if(scheme->id == id)
			return scheme;
	}

	return nullptr;
}

//=================================================================================================
ItemListResult ItemList::TryGet(const AnyString& _id)
{
	ItemListResult result;
	cstring id = _id.s;

	if(id[0] == '-')
	{
		result.mod = -(id[1] - '0');
		id = id + 2;
		assert(InRange(result.mod, -9, -1));
	}
	else
		result.mod = 0;

	for(auto lis : lists)
	{
		if(lis->id == id)
		{
			result.lis = lis;
			result.is_leveled = false;
			return result;
		}
	}

	for(auto lis : LeveledItemList::lists)
	{
		if(lis->id == id)
		{
			result.llis = lis;
			result.is_leveled = true;
			return result;
		}
	}

	return nullptr;
}

//=================================================================================================
const Item* FindItemOrList(const AnyString& id, ItemListResult& lis)
{
	auto item = Item::TryGet(id);
	if(item)
	{
		lis.lis = nullptr;
		return item;
	}

	lis = ItemList::TryGet(id);
	return nullptr;
}
