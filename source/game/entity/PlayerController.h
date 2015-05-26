#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"
#include "UnitStats.h"
#include "ItemSlot.h"

//-----------------------------------------------------------------------------
struct Chest;
struct DialogContext;
struct Useable;

//-----------------------------------------------------------------------------
enum PO_AKCJA
{
	PO_BRAK,
	PO_ZDEJMIJ, // zdejmowanie za³o¿onego przedmiotu, Inventory::lock_index
	PO_ZALOZ, // zak³adanie przedmiotu, Inventory::lock_index
	PO_WYRZUC, // wyrzucanie za³o¿onego przedmiotu, Inventory::lock_index
	PO_WYPIJ, // picie miksturki, Inventory::lock_index
	PO_UZYJ, // u¿ywanie u¿ywalnego obiektu
	PO_SPRZEDAJ, // sprzeda¿ za³o¿onego przedmiotu, Inventory::lock_index
	PO_SCHOWAJ, // chowanie za³o¿onego przedmiotu, Inventory::lock_index
	PO_DAJ // oddawanie za³o¿onego przedmiotu, Inventory::lock_index
};

inline bool PoAkcjaTmpIndex(PO_AKCJA po)
{
	return po == PO_ZALOZ || po == PO_WYPIJ;
}

//-----------------------------------------------------------------------------
#define GAIN_STAT_STR 0
#define GAIN_STAT_END 1
#define GAIN_STAT_DEX 2
#define GAIN_STAT_WEP 3
#define GAIN_STAT_SHI 4
#define GAIN_STAT_BOW 5
#define GAIN_STAT_LAR 6
#define GAIN_STAT_HAR 7
#define GAIN_STAT_MAX 8

//-----------------------------------------------------------------------------
#define STAT_KILLS (1<<0)
#define STAT_DMG_DONE (1<<1)
#define STAT_DMG_TAKEN (1<<2)
#define STAT_KNOCKS (1<<3)
#define STAT_ARENA_FIGHTS (1<<4)

//-----------------------------------------------------------------------------
struct PlayerController : public HeroPlayerCommon
{
	float move_tick, last_dmg, last_dmg_poison, dmgc, poison_dmgc, idle_timer;
	// a - attribute, s - skill
	// *p - x points, *n - x next
	int sp[(int)Skill::MAX], sn[(int)Skill::MAX], ap[(int)Attribute::MAX], an[(int)Attribute::MAX];
	byte klawisz;
	PO_AKCJA po_akcja;
	union
	{
		int po_akcja_idx;
		Useable* po_akcja_useable;
	};
	BRON ostatnia;
	bool godmode, noclip;
	int id, free_days;
	//----------------------
	enum Action
	{
		Action_None,
		Action_LootUnit,
		Action_LootChest,
		Action_Talk,
		Action_Trade,
		Action_ShareItems,
		Action_GiveItems
	} action;
	union
	{
		Unit* action_unit;
		Chest* action_chest;
	};
	// tymczasowa zmienna u¿ywane w AddGold, nie trzeba zapisywaæ
	int gold_get;
	//
	DialogContext* dialog_ctx;
	vector<ItemSlot>* chest_trade; // zale¿ne od action (dla LootUnit,ShareItems,GiveItems ekw jednostki, dla LootChest zawartoœæ skrzyni, dla Trade skrzynia kupca)
	int kills, dmg_done, dmg_taken, knocks, arena_fights, stat_flags;

	//--------------------
	// NOWE
	//int chain;
	//float chain_timer;

	PlayerController() : dialog_ctx(NULL), stat_flags(0) {}
	~PlayerController();

	float CalculateAttack() const;
	void TravelTick();
	void Rest(bool resting);
	void Rest(int days, bool resting);

	void Init(Unit& _unit);
	void Train(Skill s, int ile);
	void Train(Attribute a, int ile);
	void TrainMove(float dt);
	void Update(float dt);
#define USE_WEAPON (1<<0)
#define USE_ARMOR (1<<1)
#define USE_BOW (1<<2)
#define USE_SHIELD (1<<3)
#define USE_WEAPON_ATTRIB_MOD (1<<4)
	float CalculateLevel(int attribs, int skills, int flags);
	void Train2(TrainWhat what, float value, float source_lvl, float precalclvl=0.f);
	int GetRequiredAttributePoints(int level);
	int GetRequiredSkillPoints(int level);

	void Save(HANDLE file);
	void Load(HANDLE file);

	inline bool IsTradingWith(Unit* t) const
	{
		if(action == Action_LootUnit || action == Action_Trade || action == Action_GiveItems || action == Action_ShareItems)
			return action_unit == t;
		else
			return false;
	}

	static inline bool IsTrade(Action a)
	{
		return a == Action_LootChest || a == Action_LootUnit || a == Action_Trade || a == Action_ShareItems || a == Action_GiveItems;
	}
};
