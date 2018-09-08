// item types
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"
#include "Material.h"
#include "DamageTypes.h"
#include "ItemType.h"
#include "ArmorUnitType.h"
#include "Resource.h"
#include "Effect.h"

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
	ITEM_ALPHA = 1 << 25, // item require alpha test
};

//-----------------------------------------------------------------------------
struct CstringHash
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
typedef std::unordered_map<cstring, Item*, CstringHash, CstringEqualComparer> ItemsMap;

//-----------------------------------------------------------------------------
// Base item type
struct Item
{
	explicit Item(ITEM_TYPE type) : type(type), weight(1), value(0), flags(0), mesh(nullptr), tex(nullptr), icon(nullptr), state(ResourceState::NotLoaded)
	{
	}
	virtual ~Item() {}

	Item& operator = (const Item& i);

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
		return type == IT_CONSUMABLE || type == IT_GOLD || (type == IT_OTHER && !IS_SET(flags, ITEM_QUEST)) || type == IT_BOOK;
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

	void CreateCopy(Item& item) const;
	Item* CreateCopy() const;
	void Rename(cstring name);

	string id, mesh_id, name, desc;
	int weight, value, flags, refid;
	ITEM_TYPE type;
	MeshPtr mesh;
	TexturePtr tex;
	TEX icon;
	ResourceState state;

	static ItemsMap items;
	static Item* TryGet(Cstring id);
	static Item* Get(Cstring id)
	{
		auto item = TryGet(id);
		assert(item);
		return item;
	}
	static void Validate(uint& err);
};

//-----------------------------------------------------------------------------
// Weapon types
enum WEAPON_TYPE
{
	WT_SHORT_BLADE,
	WT_LONG_BLADE,
	WT_BLUNT,
	WT_AXE
};

//-----------------------------------------------------------------------------
// Weapon type info
struct WeaponTypeInfo
{
	cstring name;
	float str2dmg, dex2dmg, power_speed, base_speed, dex_speed;
	SkillId skill;
	float stamina;

	static WeaponTypeInfo info[];
};

inline const WeaponTypeInfo& GetWeaponTypeInfo(SkillId s)
{
	switch(s)
	{
	default:
	case SkillId::SHORT_BLADE:
		return WeaponTypeInfo::info[WT_SHORT_BLADE];
	case SkillId::LONG_BLADE:
		return WeaponTypeInfo::info[WT_LONG_BLADE];
	case SkillId::AXE:
		return WeaponTypeInfo::info[WT_AXE];
	case SkillId::BLUNT:
		return WeaponTypeInfo::info[WT_BLUNT];
	}
}

//-----------------------------------------------------------------------------
// Weapon
struct Weapon : public Item
{
	Weapon() : Item(IT_WEAPON), dmg(10), dmg_type(DMG_BLUNT), req_str(10), weapon_type(WT_BLUNT), material(MAT_WOOD) {}

	const WeaponTypeInfo& GetInfo() const
	{
		return WeaponTypeInfo::info[weapon_type];
	}

	int dmg, dmg_type, req_str;
	WEAPON_TYPE weapon_type;
	MATERIAL_TYPE material;

	static vector<Weapon*> weapons;
};

//-----------------------------------------------------------------------------
// Bow
struct Bow : public Item
{
	Bow() : Item(IT_BOW), dmg(10), req_str(10), speed(45) {}

	int dmg, req_str, speed;

	static vector<Bow*> bows;
};

//-----------------------------------------------------------------------------
// Shield
struct Shield : public Item
{
	Shield() : Item(IT_SHIELD), block(10), req_str(10), material(MAT_WOOD) {}

	int block, req_str;
	MATERIAL_TYPE material;

	static vector<Shield*> shields;
};

//-----------------------------------------------------------------------------
// Armor
struct Armor : public Item
{
	Armor() : Item(IT_ARMOR), def(10), req_str(10), mobility(100), material(MAT_SKIN), skill(SkillId::LIGHT_ARMOR), armor_type(ArmorUnitType::HUMAN) {}

	const TexId* GetTextureOverride() const
	{
		if(tex_override.empty())
			return nullptr;
		else
			return &tex_override[0];
	}

	int def, req_str, mobility;
	MATERIAL_TYPE material;
	SkillId skill;
	ArmorUnitType armor_type;
	vector<TexId> tex_override;

	static vector<Armor*> armors;
};

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
	E_STAMINA, // regenerate stamina
	E_STUN, // unit stunned, can't do anything
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
	EffectId ToEffect() const;

	static vector<Consumable*> consumables;
};

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

	static vector<OtherItem*> others;
	static vector<OtherItem*> artifacts;
};

//-----------------------------------------------------------------------------
// Books
struct BookScheme
{
	BookScheme() : tex(nullptr), size(0, 0), prev(0, 0), next(0, 0) {}

	string id;
	TexturePtr tex;
	Int2 size, prev, next;
	vector<Rect> regions;

	static vector<BookScheme*> book_schemes;
	static BookScheme* TryGet(Cstring id);
};

struct Book : public Item
{
	Book() : Item(IT_BOOK), scheme(nullptr), runic(false) {}

	BookScheme* scheme;
	string text;
	bool runic;

	static vector<Book*> books;
};


//-----------------------------------------------------------------------------
// Item lists
struct ItemList
{
	string id;
	vector<const Item*> items;

	const Item* Get() const
	{
		return items[Rand() % items.size()];
	}
	void Get(int count, const Item** result) const;

	static vector<ItemList*> lists;
	static ItemListResult TryGet(Cstring id);
	static ItemListResult Get(Cstring id);
	static const Item* GetItem(Cstring id);
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

	static vector<LeveledItemList*> lists;
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

	ItemListResult()
	{
	}
	ItemListResult(nullptr_t) : lis(nullptr), is_leveled(false)
	{
	}
	ItemListResult(const ItemList* lis) : lis(lis), is_leveled(false), mod(0)
	{
	}
	ItemListResult(const LeveledItemList* llis) : llis(llis), is_leveled(true), mod(0)
	{
	}

	operator bool() const
	{
		return lis != nullptr;
	}

	cstring GetId() const
	{
		return is_leveled ? llis->id.c_str() : lis->id.c_str();
	}

	const string& GetIdString() const
	{
		return is_leveled ? llis->id : lis->id;
	}
};

inline ItemListResult ItemList::Get(Cstring id)
{
	auto result = TryGet(id);
	assert(result.lis != nullptr);
	return result;
}

inline const Item* ItemList::GetItem(Cstring id)
{
	auto result = Get(id);
	assert(!result.is_leveled);
	return result.lis->Get();
}

//-----------------------------------------------------------------------------
struct StartItem
{
	SkillId skill;
	const Item* item;
	int value;

	StartItem(SkillId skill, const Item* item = nullptr, int value = 0) : skill(skill), item(item), value(value) {}

	static vector<StartItem> start_items;
	static const Item* GetStartItem(SkillId skill, int value);
};

//-----------------------------------------------------------------------------
bool ItemCmp(const Item* a, const Item* b);
const Item* FindItemOrList(Cstring id, ItemListResult& lis);

//-----------------------------------------------------------------------------
extern std::map<const Item*, Item*> better_items;
extern std::map<string, Item*> item_aliases;
