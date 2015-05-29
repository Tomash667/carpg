// przedmiot
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"
#include "Material.h"
#include "DamageTypes.h"
#include "ItemType.h"

//-----------------------------------------------------------------------------
// Flagi przedmiotu
enum ITEM_FLAGS
{
	ITEM_NOT_CHEST = 1<<0,
	ITEM_NOT_SHOP = 1<<1,
	ITEM_NOT_ALCHEMIST = 1<<2,
	ITEM_QUEST = 1<<3,
	ITEM_NOT_BLACKSMITH = 1<<4,
	ITEM_MAGE = 1<<5,
	ITEM_DONT_DROP = 1<<6, // nie mo¿na wyrzuciæ w czasie dialogu
	ITEM_SECRET = 1<<7,
	ITEM_BACKSTAB = 1<<8,
	ITEM_POWER_1 = 1<<9,
	ITEM_POWER_2 = 1<<10,
	ITEM_POWER_3 = 1<<11,
	ITEM_POWER_4 = 1<<12,
	ITEM_MAGIC_RESISTANCE_10 = 1<<13,
	ITEM_MAGIC_RESISTANCE_25 = 1<<14,
	ITEM_GROUND_MESH = 1<<15, // gdy le¿y na ziemi u¿ywa modelu
	ITEM_CRYSTAL_SOUND = 1<<16,
	ITEM_IMPORTANT = 1<<17, // rysowany na mapie na z³oto
	ITEM_TEX_ONLY = 1<<18,
	ITEM_NOT_MERCHANT = 1<<19,
	ITEM_NOT_RANDOM = 1<<20,
};

//-----------------------------------------------------------------------------
struct Armor;
struct Bow;
struct Consumeable;
struct Shield;
struct Weapon;
struct OtherItem;

//-----------------------------------------------------------------------------
// Bazowy typ przedmiotu
struct Item
{
	Item() {}
	Item(cstring id, cstring mesh, int weight, int value, ITEM_TYPE type, int flags, int level=0) :
		id(id), mesh(mesh), weight(weight), value(value), type(type), ani(NULL), tex(NULL), flags(flags), level(level), refid(-1)
	{
	}

	inline const Armor& ToArmor() const
	{
		assert(type == IT_ARMOR);
		return *(const Armor*)this;
	}
	inline const Bow& ToBow() const
	{
		assert(type == IT_BOW);
		return *(const Bow*)this;
	}
	inline const Consumeable& ToConsumeable() const
	{
		assert(type == IT_CONSUMEABLE);
		return *(const Consumeable*)this;
	}
	inline const Shield& ToShield() const
	{
		assert(type == IT_SHIELD);
		return *(const Shield*)this;
	}
	inline const Weapon& ToWeapon() const
	{
		assert(type == IT_WEAPON);
		return *(const Weapon*)this;
	}
	inline const OtherItem& ToOther() const
	{
		assert(type == IT_OTHER);
		return *(const OtherItem*)this;
	}

	inline const Armor* ToArmorPtr() const
	{
		assert(type == IT_ARMOR);
		return (const Armor*)this;
	}

	inline float GetWeight() const
	{
		return float(weight)/10;
	}
	inline bool IsStackable() const
	{
		return type == IT_CONSUMEABLE || type == IT_GOLD || (type == IT_OTHER && !IS_SET(flags, ITEM_QUEST));
	}
	inline bool CanBeGenerated() const
	{
		return !IS_SET(flags, ITEM_NOT_RANDOM);
	}
	inline bool IsWearable() const
	{
		return type == IT_WEAPON || type == IT_ARMOR || type == IT_BOW || type == IT_SHIELD;
	}
	inline bool IsWearableByHuman() const;
	inline bool IsQuest() const
	{
		return IS_SET(flags, ITEM_QUEST);
	}

	inline int GetMagicPower() const
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

	inline float GetWeightValue() const
	{
		return float(value)/weight;
	}

	cstring id, mesh;
	string name, desc;
	int weight, value, flags, level, refid;
	ITEM_TYPE type;
	Animesh* ani;
	TEX tex;

	void Export(std::ofstream& out);
};

//-----------------------------------------------------------------------------
// Typ broni
enum WEAPON_TYPE
{
	WT_SHORT,
	WT_LONG,
	WT_MACE,
	WT_AXE
};

//-----------------------------------------------------------------------------
// Opis typu broni
struct WeaponTypeInfo
{
	cstring name;
	float str2dmg, dex2dmg, power_speed, base_speed, dex_speed;
};
extern WeaponTypeInfo weapon_type_info[];

//-----------------------------------------------------------------------------
// Broñ
struct Weapon : public Item
{
	Weapon(cstring id, int weigth, int value, cstring mesh, int dmg, int sila, WEAPON_TYPE wt, MATERIAL_TYPE mat, int dmg_type, int flags, int level) :
		Item(id, mesh, weigth, value, IT_WEAPON, flags, level),
		dmg(dmg), weapon_type(wt), material(mat), dmg_type(dmg_type), sila(sila)
	{
	}

	inline const WeaponTypeInfo& GetInfo() const
	{
		return weapon_type_info[weapon_type];
	}
	
	int dmg, dmg_type, sila;
	WEAPON_TYPE weapon_type;
	MATERIAL_TYPE material;

	void Export(std::ofstream& out);
};
extern Weapon g_weapons[];
extern const uint n_weapons;

//-----------------------------------------------------------------------------
// £uk
struct Bow : public Item
{
	Bow(cstring id, int weigth, int value, cstring mesh, int dmg, int sila, int flags, int level) :
		Item(id, mesh, weigth, value, IT_BOW, flags, level),
		dmg(dmg), sila(sila)
	{
	}

	int dmg, sila;

	void Export(std::ofstream& out);
};
extern Bow g_bows[];
extern const uint n_bows;

//-----------------------------------------------------------------------------
// Tarcza
struct Shield : public Item
{
	Shield(cstring id, int weight, int value, cstring mesh, int def, MATERIAL_TYPE mat, int sila, int flags, int level) :
		Item(id, mesh, weight, value, IT_SHIELD, flags, level),
		def(def), material(mat), sila(sila)
	{
	}

	int def, sila;
	MATERIAL_TYPE material;

	void Export(std::ofstream& out);
};
extern Shield g_shields[];
extern const uint n_shields;

//-----------------------------------------------------------------------------
// Rodzaj pancerza
enum ARMOR_TYPE
{
	A_LIGHT,
	A_HEAVY,
	A_MONSTER_LIGHT,
	A_MONSTER_HEAVY
};
extern cstring armor_type_string[];

//-----------------------------------------------------------------------------
// Pancerz
struct Armor : public Item
{
	Armor(cstring id, int weight, int value, cstring mesh, ARMOR_TYPE armor_type, MATERIAL_TYPE mat, int def, int sila, int zrecznosc, int flags, int level) :
		Item(id, mesh, weight, value, IT_ARMOR, flags, level),
		armor_type(armor_type), material(mat), def(def), sila(sila), zrecznosc(zrecznosc)
	{
	}

	int def, sila, zrecznosc;
	MATERIAL_TYPE material;
	ARMOR_TYPE armor_type;

	inline bool IsNormal() const
	{
		return armor_type == A_LIGHT || armor_type == A_HEAVY;
	}
	inline bool IsHeavy() const
	{
		return armor_type == A_HEAVY || armor_type == A_MONSTER_HEAVY;
	}
	inline Skill GetSkill() const
	{
		if(IsHeavy())
			return Skill::HEAVY_ARMOR;
		else
			return Skill::LIGHT_ARMOR;
	}

	void Export(std::ofstream& out);
};
extern Armor g_armors[];
extern const uint n_armors;

//-----------------------------------------------------------------------------
// Czy przedmiot mo¿e byæ u¿ywany przez cz³owieka
inline bool Item::IsWearableByHuman() const
{
	if(type == IT_ARMOR)
		return ToArmor().IsNormal();
	else
		return type == IT_WEAPON || type == IT_BOW || type == IT_SHIELD;
}

//-----------------------------------------------------------------------------
// Efekty miksturek
enum ConsumeEffect
{
	E_NONE, // brak efektu
	E_HEAL, // natychmiast leczy X hp
	E_REGENERATE, // leczy X hp co sekundê przez Y czasu (nie stackuje siê)
	E_NATURAL, // przyœpiesza naturaln¹ regeneracjê przez Y dni; EndEffects/UpdateEffects nie obs³uguje ró¿nych wartoœci!
	E_ANTIDOTE, // leczy truciznê i alkohol
	E_POISON, // zadaje X obra¿eñ co sekundê przez Y czasu
	E_ALCOHOL, // zwiêksza iloœæ alkoholu o X w ci¹gu Y sekund
	E_STR, // na sta³e zwiêksza si³ê
	E_END, // na sta³e zwiêksza kondycjê
	E_DEX, // na sta³e zwiêksza zrêcznoœæ
	E_ANTIMAGIC, // daje 50% odpornoœci na czary przez Y sekund
	E_FOOD, // leczy 1 hp na sekundê przez Y sekund
};

//-----------------------------------------------------------------------------
// Jadalny przedmiot (jedzenie, napój, miksturka)
enum ConsumeableType
{
	Food,
	Drink,
	Potion
};
struct Consumeable : public Item
{
	Consumeable(cstring id, ConsumeableType cons_type, int weight, int value, cstring mesh, ConsumeEffect effect, float power, float time, int flags) :
		Item(id, mesh, weight, value, IT_CONSUMEABLE, flags),
		effect(effect), power(power), time(time), cons_type(cons_type)
	{
	}

	inline bool IsHealingPotion() const
	{
		return effect == E_HEAL && cons_type == Potion;
	}
	
	ConsumeEffect effect;
	float power, time;
	ConsumeableType cons_type;

	void Export(std::ofstream& out);
};
extern Consumeable g_consumeables[];
extern const uint n_consumeables;

//-----------------------------------------------------------------------------
// List (nieu¿ywane)
// struct Letter : public Item
// {
// 	Letter(cstring id, cstring name, int weight, int _value, cstring _mesh, cstring _text, cstring _desc) :
// 		Item(_id, _name, _mesh, _weight, _value, _desc, IT_LETTER, 0),
// 		text(_text)
// 	{
// 	}
// 
// 	string text;
// };

//-----------------------------------------------------------------------------
// Pozosta³e przedmioty
// kamienie szlachetne, skradzione przedmioty questowe
enum OtherType
{
	Tool,
	ValueableStone,
	OtherItems
};
struct OtherItem : public Item
{
	OtherItem() {}
	OtherItem(cstring id, OtherType other_type, cstring mesh, int weight, int value, int flags) :
		Item(id, mesh, weight, value, IT_OTHER, flags),
		other_type(other_type)
	{
	}

	OtherType other_type;

	void Export(std::ofstream& out);
};
extern OtherItem g_others[];
extern const uint n_others;

//-----------------------------------------------------------------------------
inline bool ItemCmp(const Item* a, const Item* b)
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
			ARMOR_TYPE a1 = a->ToArmor().armor_type,
				a2 = b->ToArmor().armor_type;
			if(a1 != a2)
				return a1 < a2;
		}
		else if(a->type == IT_CONSUMEABLE)
		{
			ConsumeableType c1 = a->ToConsumeable().cons_type,
				c2 = b->ToConsumeable().cons_type;
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

//-----------------------------------------------------------------------------
// Listy przedmiotów
struct ItemEntry
{
	cstring name;
	const Item* item;
};

struct ItemList
{
	cstring name;
	ItemEntry* items;
	uint count;

	inline const Item* Get() const
	{
		return items[rand2()%count].item;
	}
};

extern ItemList g_item_lists[];
extern const uint n_item_lists;

void SetItemLists();

//-----------------------------------------------------------------------------
const Item* FindItem(cstring name, bool report=true, const ItemList** lis=NULL);
const ItemList* FindItemList(cstring name, bool report=true);
Item* CreateItemCopy(const Item* item);
void ExportItems();
