// item types
#pragma once

//-----------------------------------------------------------------------------
#include "Material.h"
#include "DamageTypes.h"
#include "ItemType.h"
#include "ArmorUnitType.h"
#include "Effect.h"
#include "Skill.h"
#include "ContentItem.h"

//-----------------------------------------------------------------------------
static const int HEIRLOOM = -1;

//-----------------------------------------------------------------------------
// Item flags
enum ITEM_FLAGS
{
	ITEM_NOT_SHOP = 1 << 0,
	ITEM_QUEST = 1 << 1,
	ITEM_NOT_BLACKSMITH = 1 << 2, // blacksmith don't sell this
	ITEM_MAGE = 1 << 3, // equipped by mages, allow casting spells
	ITEM_DONT_DROP = 1 << 4, // can't drop when in dialog
	ITEM_GROUND_MESH = 1 << 5, // when on ground is displayed as mesh not as bag
	ITEM_CRYSTAL_SOUND = 1 << 6,
	ITEM_IMPORTANT = 1 << 7, // drawn on map as gold bag in minimap, mark dead units with this item
	ITEM_TEX_ONLY = 1 << 8,
	ITEM_NOT_MERCHANT = 1 << 9, // merchant don't sell this
	ITEM_NOT_RANDOM = 1 << 10, // don't spawn in chest, not randomly given by crazies
	ITEM_HQ = 1 << 11, // high quality item icon
	ITEM_MAGICAL = 1 << 12, // magic quality item icon
	ITEM_UNIQUE = 1 << 13, // unique quality item icon
	ITEM_MAGIC_SCROLL = 1 << 14,
	ITEM_WAND = 1 << 15, // cast magic bolts instead of attacking
	ITEM_INGREDIENT = 1 << 16, // shows in crafting panel
	ITEM_SINGLE_USE = 1 << 17, // mark single use recipes
	ITEM_NOT_TEAM = 1 << 18, // always spawned as non team item
	ITEM_MEAT_SOUND = 1 << 19
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
	bool onAttack;
};

//-----------------------------------------------------------------------------
// Base item type
struct Item
{
	explicit Item(ITEM_TYPE type) : type(type), weight(1), value(0), aiValue(0), flags(0), mesh(nullptr), tex(nullptr), icon(nullptr),
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
	bool IsQuest(int questId) const { return IsQuest() && this->questId == questId; }

	const string& GetName() const { return name; }
	float GetWeight() const { return float(weight) / 10; }
	float GetWeightValue() const { return float(value) / weight; }
	float GetEffectPower(EffectId effect) const;

	void CreateCopy(Item& item) const;
	Item* CreateCopy() const;
	Item* QuestCopy(Quest* quest);
	Item* QuestCopy(Quest* quest, const string& name, const string& desc);
	void Rename(cstring name);
	void RenameS(const string& name) { Rename(name.c_str()); }

	string id, name, desc;
	int weight, value, aiValue, flags, questId;
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
	float str2dmg, dex2dmg, powerSpeed, baseSpeed, dexSpeed;
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
	Weapon() : Item(IT_WEAPON), dmg(10), dmgType(DMG_BLUNT), reqStr(10), weaponType(WT_BLUNT), material(MAT_WOOD) {}

	const WeaponTypeInfo& GetInfo() const
	{
		return WeaponTypeInfo::info[weaponType];
	}
	SkillId GetSkill() const
	{
		return GetInfo().skill;
	}

	int dmg, dmgType, reqStr;
	WEAPON_TYPE weaponType;
	MATERIAL_TYPE material;

	static vector<Weapon*> weapons;
};

//-----------------------------------------------------------------------------
// Bow
struct Bow : public Item
{
	Bow() : Item(IT_BOW), dmg(10), reqStr(10), speed(45) {}

	int dmg, reqStr, speed;

	static vector<Bow*> bows;
};

//-----------------------------------------------------------------------------
// Shield
struct Shield : public Item
{
	Shield() : Item(IT_SHIELD), attackMod(0.5f), block(10), reqStr(10), material(MAT_WOOD) {}

	float attackMod;
	int block, reqStr;
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

inline SkillId GetArmorTypeSkill(ARMOR_TYPE armorType)
{
	switch(armorType)
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
	Armor() : Item(IT_ARMOR), def(10), reqStr(10), mobility(100), material(MAT_SKIN), armorType(AT_LIGHT), armorUnitType(ArmorUnitType::HUMAN) {}

	const TexOverride* GetTextureOverride() const
	{
		if(texOverride.empty())
			return nullptr;
		else
			return texOverride.data();
	}
	SkillId GetSkill() const { return GetArmorTypeSkill(armorType); }

	int def, reqStr, mobility;
	MATERIAL_TYPE material;
	ARMOR_TYPE armorType;
	ArmorUnitType armorUnitType;
	vector<TexOverride> texOverride;

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
	TAG_MANA,
	TAG_CLERIC,
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
struct Consumable : public Item
{
	enum class Subtype
	{
		Food,
		Drink,
		Herb,
		Potion
	};

	enum class AiType
	{
		None,
		Healing,
		Mana
	};

	Consumable() : Item(IT_CONSUMABLE), time(0), subtype(Subtype::Drink), aiType(AiType::None) {}

	float time;
	Subtype subtype;
	AiType aiType;

	static vector<Consumable*> consumables;
};

//-----------------------------------------------------------------------------
// Other items
struct OtherItem : public Item
{
	enum class Subtype
	{
		MiscItem,
		Tool,
		Valuable,
		Ingredient
	};

	OtherItem() : Item(IT_OTHER), subtype(Subtype::MiscItem) {}

	Subtype subtype;

	static vector<OtherItem*> others;
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

	static vector<BookScheme*> bookSchemes;
	static BookScheme* TryGet(Cstring id);
};

struct Book : public Item
{
	enum class Subtype
	{
		NormalBook,
		Recipe
	};

	Book() : Item(IT_BOOK), scheme(nullptr), subtype(Subtype::NormalBook), runic(false) {}

	BookScheme* scheme;
	vector<Recipe*> recipes;
	string text;
	Subtype subtype;
	bool runic;

	static vector<Book*> books;
	// get random book for shelf (can be nullptr)
	static const Item* GetRandom();
};

//-----------------------------------------------------------------------------
// Item lists
struct ItemList
{
	struct Entry
	{
		const Item* item;
		int level;
		int chance;
	};

	string id;
	vector<Entry> items;
	int total;
	bool isLeveled, isPriority;

	const Item* Get() const;
	const Item* GetLeveled(int level) const;
	void Get(int count, const Item** result) const;
	const Item* GetByIndex(int index) const;
	inline uint GetSize() const { return items.size(); }

	static vector<ItemList*> lists;
	static ItemList* TryGet(Cstring id);
	static ItemList& Get(Cstring id)
	{
		ItemList* lis = TryGet(id);
		assert(lis);
		return *lis;
	}
	static const ItemList* GetS(const string& id)
	{
		return TryGet(id);
	}
	static const Item* GetItem(Cstring id)
	{
		return Get(id).Get();
	}
	static const Item* GetItemS(const string& id)
	{
		return Get(id).Get();
	}
};

//-----------------------------------------------------------------------------
struct StartItem
{
	SkillId skill;
	const Item* item;
	int value;
	bool mage;

	StartItem(SkillId skill, const Item* item = nullptr, int value = 0, bool mage = false) : skill(skill), item(item), value(value), mage(mage) {}

	static vector<StartItem> startItems;
	static const Item* GetStartItem(SkillId skill, int value, bool mage);
};

//-----------------------------------------------------------------------------
struct Recipe : public ContentItem<Recipe>
{
	inline static const cstring typeName = "recipe";

	const Item* result;
	vector<pair<const Item*, uint>> ingredients;
	int skill, order;
	bool autolearn, defined;

	explicit Recipe() : result(nullptr), skill(0), autolearn(false) {}

	static Recipe* ForwardGet(const string& id);
};

//-----------------------------------------------------------------------------
bool ItemCmp(const Item* a, const Item* b);
const Item* FindItemOrList(Cstring id, ItemList*& lis);
inline void operator << (GameWriter& f, const Item* item)
{
	if(item != nullptr)
		f.WriteString1(item->id);
	else
		f.Write0();
}
inline void operator >> (GameReader& f, const Item*& item)
{
	const string& id = f.ReadString1();
	if(id.empty())
		item = nullptr;
	else
		item = Item::Get(id);
}

//-----------------------------------------------------------------------------
extern std::map<const Item*, Item*> betterItems;
extern std::map<string, Item*> itemAliases;
