#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"
#include "UnitStats.h"
#include "ItemSlot.h"
#include "Perk.h"
#include "KeyStates.h"

//-----------------------------------------------------------------------------
enum NextAction
{
	NA_NONE,
	NA_REMOVE, // unequip item
	NA_EQUIP, // equip item after unequiping old one
	NA_DROP, // drop item after hiding it
	NA_CONSUME, // use consumable
	NA_USE, // use usable
	NA_SELL, // sell equipped item
	NA_PUT, // put equipped item in container
	NA_GIVE // give equipped item
};

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
	Trade, // player traded items [value]

	Stamina, // player uses stamina [value],
	BullsCharge, // trains str
	Dash
};

//-----------------------------------------------------------------------------
enum class TrainMode
{
	Normal,
	Tutorial,
	Potion
};

//-----------------------------------------------------------------------------
inline int GetRequiredAttributePoints(int level)
{
	return 4 * (level + 20)*(level + 25);
}
inline int GetRequiredSkillPoints(int level)
{
	return 3 * (level + 20)*(level + 25);
}

//-----------------------------------------------------------------------------
struct PlayerController : public HeroPlayerCommon
{
	struct StatData
	{
		int points;
		int next;
		int train;
		int apt;
		float train_part;
		bool blocked;
	};

	PlayerInfo* player_info;
	float move_tick, last_dmg, last_dmg_poison, dmgc, poison_dmgc, idle_timer, action_recharge, action_cooldown;
	StatData skill[(int)SkillId::MAX], attrib[(int)AttributeId::MAX];
	byte action_key;
	NextAction next_action;
	union
	{
		ITEM_SLOT slot;
		Usable* usable;
		struct
		{
			const Item* item;
			int index;
		};
	} next_action_data;
	WeaponType last_weapon;
	bool godmode, noclip, is_local, recalculate_level, leaving_event, always_run, last_ring;
	int id, free_days, action_charges, learning_points, exp, exp_need, exp_level;
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
		Action_LootContainer,
		Action_TalkUsable
	} action;
	union
	{
		Unit* action_unit;
		Chest* action_chest;
		Usable* action_usable;
	};
	// tymczasowa zmienna u¿ywane w AddGold, nie trzeba zapisywaæ
	int gold_get;
	//
	DialogContext* dialog_ctx;
	vector<ItemSlot>* chest_trade; // zale¿ne od action (dla LootUnit,ShareItems,GiveItems ekw jednostki, dla LootChest zawartoœæ skrzyni, dla Trade skrzynia kupca)
	int kills, dmg_done, dmg_taken, knocks, arena_fights, stat_flags;
	vector<TakenPerk> perks;
	vector<Unit*> action_targets;

	PlayerController() : dialog_ctx(nullptr), stat_flags(0), player_info(nullptr), is_local(false), action_recharge(0.f),
		action_cooldown(0.f), action_charges(0), last_ring(false)
	{
	}
	~PlayerController();

	void Rest(int days, bool resting, bool travel = false);

	void Init(Unit& _unit, bool partial = false);
	void Update(float dt, bool is_local = true);
private:
	void Train(SkillId s, float points);
	void Train(AttributeId a, float points);
	void TrainMod(AttributeId a, float points);
	void TrainMod2(SkillId s, float points);
	void TrainMod(SkillId s, float points);
public:
	void TrainMove(float dist);
	void Train(TrainWhat what, float value, int level);
	void Train(bool is_skill, int id, TrainMode mode = TrainMode::Normal);
	void SetRequiredPoints();
	int CalculateLevel();
	void RecalculateLevel();
	int GetAptitude(SkillId skill);

	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f);

	bool IsTradingWith(Unit* t) const
	{
		if(Any(action, Action_LootUnit, Action_Trade, Action_GiveItems, Action_ShareItems))
			return action_unit == t;
		else
			return false;
	}

	static bool IsTrade(Action a)
	{
		return Any(a, Action_LootChest, Action_LootUnit, Action_Trade, Action_ShareItems, Action_GiveItems, Action_LootContainer);
	}
	bool IsTrading() const { return IsTrade(action); }
	bool IsLocal() const { return is_local; }
	bool IsLeader() const;
	::Action& GetAction();
	bool CanUseAction() const
	{
		return action_charges > 0 && action_cooldown <= 0;
	}
	bool UseActionCharge();
	void RefreshCooldown();
	bool IsHit(Unit* unit) const;
	int GetNextActionItemIndex() const;
	void AddItemMessage(uint count);
	void PayCredit(int count);
	void UseDays(int count);
	void StartDialog(Unit* talker, GameDialog* dialog = nullptr);

	// perks
	bool HavePerk(Perk perk, int value = -1);
	bool HavePerkS(const string& perk_id);
	bool AddPerk(Perk perk, int value);
	bool RemovePerk(Perk perk, int value);

	void AddLearningPoint(int count = 1);
	void AddExp(int exp);
	int GetExpNeed() const;
	int GetTrainCost(int train) const;
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
	Unit* selected_unit, // unit marked with 'select' command
		*target_unit; // unit in front of player
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
		target_unit = nullptr;
		picking_item = nullptr;
		picking_item_state = 0;
		rot_buf = 0.f;
		wasted_key = VK_NONE;
		autowalk = false;
		action_ready = false;
	}
};
