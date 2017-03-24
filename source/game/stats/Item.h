// item types
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"
#include "Material.h"
#include "DamageTypes.h"
#include "ItemType.h"
#include "ArmorUnitType.h"
#include "Resource.h"
#include "ItemSlot.h"

//-----------------------------------------------------------------------------
static const int HEIRLOOM = -1;

//-----------------------------------------------------------------------------
// Item flags
enum ITEM_FLAGS
{
	ITEM_NOT_CHEST = 1 << 0,
	ITEM_NOT_SHOP = 1 << 1,
	ITEM_NOT_ALCHEMIST = 1 << 2,
	ITEM_QUEST = 1 << 3,
	ITEM_NOT_BLACKSMITH = 1 << 4,
	ITEM_MAGE = 1 << 5,
	ITEM_DONT_DROP = 1 << 6, // can't drop when in dialog
	ITEM_SECRET = 1 << 7,
	ITEM_BACKSTAB = 1 << 8,
	ITEM_POWER_1 = 1 << 9,
	ITEM_POWER_2 = 1 << 10,
	ITEM_POWER_3 = 1 << 11,
	ITEM_POWER_4 = 1 << 12,
	ITEM_MAGIC_RESISTANCE_10 = 1 << 13,
	ITEM_MAGIC_RESISTANCE_25 = 1 << 14,
	ITEM_GROUND_MESH = 1 << 15, // when on ground is displayed as mesh not as bag
	ITEM_CRYSTAL_SOUND = 1 << 16,
	ITEM_IMPORTANT = 1 << 17, // drawn on map as gold bag in minimap
	ITEM_TEX_ONLY = 1 << 18,
	ITEM_NOT_MERCHANT = 1 << 19,
	ITEM_NOT_RANDOM = 1 << 20,
	ITEM_HQ = 1 << 21, // high quality item icon
	ITEM_MAGICAL = 1 << 23, // magic quality item icon
	ITEM_UNIQUE = 1 << 24, // unique quality item icon
};

//-----------------------------------------------------------------------------
struct Armor;
struct Bow;
struct Consumable;
struct Shield;
struct Weapon;
struct OtherItem;
struct Book;

//-----------------------------------------------------------------------------
// Base item type
struct Item
{
	explicit Item(ITEM_TYPE type) : type(type), weight(1), value(0), flags(0), mesh(nullptr), tex(nullptr)
	{
	}
	virtual ~Item() {}

	template<typename T, ITEM_TYPE _type>
	T& Cast()
	{
		assert(type == _type);
		return *(T*)this;
	}

	template<typename T, ITEM_TYPE _type>
	const T& Cast() const
	{
		assert(type == _type);
		return *(const T*)this;
	}

	Weapon& ToWeapon()
	{
		return Cast<Weapon, IT_WEAPON>();
	}
	Bow& ToBow()
	{
		return Cast<Bow, IT_BOW>();
	}
	Shield& ToShield()
	{
		return Cast<Shield, IT_SHIELD>();
	}
	Armor& ToArmor()
	{
		return Cast<Armor, IT_ARMOR>();
	}
	Consumable& ToConsumable()
	{
		return Cast<Consumable, IT_CONSUMABLE>();
	}
	OtherItem& ToOther()
	{
		return Cast<OtherItem, IT_OTHER>();
	}
	Book& ToBook()
	{
		return Cast<Book, IT_BOOK>();
	}

	const Weapon& ToWeapon() const
	{
		return Cast<Weapon, IT_WEAPON>();
	}
	const Bow& ToBow() const
	{
		return Cast<Bow, IT_BOW>();
	}
	const Shield& ToShield() const
	{
		return Cast<Shield, IT_SHIELD>();
	}
	const Armor& ToArmor() const
	{
		return Cast<Armor, IT_ARMOR>();
	}
	const Consumable& ToConsumable() const
	{
		return Cast<Consumable, IT_CONSUMABLE>();
	}
	const OtherItem& ToOther() const
	{
		return Cast<OtherItem, IT_OTHER>();
	}
	const Book& ToBook() const
	{
		return Cast<Book, IT_BOOK>();
	}

	float GetWeight() const
	{
		return float(weight) / 10;
	}
	bool IsStackable() const
	{
		return type == IT_CONSUMABLE || type == IT_GOLD || (type == IT_OTHER && !IS_SET(flags, ITEM_QUEST));
	}
	bool CanBeGenerated() const
	{
		return !IS_SET(flags, ITEM_NOT_RANDOM);
	}
	bool IsWearable() const
	{
		return type == IT_WEAPON || type == IT_ARMOR || type == IT_BOW || type == IT_SHIELD;
	}
	bool IsWearableByHuman() const;
	bool IsQuest() const
	{
		return IS_SET(flags, ITEM_QUEST);
	}
	bool IsQuest(int quest_refid) const
	{
		return IsQuest() && refid == quest_refid;
	}

	int GetMagicPower() const
	{
		if(IS_SET(flags, ITEM_POWER_1))
			return 1;
		else if(IS_SET(flags, ITEM_POWER_2))
			return 2;
		else if(IS_SET(flags, ITEM_POWER_3))
			return 3;
		else if(IS_SET(flags, ITEM_POWER_4))
			return 4;
		else
			return 0;
	}

	float GetWeightValue() const
	{
		return float(value) / weight;
	}

	static void Validate(uint& err);

	string id, mesh_id, name, desc;
	int weight, value, flags, refid;
	ITEM_TYPE type;
	Animesh* mesh;
	TEX tex;
};

//-----------------------------------------------------------------------------
// Weapon types
enum WEAPON_TYPE
{
	WT_SHORT,
	WT_LONG,
	WT_MACE,
	WT_AXE
};

//-----------------------------------------------------------------------------
// Weapon type info
struct WeaponTypeInfo
{
	cstring name;
	float str2dmg, dex2dmg, power_speed, base_speed, dex_speed;
	Skill skill;
};
extern WeaponTypeInfo weapon_type_info[];

inline const WeaponTypeInfo& GetWeaponTypeInfo(Skill s)
{
	switch(s)
	{
	default:
	case Skill::SHORT_BLADE:
		return weapon_type_info[WT_SHORT];
	case Skill::LONG_BLADE:
		return weapon_type_info[WT_LONG];
	case Skill::AXE:
		return weapon_type_info[WT_AXE];
	case Skill::BLUNT:
		return weapon_type_info[WT_MACE];
	}
}

//-----------------------------------------------------------------------------
// Weapon
struct Weapon : public Item
{
	Weapon() : Item(IT_WEAPON), dmg(10), dmg_type(DMG_BLUNT), req_str(10), weapon_type(WT_MACE), material(MAT_WOOD) {}

	const WeaponTypeInfo& GetInfo() const
	{
		return weapon_type_info[weapon_type];
	}

	int dmg, dmg_type, req_str;
	WEAPON_TYPE weapon_type;
	MATERIAL_TYPE material;
};
extern vector<Weapon*> g_weapons;

//-----------------------------------------------------------------------------
// Bow
struct Bow : public Item
{
	Bow() : Item(IT_BOW), dmg(10), req_str(10), speed(45) {}

	int dmg, req_str, speed;
};
extern vector<Bow*> g_bows;

//-----------------------------------------------------------------------------
// Shield
struct Shield : public Item
{
	Shield() : Item(IT_SHIELD), def(10), req_str(10), material(MAT_WOOD) {}

	int def, req_str;
	MATERIAL_TYPE material;
};
extern vector<Shield*> g_shields;

//-----------------------------------------------------------------------------
// Armor
struct Armor : public Item
{
	Armor() : Item(IT_ARMOR), def(10), req_str(10), mobility(100), material(MAT_SKIN), skill(Skill::LIGHT_ARMOR), armor_type(ArmorUnitType::HUMAN) {}

	const TexId* GetTextureOverride() const
	{
		if(tex_override.empty())
			return nullptr;
		else
			return &tex_override[0];
	}

	int def, req_str, mobility;
	MATERIAL_TYPE material;
	Skill skill;
	ArmorUnitType armor_type;
	vector<TexId> tex_override;
};
extern vector<Armor*> g_armors;

//-----------------------------------------------------------------------------
// Can item can be weared by human?
inline bool Item::IsWearableByHuman() const
{
	if(type == IT_ARMOR)
		return ToArmor().armor_type == ArmorUnitType::HUMAN;
	else
		return type == IT_WEAPON || type == IT_BOW || type == IT_SHIELD;
}

//-----------------------------------------------------------------------------
// Consumable item effects
enum ConsumeEffect
{
	E_NONE, // no effects
	E_HEAL, // heals instantly X hp
	E_REGENERATE, // heals X hp/sec for Y sec (don't stack)
	E_NATURAL, // speed up natural regeneration for Y days (EndEffects/UpdateEffects is hardcoded to use first value when there is multiple effects)
	E_ANTIDOTE, // remove poison and alcohol
	E_POISON, // deal X dmg/sec for Y sec
	E_ALCOHOL, // deals X alcohol points in Y sec
	E_STR, // permanently increase strength
	E_END, // permanently increase endurance
	E_DEX, // permanently increase dexterity
	E_ANTIMAGIC, // gives 50% magic resistance for Y sec
	E_FOOD, // heals 1 hp/sec for Y sec (stack)
	E_GREEN_HAIR, // turn hair into green
};

//-----------------------------------------------------------------------------
// Eatible item (food, drink, potion)
enum ConsumableType
{
	Food,
	Drink,
	Potion
};
struct Consumable : public Item
{
	Consumable() : Item(IT_CONSUMABLE), effect(E_NONE), power(0), time(0), cons_type(Drink) {}

	bool IsHealingPotion() const
	{
		return effect == E_HEAL && cons_type == Potion;
	}

	ConsumeEffect effect;
	float power, time;
	ConsumableType cons_type;
};
extern vector<Consumable*> g_consumables;

//-----------------------------------------------------------------------------
// Other items
// valuable items, tools, quest items
enum OtherType
{
	Tool,
	Valuable,
	OtherItems,
	Artifact
};
struct OtherItem : public Item
{
	OtherItem() : Item(IT_OTHER), other_type(OtherItems) {}

	OtherType other_type;
};
extern vector<OtherItem*> g_others;
extern vector<OtherItem*> g_artifacts;

//-----------------------------------------------------------------------------
// Books
struct BookScheme
{
	BookScheme() : tex(nullptr), size(0, 0), prev(0, 0), next(0, 0) {}

	string id;
	TextureResourcePtr tex;
	INT2 size, prev, next;
	vector<IBOX2D> regions;
};
extern vector<BookScheme*> g_book_schemes;

struct Book : public Item
{
	Book() : Item(IT_BOOK), scheme(nullptr) {}

	BookScheme* scheme;
	string text;
};
extern vector<Book*> g_books;

//-----------------------------------------------------------------------------
// Item lists
struct ItemList
{
	string id;
	vector<const Item*> items;

	const Item* Get() const
	{
		return items[rand2() % items.size()];
	}
	void Get(int count, const Item** result) const;
};

//-----------------------------------------------------------------------------
// Leveled item lists
struct LeveledItemList
{
	struct Entry
	{
		const Item* item;
		int level;
	};

	string id;
	vector<Entry> items;
	
	const Item* Get(int level) const;
};

//-----------------------------------------------------------------------------
struct ItemListResult
{
	union
	{
		const ItemList* lis;
		const LeveledItemList* llis;
	};
	int mod;
	bool is_leveled;

	cstring GetId() const
	{
		return is_leveled ? llis->id.c_str() : lis->id.c_str();
	}

	const string& GetIdString() const
	{
		return is_leveled ? llis->id : lis->id;
	}
};

//-----------------------------------------------------------------------------
enum StockEntry
{
	SE_ADD,
	SE_MULTIPLE,
	SE_ITEM,
	SE_CHANCE,
	SE_RANDOM,
	SE_CITY,
	SE_NOT_CITY,
	SE_ANY_CITY,
	SE_START_SET,
	SE_END_SET,
	SE_LIST,
	SE_LEVELED_LIST,
	SE_SAME_MULTIPLE,
	SE_SAME_RANDOM
};

//-----------------------------------------------------------------------------
struct Stock
{
	string id;
	vector<int> code;
};
extern vector<Stock*> stocks;

Stock* FindStockScript(cstring id);
void ParseStockScript(Stock* stock, int level, bool city, vector<ItemSlot>& items);

//-----------------------------------------------------------------------------
struct StartItem
{
	Skill skill;
	const Item* item;
	int value;

	StartItem(Skill skill, const Item* item = nullptr, int value = 0) : skill(skill), item(item), value(value) {}
};
extern vector<StartItem> start_items;
const Item* GetStartItem(Skill skill, int value);

//-----------------------------------------------------------------------------
bool ItemCmp(const Item* a, const Item* b);
const Item* FindItem(cstring id, bool report = true, ItemListResult* lis = nullptr);
ItemListResult FindItemList(cstring id, bool report = true);
void CreateItemCopy(Item& item, const Item* base_item);
Item* CreateItemCopy(const Item* item);
uint LoadItems(uint& crc, uint& errors);
void CleanupItems();

//-----------------------------------------------------------------------------
struct Hash
{
	size_t operator() (cstring s) const
	{
		size_t hash = 0;
		while(*s)
		{
			hash = hash * 101 + *s++;
		}
		return hash;
	}
};

struct CmpCstring
{
	bool operator () (cstring a, cstring b) const
	{
		return strcmp(a, b) == 0;
	}
};

typedef std::unordered_map<cstring, Item*, Hash, CmpCstring> ItemsMap;

extern ItemsMap g_items;
extern std::map<const Item*, const Item*> better_items;
