#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"
#include "UnitStats.h"
#include "ItemSlot.h"
#include "Perk.h"
#include "Input.h"

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
	NA_GIVE, // give equipped item
	NA_EQUIP_DRAW // equip item and draw it if possible
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
	Persuade, // player used persuasion [value: 0-100 chance]

	Stamina, // player uses stamina [value],
	BullsCharge, // trains str
	Dash,
	CastCleric, // player cast cleric spell [value]
	CastMage, // player cast mage spell [value]
	Mana, // player uses mana [value]
	Craft, // player craft potion [value]
};

//-----------------------------------------------------------------------------
enum class TrainMode
{
	Normal,
	Tutorial,
	Potion
};

//-----------------------------------------------------------------------------
enum class PlayerAction
{
	None,
	LootUnit,
	LootChest,
	Talk,
	Trade,
	ShareItems,
	GiveItems,
	LootContainer,
	TalkUsable
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
inline int GetRequiredAttributePoints(int level)
{
	return 4 * (level + 20) * (level + 25);
}
inline int GetRequiredSkillPoints(int level)
{
	return 3 * (level + 20) * (level + 25);
}

//-----------------------------------------------------------------------------
struct PlayerAbility
{
	Ability* ability;
	float recharge, cooldown;
	int charges;
};

/**
 * Represents recipe known to the player
 */
struct MemorizedRecipe
{
	Recipe* recipe;
};


//-----------------------------------------------------------------------------
struct Shortcut
{
	static const uint MAX = 10;

	enum Type
	{
		TYPE_NONE,
		TYPE_SPECIAL,
		TYPE_ITEM,
		TYPE_ABILITY
	};

	enum Special
	{
		SPECIAL_MELEE_WEAPON,
		SPECIAL_RANGED_WEAPON,
		SPECIAL_ABILITY_OLD, // removed in V_0_13
		SPECIAL_HEALTH_POTION,
		SPECIAL_MANA_POTION
	};

	Type type;
	union
	{
		int value;
		const Item* item;
		Ability* ability;
	};
	bool trigger;
};

//-----------------------------------------------------------------------------
struct LocalPlayerData
{
	union BeforePlayerPtr
	{
		Unit* unit;
		Chest* chest;
		Door* door;
		GroundItem* item;
		Usable* usable;
		void* any;
	};

	BeforePlayer beforePlayer;
	BeforePlayerPtr beforePlayerPtr;
	Entity<Unit> selectedUnit;
	Entity<Unit> abilityTarget;
	Vec3 abilityPoint;
	float rotBuf, abilityRot, grayout, rangeRatio;
	Key wastedKey;
	Ability* abilityReady;
	bool autowalk, abilityOk;

	// called on new game or load
	void Reset()
	{
		beforePlayer = BP_NONE;
		beforePlayerPtr.any = nullptr;
		selectedUnit = nullptr;
		abilityTarget = nullptr;
		rotBuf = 0.f;
		grayout = 0.f;
		wastedKey = Key::None;
		autowalk = false;
		abilityReady = nullptr;
	}
	Unit* GetTargetUnit()
	{
		if(beforePlayer == BP_UNIT)
			return beforePlayerPtr.unit;
		return nullptr;
	}
};

//-----------------------------------------------------------------------------
struct PlayerController : public HeroPlayerCommon
{
	struct StatData
	{
		int points;
		int next;
		int train;
		int apt;
		float trainPart;
	};

	PlayerInfo* playerInfo;
	float moveTick, lastDmg, lastDmgPoison, dmgc, poisonDmgc, idleTimer, buildingUndergroundValue;
	StatData skill[(int)SkillId::MAX], attrib[(int)AttributeId::MAX];
	Key actionKey;
	NextAction nextAction;
	union
	{
		ITEM_SLOT slot;
		Usable* usable;
		struct
		{
			const Item* item;
			int index;
		};
	} nextActionData;
	WeaponType lastWeapon;
	bool godmode, nocd, noclip, invisible, isLocal, recalculateLevel, leavingEvent, alwaysRun, lastRing, buildingUndergroundState;
	int id, freeDays, learningPoints, exp, expNeed, expLevel;
	PlayerAction action;
	union
	{
		Unit* actionUnit;
		Chest* actionChest;
		Usable* actionUsable;
	};
	DialogContext* dialogCtx;
	vector<ItemSlot>* chestTrade; // depends on action (can be unit inventory or chest or trader stock)
	int kills, dmgDone, dmgTaken, knocks, arenaFights, statFlags;
	vector<TakenPerk> perks;
	Shortcut shortcuts[Shortcut::MAX];
	vector<PlayerAbility> abilities;
	vector<MemorizedRecipe> recipes;
	static LocalPlayerData data;

	//----------------------
	// Temporary
	//----------------------
	int goldGet; // used in AddGold

	PlayerController() : dialogCtx(nullptr), statFlags(0), playerInfo(nullptr), isLocal(false), lastRing(false), buildingUndergroundState(false),
		buildingUndergroundValue(0.f) {}
	~PlayerController();

	void Rest(int days, bool resting, bool travel = false);

	void Init(Unit& unit, CreatedCharacter* cc);
private:
	void InitShortcuts();
	void Train(SkillId s, float points);
	void Train(AttributeId a, float points);
	void TrainMod(AttributeId a, float points);
	void TrainMod2(SkillId s, float points);
	void TrainMod(SkillId s, float points);
public:
	void TrainMove(float dist);
	void Train(TrainWhat what, float value, int level);
	void Train(bool isSkill, int id, TrainMode mode = TrainMode::Normal);
	void SetRequiredPoints();
	int CalculateLevel();
	void RecalculateLevel();
	int GetAptitude(SkillId skill);

	void Save(GameWriter& f);
	void Load(GameReader& f);
	void WriteStart(BitStreamWriter& f) const;
	void Write(BitStreamWriter& f) const;
	void ReadStart(BitStreamReader& f);
	bool Read(BitStreamReader& f);

	bool IsTradingWith(Unit* t) const
	{
		if(Any(action, PlayerAction::LootUnit, PlayerAction::Trade, PlayerAction::GiveItems, PlayerAction::ShareItems))
			return actionUnit == t;
		else
			return false;
	}

	static bool IsTrade(PlayerAction a)
	{
		return Any(a, PlayerAction::LootChest, PlayerAction::LootUnit, PlayerAction::Trade,
			PlayerAction::ShareItems, PlayerAction::GiveItems, PlayerAction::LootContainer);
	}
	bool IsTrading() const { return IsTrade(action); }
	bool IsLocal() const { return isLocal; }
	bool IsLeader() const;
	int GetNextActionItemIndex() const;
	void PayCredit(int count);
	void UseDays(int count);
	void StartDialog(Unit* talker, GameDialog* dialog = nullptr, Quest* quest = nullptr);

	// perks
	bool HavePerk(Perk* perk, int value = -1);
	bool HavePerkS(const string& perkId);
	bool AddPerk(Perk* perk, int value);
	bool RemovePerk(Perk* perk, int value);

	// abilities
	bool HaveAbility(Ability* ability) const;
	bool AddAbility(Ability* ability);
	bool RemoveAbility(Ability* ability);
	PlayerAbility* GetAbility(Ability* ability);
	const PlayerAbility* GetAbility(Ability* ability) const;
	enum class CanUseAbilityResult
	{
		Yes,
		No,
		NeedWand,
		TakeWand,
		NeedBow,
		TakeBow
	};
	CanUseAbilityResult CanUseAbility(Ability* ability, bool prepare) const;
	bool CanUseAbilityPreview(Ability* ability) const;
	bool CanUseAbilityCheck() const;
	void UpdateCooldown(float dt);
	void RefreshCooldown();
	void UseAbility(Ability* ability, bool fromServer, const Vec3* pos = nullptr, Unit* target = nullptr);
	bool IsAbilityPrepared() const;

	// recipes
	bool AddRecipe(Recipe* recipe);
	bool HaveRecipe(Recipe* recipe) const;

	void AddLearningPoint(int count = 1);
	void AddExp(int exp);
	int GetExpNeed() const;
	int GetTrainCost(int train) const;
	void Yell();
	void ClearShortcuts();
	void SetShortcut(int index, Shortcut::Type type, int value = 0);
	void CheckObjectDistance(const Vec3& pos, void* ptr, float& bestDist, BeforePlayer type);
	void UseUsable(Usable& useable, bool afterAction);
	void Update(float dt);
	void UpdateMove(float dt, bool allowRot);
	bool WantExitLevel();
	void ClearNextAction();
	Vec3 RaytestTarget(float range);
	bool ShouldUseRaytest() const;
	void ReadBook(int index);
	void CheckBuildingUnderground(bool instant);
};
