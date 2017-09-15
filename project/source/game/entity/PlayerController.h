#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"
#include "UnitStats.h"
#include "ItemSlot.h"
#include "Perk.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
struct Chest;
struct DialogContext;
struct Door;
struct GroundItem;
struct Usable;
struct PlayerInfo;

//-----------------------------------------------------------------------------
enum NextAction
{
	NA_NONE,
	NA_REMOVE, // zdejmowanie za�o�onego przedmiotu, Inventory::lock_index
	NA_EQUIP, // zak�adanie przedmiotu, Inventory::lock_index
	NA_DROP, // wyrzucanie za�o�onego przedmiotu, Inventory::lock_index
	NA_CONSUME, // picie miksturki, Inventory::lock_index
	NA_USE, // u�ywanie u�ywalnego obiektu
	NA_SELL, // sprzeda� za�o�onego przedmiotu, Inventory::lock_index
	NA_PUT, // chowanie za�o�onego przedmiotu, Inventory::lock_index
	NA_GIVE // oddawanie za�o�onego przedmiotu, Inventory::lock_index
};

inline bool PoAkcjaTmpIndex(NextAction po)
{
	return po == NA_EQUIP || po == NA_CONSUME;
}

//-----------------------------------------------------------------------------
#define STAT_KILLS (1<<0)
#define STAT_DMG_DONE (1<<1)
#define STAT_DMG_TAKEN (1<<2)
#define STAT_KNOCKS (1<<3)
#define STAT_ARENA_FIGHTS (1<<4)
#define STAT_MAX 0x1F

//-----------------------------------------------------------------------------
enum class TrainWhat
{
	TakeDamage, // player take damage [damage%, level]
	NaturalHealing, // player heals [damage%, -]
	TakeDamageArmor, // player block damage by armor [damage%, level]

	AttackStart, // player start attack [0]
	AttackNoDamage, // player hit target but deal no damage [0, level]
	AttackHit, // player deal damage with weapon [damage%, level]

	BlockBullet, // player block bullet [damage%, level]
	BlockAttack, // player block hit [damage%, level]
	BashStart, // player start bash [0]
	BashNoDamage, // player bash hit but deal no damage [0, level]
	BashHit, // player deal damage with base [damage%, level]

	BowStart, // player start shooting [0]
	BowNoDamage, // player hit target but deal no damage [0, level]
	BowAttack, // player deal damage with bow [damage%, level]

	Move, // player moved [0]

	Talk, // player talked [0]
	Trade, // player traded items [0]

	Stamina, // player uses stamina [value]
};

inline int GetRequiredAttributePoints(int level)
{
	return 4 * (level + 20)*(level + 25);
}

inline int GetRequiredSkillPoints(int level)
{
	return 3 * (level + 20)*(level + 25);
}

inline float GetBaseSkillMod(int v)
{
	return float(v) / 60;
}

inline float GetBaseAttributeMod(int v)
{
	return float(max(0, v - 50)) / 40;
}

//-----------------------------------------------------------------------------
struct PlayerController : public HeroPlayerCommon
{
	float move_tick, last_dmg, last_dmg_poison, dmgc, poison_dmgc, idle_timer, action_recharge, action_cooldown;
	// a - attribute, s - skill
	// *p - x points, *n - x next
	int sp[(int)Skill::MAX], sn[(int)Skill::MAX], ap[(int)Attribute::MAX], an[(int)Attribute::MAX];
	byte action_key;
	NextAction next_action;
	union
	{
		int next_action_idx;
		Usable* next_action_usable;
	};
	WeaponType ostatnia;
	bool godmode, noclip, is_local, recalculate_level;
	int id, free_days, action_charges;
	//----------------------
	enum Action
	{
		Action_None,
		Action_LootUnit,
		Action_LootChest,
		Action_Talk,
		Action_Trade,
		Action_ShareItems,
		Action_GiveItems,
		Action_LootContainer
	} action;
	union
	{
		Unit* action_unit;
		Chest* action_chest;
		Usable* action_container;
	};
	// tymczasowa zmienna u�ywane w AddGold, nie trzeba zapisywa�
	int gold_get;
	//
	DialogContext* dialog_ctx;
	vector<ItemSlot>* chest_trade; // zale�ne od action (dla LootUnit,ShareItems,GiveItems ekw jednostki, dla LootChest zawarto�� skrzyni, dla Trade skrzynia kupca)
	int kills, dmg_done, dmg_taken, knocks, arena_fights, stat_flags;
	UnitStats base_stats;
	PlayerInfo* player_info;
	StatState attrib_state[(int)Attribute::MAX], skill_state[(int)Skill::MAX];
	vector<TakenPerk> perks;
	vector<Unit*> action_targets;

	PlayerController() : dialog_ctx(nullptr), stat_flags(0), player_info(nullptr), is_local(false), action_recharge(0.f), action_cooldown(0.f), action_charges(0)
	{
	}
	~PlayerController();

	float CalculateAttack() const;
	void TravelTick();
	void Rest(int days, bool resting, bool travel = false);

	void Init(Unit& _unit, bool partial = false);
	void ResetStatState();
	void Update(float dt, bool is_local = true);
	void Train(Skill s, int points);
	void Train(Attribute a, int points);
	void TrainMove(float dt, bool run);
	void Train(TrainWhat what, float value, int level);
	void TrainMod(Attribute a, float points);
	void TrainMod2(Skill s, float points);
	void TrainMod(Skill s, float points);
	void SetRequiredPoints();

	void Save(HANDLE file);
	void Load(HANDLE file);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);

	bool IsTradingWith(Unit* t) const
	{
		if(action == Action_LootUnit || action == Action_Trade || action == Action_GiveItems || action == Action_ShareItems)
			return action_unit == t;
		else
			return false;
	}

	static bool IsTrade(Action a)
	{
		return a == Action_LootChest || a == Action_LootUnit || a == Action_Trade || a == Action_ShareItems || a == Action_GiveItems
			|| a == Action_LootContainer;
	}

	bool IsTrading() const
	{
		return IsTrade(action);
	}

	int GetBase(Attribute a) const
	{
		return base_stats.attrib[(int)a];
	}
	int GetBase(Skill s) const
	{
		return base_stats.skill[(int)s];
	}

	// change base stats, don't modify Unit stats
	void SetBase(Attribute a, int value)
	{
		base_stats.attrib[(int)a] = value;
	}
	void SetBase(Skill s, int value)
	{
		base_stats.skill[(int)s] = value;
	}
	bool IsLocal() const
	{
		return is_local;
	}

	::Action& GetAction();
	bool CanUseAction() const
	{
		return action_charges > 0 && action_cooldown <= 0;
	}
	bool UseActionCharge();
	void RefreshCooldown();
	bool IsHit(Unit* unit) const;
};

//-----------------------------------------------------------------------------
enum BeforePlayer
{
	BP_NONE,
	BP_UNIT,
	BP_CHEST,
	BP_DOOR,
	BP_ITEM,
	BP_USABLE
};

//-----------------------------------------------------------------------------
union BeforePlayerPtr
{
	Unit* unit;
	Chest* chest;
	Door* door;
	GroundItem* item;
	Usable* usable;
	void* any;
};

//-----------------------------------------------------------------------------
struct LocalPlayerData
{
	BeforePlayer before_player;
	BeforePlayerPtr before_player_ptr;
	Unit* selected_unit, *selected_target;
	GroundItem* picking_item;
	Vec3 action_point;
	int picking_item_state;
	float rot_buf, action_rot;
	byte wasted_key;
	bool autowalk, action_ready, action_ok;

	void Reset()
	{
		before_player = BP_NONE;
		before_player_ptr.any = nullptr;
		selected_unit = nullptr;
		selected_target = nullptr;
		picking_item = nullptr;
		picking_item_state = 0;
		rot_buf = 0.f;
		wasted_key = VK_NONE;
		autowalk = false;
		action_ready = false;
	}
};
