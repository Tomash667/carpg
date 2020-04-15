// item types
#pragma once

//-----------------------------------------------------------------------------
#include "Material.h"
#include "DamageTypes.h"
#include "ItemType.h"
#include "ArmorUnitType.h"
#include "Effect.h"
#include "Skill.h"


struct Item;

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
	ITEM_MAGE = 1 << 5, // equipped by mages, allow casting spells
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
	ITEM_MAGIC_SCROLL = 1 << 17,
	ITEM_WAND = 1 << 18, // cast magic bolts instead of attacking
	ITEM_INGREDIENT = 1 << 19, // shows in crafting panel
};

/**
 * Enum flags for marking what properties an Item has.
 * When adding a new ItemProp, add an entry here
 */
enum ITEM_PROP
{
	IT_PROP_NONE = 0,
	IT_PROP_WEAPON = 1 << 0,
	IT_PROP_BOW = 1 << 1,
	IT_PROP_SHIELD = 1 << 2,
	IT_PROP_ARMOR = 1 << 3,
	IT_PROP_AMULET = 1 << 4,
	IT_PROP_RING = 1 << 5,
	IT_PROP_CONSUMABLE = 1 << 6,
	IT_PROP_OTHER_ITEM = 1 << 7,
	IT_PROP_BOOK = 1 << 8
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

/**
 * Some Item property
 * Every Item property has to inherit from this.
 * Every subclass is required to have:
 * - static variable ITEM_TYPE type with unique value
 * - static variable std::map<Item*, PropertyType*> storage
 * 
 * NOTE: Do no forget to clean up the storage var for each property type
 * 
 * WARNING: Item pointer is stored in property for compatibility reasons only! Do not use it in new code!
 */
class ItemProp
{
protected:
	friend Item;
	Item* item;
	ItemProp() : item(nullptr) {}
public:
	~ItemProp() {}

	/**
	 * @deprecated This function is for compatibility reasons only and can be removed in future
	 */
	const Item& GetItem() const { assert(item); return *item; }

	/**
	 * @deprecated This function is for compatibility reasons only and can be removed in future
	 */
	Item& GetItem() { assert(item); return *item; }
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
struct Weapon : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_WEAPON;
	Weapon() : dmg(10), dmg_type(DMG_BLUNT), req_str(10), weapon_type(WT_BLUNT), material(MAT_WOOD) {}
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
	static std::map<Item*, Weapon*> storage;
};

//-----------------------------------------------------------------------------
// Bow
struct Bow : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_BOW;
	Bow() : dmg(10), req_str(10), speed(45) {}

	int dmg, req_str, speed;

	static vector<Bow*> bows;
	static std::map<Item*, Bow*> storage;
};

//-----------------------------------------------------------------------------
// Shield
struct Shield : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_SHIELD;
	Shield() : block(10), req_str(10), material(MAT_WOOD) {}

	int block, req_str;
	MATERIAL_TYPE material;

	static vector<Shield*> shields;
	static std::map<Item*, Shield*> storage;
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
struct Armor : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_ARMOR;
	Armor() : def(10), req_str(10), mobility(100), material(MAT_SKIN), armor_type(AT_LIGHT), armor_unit_type(ArmorUnitType::HUMAN) {}

	const TexOverride* GetTextureOverride() const
	{
		if(tex_override.empty())
			return nullptr;
		else
			return tex_override.data();
	}
	SkillId GetSkill() const { return GetArmorTypeSkill(armor_type); }

	int def, req_str, mobility;
	MATERIAL_TYPE material;
	ARMOR_TYPE armor_type;
	ArmorUnitType armor_unit_type;
	vector<TexOverride> tex_override;

	static vector<Armor*> armors;
	static std::map<Item*, Armor*> storage;
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
struct Amulet : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_AMULET;
	Amulet() : tag() {}

	ItemTag tag[MAX_ITEM_TAGS];

	static vector<Amulet*> amulets;
	static std::map<Item*, Amulet*> storage;
};

//-----------------------------------------------------------------------------
// Ring
struct Ring : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_RING;
	Ring() : tag() {}

	ItemTag tag[MAX_ITEM_TAGS];

	static vector<Ring*> rings;
	static std::map<Item*, Ring*> storage;
};

//-----------------------------------------------------------------------------
// Eatible item (food, drink, potion)
enum class ConsumableType
{
	Food,
	Drink,
	Herb,
	Potion
};
enum class ConsumableAiType
{
	None,
	Healing,
	Mana
};
struct Consumable : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_CONSUMABLE;
	Consumable() : time(0), cons_type(ConsumableType::Drink), ai_type(ConsumableAiType::None) {}

	float time;
	ConsumableType cons_type;
	ConsumableAiType ai_type;

	static vector<Consumable*> consumables;
	static std::map<Item*, Consumable*> storage;
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
struct OtherItem : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_OTHER_ITEM;
	OtherItem() : other_type(OtherItems) {}
	~OtherItem() {}

	OtherType other_type;

	static vector<OtherItem*> others;
	static vector<OtherItem*> artifacts;
	static std::map<Item*, OtherItem*> storage;
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

struct Book : public ItemProp
{
	const static ITEM_PROP type = IT_PROP_BOOK;
	Book() : scheme(nullptr), runic(false) {}

	BookScheme* scheme;
	vector<string> recipe_ids;
	vector<Recipe*> recipes;
	string text;
	bool runic;

	static vector<Book*> books;
	static std::map<Item*, Book*> storage;
	// get random book for shelf (can be nullptr)
	static const Item* GetRandom();
};

/**
 * Basic item type with settable properties.
 * 
 * Each Item may have one or more properties set.
 */
struct Item
{
	explicit Item(ITEM_TYPE type) : type(type), weight(1), value(0), ai_value(0), flags(0), mesh(nullptr), tex(nullptr), icon(nullptr),
		state(ResourceState::NotLoaded), properties(IT_PROP_NONE)
	{
	}
	~Item()
	{
	}

	Item& operator = (const Item& i);

	/**
	 * Return true if item has property set, false otherwise
	 */
	template<typename Prop>
	const bool HasProperty() const
	{
		return IsSet(properties, Prop::type);
	}

	/**
	 * Return an Item's property, nullptr if property not set
	 */
	template<typename Prop>
	Prop& GetProperty()
	{
		assert(HasProperty<Prop>());
		return *Prop::storage[this];
	}
	
	/**
	 * Return an Item's property as const, nullptr if property not set
	 */
	template<typename Prop>
	const Prop& GetProperty() const
	{
		assert(HasProperty<Prop>());
		return *(Prop::storage[(Item*)this]);
	}

	/**
	 * Sets some property of this Item and stores it in respective storage
	 * Sets property's Item to this instance
	 */
	template<typename Prop>
	void SetProperty(Prop* p)
	{
		properties = (ITEM_PROP)(properties | Prop::type);
		Prop::storage[this] = p;
		p->item = this;
	}

	/**
	 * Removes Item's property from respective storage and unmarks it on Item.
	 * Fails with assert if property is not set.
	 */
	template<typename Prop>
	void RemoveProperty()
	{
		assert(HasProperty<Prop>());
		properties = (ITEM_PROP)(properties ^ Prop::type);
		DeleteProperty<Prop>();
	}

	/**
	 * Wrappers for retrieveval of various Item's properties.
	 */
	/*@{*/
	Weapon& ToWeapon() { return GetProperty<Weapon>(); }
	Bow& ToBow() { return GetProperty<Bow>(); }
	Shield& ToShield() { return GetProperty<Shield>(); }
	Armor& ToArmor() { return GetProperty<Armor>(); }
	Amulet& ToAmulet() { return GetProperty<Amulet>(); }
	Ring& ToRing() { return GetProperty<Ring>(); }
	Consumable& ToConsumable() { return GetProperty<Consumable>(); }
	OtherItem& ToOther() { return GetProperty<OtherItem>(); }
	Book& ToBook() { return GetProperty<Book>(); }

	const Weapon& ToWeapon() const { return GetProperty<Weapon>(); }
	const Bow& ToBow() const { return GetProperty<Bow>(); }
	const Shield& ToShield() const { return GetProperty<Shield>(); }
	const Armor& ToArmor() const { return GetProperty<Armor>(); }
	const Amulet& ToAmulet() const { return GetProperty<Amulet>(); }
	const Ring& ToRing() const { return GetProperty<Ring>(); }
	const Consumable& ToConsumable() const { return GetProperty<Consumable>(); }
	const OtherItem& ToOther() const { return GetProperty<OtherItem>(); }
	const Book& ToBook() const { return GetProperty<Book>(); }
	/*@}*/

	bool IsStackable() const { return Any(type, IT_CONSUMABLE, IT_GOLD, IT_BOOK) || (type == IT_OTHER && !IsSet(flags, ITEM_QUEST)); }
	bool CanBeGenerated() const { return !IsSet(flags, ITEM_NOT_RANDOM); }
	bool IsWearable() const { return Any(type, IT_WEAPON, IT_BOW, IT_SHIELD, IT_ARMOR, IT_AMULET, IT_RING); }
	bool IsQuest() const { return IsSet(flags, ITEM_QUEST); }
	bool IsQuest(int quest_id) const { return IsQuest() && this->quest_id == quest_id; }

	const string& GetName() const { return name; }
	float GetWeight() const { return float(weight) / 10; }
	float GetWeightValue() const { return float(value) / weight; }
	float GetEffectPower(EffectId effect) const;

	void CreateCopy(Item& item) const;
	Item* CreateCopy() const;
	Item* QuestCopy(Quest* quest, const string& name);
	void Rename(cstring name);
	void RenameS(const string& name) { Rename(name.c_str()); }

	string id, name, desc;
	int weight, value, ai_value, flags, quest_id;
	vector<ItemEffect> effects;
	ITEM_TYPE type;
	MeshPtr mesh;
	TexturePtr tex, icon;
	ResourceState state;

	/**
	 * Bit record of properties this Item has 
	 */
	ITEM_PROP properties;

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
private:
	/**
	 * Removes Item's property from respective storage and deletes it.
	 */
	template<typename Prop>
	void DeleteProperty()
	{
		auto& it = Prop::storage.find(this);
		if(it != Prop::storage.end())
		{
			delete it->second;
			Prop::storage.erase(it);
		}
	}
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
	bool is_leveled, is_priority;

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
};

//-----------------------------------------------------------------------------
struct StartItem
{
	SkillId skill;
	const Item* item;
	int value;
	bool mage;

	StartItem(SkillId skill, const Item* item = nullptr, int value = 0, bool mage = false) : skill(skill), item(item), value(value), mage(mage) {}

	static vector<StartItem> start_items;
	static const Item* GetStartItem(SkillId skill, int value, bool mage);
};

//-----------------------------------------------------------------------------
struct Recipe
{
	int hash;
	string id;
	const Item* result;
	vector<pair<const Item*, uint>> items;
	int skill;

	explicit Recipe() : hash(0), result(nullptr), skill(0) {}

	static std::map<int, Recipe*> hash_recipes;
	static Recipe* Get(int hash);
	static Recipe* TryGet(Cstring id) { return Get(Hash(id)); }
	static Recipe* GetS(const string& id);
};

//-----------------------------------------------------------------------------
bool ItemCmp(const Item* a, const Item* b);
const Item* FindItemOrList(Cstring id, ItemList*& lis);

//-----------------------------------------------------------------------------
extern std::map<const Item*, Item*> better_items;
extern std::map<string, Item*> item_aliases;
