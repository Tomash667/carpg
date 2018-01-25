#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"
#include "UnitStats.h"
#include "ItemSlot.h"
#include "Perk.h"
#include "KeyStates.h"
#include "VarsContainer.h"

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
	NA_REMOVE, // zdejmowanie za³o¿onego przedmiotu, Inventory::lock_index
	NA_EQUIP, // zak³adanie przedmiotu, Inventory::lock_index
	NA_DROP, // wyrzucanie za³o¿onego przedmiotu, Inventory::lock_index
	NA_CONSUME, // picie miksturki, Inventory::lock_index
	NA_USE, // u¿ywanie u¿ywalnego obiektu
	NA_SELL, // sprzeda¿ za³o¿onego przedmiotu, Inventory::lock_index
	NA_PUT, // chowanie za³o¿onego przedmiotu, Inventory::lock_index
	NA_GIVE // oddawanie za³o¿onego przedmiotu, Inventory::lock_index
};

inline bool PoAkcjaTmpIndex(NextAction po)
{
	return po == NA_EQUIP || po == NA_CONSUME;
}

//-----------------------------------------------------------------------------
enum PlayerStats
{
	STAT_KILLS = 1 << 0,
	STAT_DMG_DONE = 1 << 1,
	STAT_DMG_TAKEN = 1 << 2,
	STAT_KNOCKS = 1 << 3,
	STAT_ARENA_FIGHTS = 1 << 4,
	STAT_MAX = 0x1F
};

//-----------------------------------------------------------------------------
enum class TrainWhat
{
	Attack, // player starts attack [skill] - str when too low for weapon
	Hit, // player hits target [skill, level, value (0-1)] - str when too low for weapon
	Move, // player moved [value] - str when too low for armor, str when overcarrying
	Block, // player blocked attack [attack blocked] - str when too low for shield
	TakeHit, // player take attack [attack value]
	TakeDamage, // player take damage [damage]
	Regenerate, // player regenerate damage [value]
	Stamina, // player is using stamina [value]
	Trade, // player bought/sell goods [value]
	Read, // player read book
	Dodge, // player dodged attack
};

inline int GetRequiredAttributePoints(int level)
{
	return 4 * (level + 20)*(level + 25);
}

inline int GetRequiredSkillPoints(int level)
{
	return 3 * (level + 15)*(level + 20);
}

namespace old
{
	// pre 0.7
	inline int GetRequiredSkillPoints(int level)
	{
		return 3 * (level + 20)*(level + 25);
	}
}

//-----------------------------------------------------------------------------
struct TrainData
{
	int value;
	int next;
	float part;

	float Percentage() const
	{
		return FLT10(float(value) * 100 / next);
	}
};

//-----------------------------------------------------------------------------
struct PlayerController : public HeroPlayerCommon
{
	PlayerInfo* player_info;
	float move_tick, last_dmg, last_dmg_poison, dmgc, poison_dmgc, idle_timer, action_recharge, action_cooldown, level;
	TrainData attrib_pts[(int)Attribute::MAX], skill_pts[(int)Skill::MAX];
	byte action_key;
	NextAction next_action;
	union
	{
		int next_action_idx;
		Usable* next_action_usable;
	};
	WeaponType ostatnia;
	bool godmode, noclip, is_local, recalculate_level;
	int id, free_days, action_charges, perk_points;
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
	// tymczasowa zmienna u¿ywane w AddGold, nie trzeba zapisywaæ
	int gold_get;
	//
	DialogContext* dialog_ctx;
	vector<ItemSlot>* chest_trade; // zale¿ne od action (dla LootUnit,ShareItems,GiveItems ekw jednostki, dla LootChest zawartoœæ skrzyni, dla Trade skrzynia kupca)
	int kills, dmg_done, dmg_taken, knocks, arena_fights, stat_flags;
	vector<Unit*> action_targets;
	vector<const Item*> book_read;
	ObjectPoolRef<VarsContainer> vars;

	PlayerController() : dialog_ctx(nullptr), stat_flags(0), player_info(nullptr), is_local(false), action_recharge(0.f), action_cooldown(0.f), action_charges(0)
	{
	}
	~PlayerController();

	void Init(Unit& _unit, bool partial = false);
	void OnRest();
	void Update(float dt, bool is_local = true);
	void TrainMove(float dist);
	void Train(TrainWhat what, float value, int level, Skill skill = Skill::NONE);
	enum TrainMode
	{
		TM_NORMAL,
		TM_SINGLE_POINT,
		TM_POTION
	};
	void Train(TrainMode mode, int what, bool is_skill);
private:
	void Train(Attribute attrib, float points);
	void Train(Skill skill, float points, bool train_attrib = true);
public:
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

	void RecalculateLevel(bool initial = false);

	void OnReadBook(int i_index);

	void AddPerkPoint(int count = 1);
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
