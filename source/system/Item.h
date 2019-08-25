// item types
#pragma once

//-----------------------------------------------------------------------------
#include "Material.h"
#include "DamageTypes.h"
#include "ItemType.h"
#include "ArmorUnitType.h"
#include "Resource.h"
#include "Effect.h"
#include "Skill.h"

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
	ITEM_GROUND_MESH = 1 << 7, // when on ground is displayed as mesh not as bag
	ITEM_CRYSTAL_SOUND = 1 << 8,
	ITEM_IMPORTANT = 1 << 9, // drawn on map as gold bag in minimap, mark dead units with this item
	ITEM_TEX_ONLY = 1 << 10,
	ITEM_NOT_MERCHANT = 1 << 11,
	ITEM_NOT_RANDOM = 1 << 12,
	ITEM_HQ = 1 << 13, // high quality item icon
	ITEM_MAGICAL = 1 << 14, // magic quality item icon
	ITEM_UNIQUE = 1 << 15, // unique quality item icon
	ITEM_ALPHA = 1 << 16, // item require alpha test
	ITEM_MAGIC_SCROLL = 1 << 17
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
struct ItemEffect
{
	EffectId effect;
	float power;
	int value;
	bool on_attack;
};

//-----------------------------------------------------------------------------
// Base item type
struct Item
{
	explicit Item(ITEM_TYPE type) : type(type), weight(1), value(0), ai_value(0), flags(0), mesh(nullptr), tex(nullptr), icon(nullptr),
		state(ResourceState::NotLoaded)
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

	Weapon& ToWeapon() { return Cast<Weapon, IT_WEAPON>(); }
	Bow& ToBow() { return Cast<Bow, IT_BOW>(); }
	Shield& ToShield() { return Cast<Shield, IT_SHIELD>(); }
	Armor& ToArmor() { return Cast<Armor, IT_ARMOR>(); }
	Amulet& ToAmulet() { return Cast<Amulet, IT_AMULET>(); }
	Ring& ToRing() { return Cast<Ring, IT_RING>(); }
	Consumable& ToConsumable() { return Cast<Consumable, IT_CONSUMABLE>(); }
	OtherItem& ToOther() { return Cast<OtherItem, IT_OTHER>(); }
	Book& ToBook() { return Cast<Book, IT_BOOK>(); }

	const Weapon& ToWeapon() const { return Cast<Weapon, IT_WEAPON>(); }
	const Bow& ToBow() const { return Cast<Bow, IT_BOW>(); }
	const Shield& ToShield() const { return Cast<Shield, IT_SHIELD>(); }
	const Armor& ToArmor() const { return Cast<Armor, IT_ARMOR>(); }
	const Amulet& ToAmulet() const { return Cast<Amulet, IT_AMULET>(); }
	const Ring& ToRing() const { return Cast<Ring, IT_RING>(); }
	const Consumable& ToConsumable() const { return Cast<Consumable, IT_CONSUMABLE>(); }
	const OtherItem& ToOther() const { return Cast<OtherItem, IT_OTHER>(); }
	const Book& ToBook() const { return Cast<Book, IT_BOOK>(); }

	bool IsStackable() const { return Any(type, IT_CONSUMABLE, IT_GOLD, IT_BOOK) || (type == IT_OTHER && !IsSet(flags, ITEM_QUEST)); }
	bool CanBeGenerated() const { return !IsSet(flags, ITEM_NOT_RANDOM); }
	bool IsWearable() const { return Any(type, IT_WEAPON, IT_BOW, IT_SHIELD, IT_ARMOR, IT_AMULET, IT_RING); }
	bool IsQuest() const { return IsSet(flags, ITEM_QUEST); }
	bool IsQuest(int quest_id) const { return IsQuest() && this->quest_id == quest_id; }

	float GetWeight() const { return float(weight) / 10; }
	float GetWeightValue() const { return float(value) / weight; }
	float GetEffectPower(EffectId effect) const;

	void CreateCopy(Item& item) const;
	Item* CreateCopy() const;
	Item* QuestCopy(Quest* quest, const string& name);
	void Rename(cstring name);

	string id, mesh_id, name, desc;
	int weight, value, ai_value, flags, quest_id;
	vector<ItemEffect> effects;
	ITEM_TYPE type;
	MeshPtr mesh;
	TexturePtr tex, icon;
	ResourceState state;

	static const Item* gold;
	static ItemsMap items;
	static Item* TryGet(Cstring id);
	static Item* Get(Cstring id)
	{
		auto item = TryGet(id);
		assert(item);
		return item;
	}
	static Item* GetS(const string& id)
	{
		return TryGet(id);
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
	WT_AXE,
	WT_MAX
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

inline WEAPON_TYPE GetWeaponType(SkillId s)
{
	switch(s)
	{
	default:
	case SkillId::SHORT_BLADE:
		return WT_SHORT_BLADE;
	case SkillId::LONG_BLADE:
		return WT_LONG_BLADE;
	case SkillId::AXE:
		return WT_AXE;
	case SkillId::BLUNT:
		return WT_BLUNT;
	}
}

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
	SkillId GetSkill() const
	{
		return GetInfo().skill;
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
enum ARMOR_TYPE
{
	AT_LIGHT,
	AT_MEDIUM,
	AT_HEAVY,
	AT_MAX
};

inline ARMOR_TYPE GetArmorType(SkillId skill)
{
	switch(skill)
	{
	case SkillId::LIGHT_ARMOR:
		return AT_LIGHT;
	case SkillId::MEDIUM_ARMOR:
		return AT_MEDIUM;
	case SkillId::HEAVY_ARMOR:
		return AT_HEAVY;
	default:
		assert(0);
		return AT_MAX;
	}
}

inline SkillId GetArmorTypeSkill(ARMOR_TYPE armor_type)
{
	switch(armor_type)
	{
	default:
		assert(0);
	case AT_LIGHT:
		return SkillId::LIGHT_ARMOR;
	case AT_MEDIUM:
		return SkillId::MEDIUM_ARMOR;
	case AT_HEAVY:
		return SkillId::HEAVY_ARMOR;
	}
}

//-----------------------------------------------------------------------------
// Armor
struct Armor : public Item
{
	Armor() : Item(IT_ARMOR), def(10), req_str(10), mobility(100), material(MAT_SKIN), armor_type(AT_LIGHT), armor_unit_type(ArmorUnitType::HUMAN) {}

	const TexId* GetTextureOverride() const
	{
		if(tex_override.empty())
			return nullptr;
		else
			return &tex_override[0];
	}
	SkillId GetSkill() const { return GetArmorTypeSkill(armor_type); }

	int def, req_str, mobility;
	MATERIAL_TYPE material;
	ARMOR_TYPE armor_type;
	ArmorUnitType armor_unit_type;
	vector<TexId> tex_override;

	static vector<Armor*> armors;
};

//-----------------------------------------------------------------------------
enum ItemTag
{
	TAG_NONE = -1,
	TAG_STR,
	TAG_DEX,
	TAG_MELEE,
	TAG_RANGED,
	TAG_DEF,
	TAG_STAMINA,
	TAG_MAGE,
	TAG_MAX
};
static_assert(TAG_MAX < 32, "Too many ItemTag!");

static const int MAX_ITEM_TAGS = 2;

//-----------------------------------------------------------------------------
// Amulet
struct Amulet : public Item
{
	Amulet() : Item(IT_AMULET), tag() {}

	ItemTag tag[MAX_ITEM_TAGS];

	static vector<Amulet*> amulets;
};

//-----------------------------------------------------------------------------
// Ring
struct Ring : public Item
{
	Ring() : Item(IT_RING), tag() {}

	ItemTag tag[MAX_ITEM_TAGS];

	static vector<Ring*> rings;
};

//-----------------------------------------------------------------------------
// Eatible item (food, drink, potion)
enum ConsumableType
{
	Food,
	Drink,
	Herb,
	Potion
};
struct Consumable : public Item
{
	Consumable() : Item(IT_CONSUMABLE), time(0), cons_type(Drink), is_healing_potion(false) {}

	bool IsHealingPotion() const { return is_healing_potion; }

	float time;
	ConsumableType cons_type;
	bool is_healing_potion;

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
	// get random book for shelf (can be nullptr)
	static const Item* GetRandom();
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
	const Item* GetByIndex(int index) const;
	inline uint GetSize() const { return items.size(); }

	static vector<ItemList*> lists;
	static ItemListResult TryGet(Cstring id);
	static ItemListResult Get(Cstring id);
	static const ItemList* GetS(const string& id);
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

	const Item* Get() const
	{
		return items[Rand() % items.size()].item;
	}
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

inline const ItemList* ItemList::GetS(const string& id)
{
	return TryGet(id).lis;
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
