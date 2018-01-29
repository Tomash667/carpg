#pragma once

//-----------------------------------------------------------------------------
struct CreatedCharacter;
struct Unit;
struct StatsX;
enum class Perk;

//-----------------------------------------------------------------------------
namespace old
{
	enum class Perk
	{
		Weakness, // -5 attrib
		Strength, // +5 attrib
		Skilled, // +2 skill points
		SkillFocus, // -5*2 skills, +10 one skill
		Talent, // +5 skill
		AlchemistApprentice, // more potions
		Wealthy, // +500 gold
		VeryWealthy, // +2k gold
		FamilyHeirloom, // good starting item
		Leader, // start with npc
		Max,
		None
	};
}

//-----------------------------------------------------------------------------
enum class Perk
{
	//=========================================================================
	// negative starting perks
	BadBack, // -5 str, -25% carry
	ChronicDisease, // -5 end, 50% natural healing
	Sluggish, // -5 dex, -25 mobility
	SlowLearner, // -5 int, -1 apt to all skills
	Asocial, // -5 cha, worse prices
	Poor, // 10% gold, worse items
	Unlucky, // 1/2 critical hit chance

	//=========================================================================
	// positive starting perks
	Talent, // +5 attrib, +1 apt
	Skilled, // +3 skill points
	SkillFocus, // +5 skill, +1 apt
	AlchemistApprentice, // more potions
	Wealthy, // +1k gold, better items
	VeryWealthy, // [C] +5k gold, better items
	FilthyRich, // [C] +100k gold, better items
	FamilyHeirloom, // good starting item
	Leader, // start with npc
	MilitaryTraining, // +50 hp, +5 atk/def

	//=========================================================================
	// normal perks
	//=========================================================================
	// strength
	StrongBack, // (60 str) +25% carry
	StrongerBack, // (80 str) [C StrongBack] +50% carry
	Aggressive, // (60 str) +10 atk
	VeryAggressive, // (80 str) [C Aggressive] +30 atk
	Berserker, // (100 str) [C VeryAggressive] +60 atk
	HeavyHitter, // (70 str) +10% power attack
	// dexterity
	Careful, // (60 dex) +10 def
	Mobility, // (60 dex) +10 mobility
	EnhancedMobility, // (80 dex) [C Mobility] +25 mobility
	Finesse, // (75 dex) +5% critical chance
	CriticalFocus, // (90 dex) +50% critical damage
	Dodge, // (100 dex) dodge one attack every 5 seconds
	// endurance
	Tought, // (60 end) +100 hp
	Toughter, // (80 end) [C Tought] +250 hp
	Toughtest, // (100 end) [C Toughtest] +500 hp
	HardSkin, // (60 end) +10 def
	IronSkin, // (80 end) [C HardSkin] +30 def
	DiamondSkin, // (100 end) [C IronSkin] +60 def
	ExtraordinaryHealth, // (75 end) +1 hp reg
	PerfectHealth, // (100 end) [C ExtraordinaryHealth] +5 hp reg
	Energetic, // (55 end) +stamina
	VeryEnergetic, // (85 end) [C Energetic] ++stamina
	Adaptation, // (90 end) poison immunity
	IronMan, // (100 end) 10% protection
	// inteligence
	Educated, // (60 int) more exp
	// charisma
	Leadership, // (60 cha) more exp for heroes
	Charming, // (70 cha) more gold from quests
	// single handed
	SingleHandedWeaponProficiency, // (25 shw) +10 atk
	SingleHandedWeaponExpert, // (50 shw) [C SingleHandedWeaponProficiency] +20 atk, +25% crit damage
	SingleHandedCriticalFocus, // (75 shw) +5% crit
	SingleHandedWeaponMaster, // (100 shw) [C SingleHandedWeaponExpert] +30 atk, +50% crit damage
	// short blade
	ShortBladeProficiency, // (25 shb) +10 atk, +5% crit
	Backstabber, // (50 shb) increased backstab damage, double crit chance (5/10->10/20%)
	ShortBladeExpert, // (75 shb) bleeding
	ShortBladeMaster, // (100 shb) [C ShortBladeProficiency] +25 atk, +10% crit
	// long blade
	LongBladeProficiency, // (25 lob) +15 atk, +5% crit
	DefensiveCombatStyle, // (50 lob) +25 def
	LongBladeExpert, // (75 lob) bleeding
	LongBladeMaster, // (100 lob) [C LongBladeProficiency] +30 atk, +10% crit
	// axe
	AxeProficiency, // (25 axe) +15 atk, +5% crit
	Chopper, // (50 axe) +10% power attack
	AxeExpert, // (75 axe) +50% crit damage
	AxeMaster, // (100 axe) [C AxeProficiency] +40 atk, +10% crit
	// blunt
	BluntProficiency, // (25 blu) +15 atk
	Basher, // (50 blu) critical hits deal extra damage and stun
	BluntExpert, // (75 blu) +30% crit damage
	BluntMaster, // (100 blu) +40 atk, +5% crit
	// bow
	BowProficiency, // (25 bow) +20 atk, +5% crit
	PreciseShot, // (50 bow) zoom when holding, increase accuracy, +15% crit
	BowExpert, // (75 bow) [C BowProficiency] +50 atk, +10% crit, +15% crit dmg
	BowMaster, // (100 bow) shoot two arrows at once, second deal 50% damage if hit same target
	// shield
	ShieldProficiency, // (25 shi) +10% block power, less stamina used
	Shielder, // (50 shi) [C ShieldProficiency] +25% block power, less stamina used
	BattleShielder, // (75 shi) +50% shield damage, can criticaly hit and stun
	MagicShielder, // (100 shi) +50% block power, less stamina used, no penalty for blocking magic attacks
	// light armor
	LightArmorProficiency, // (25 lia) +10 def
	LightArmorMobility, // (50 lia) +25 mobility
	LightArmorExpert, // (75 lia) [C LightArmorProficiency]  +30 def
	LightArmorMaster, // (100 lia) dodge
	// medium armor
	MediumArmorProficiency, // (25 mea) +15 def
	MediumArmorAdjustment, // (50 mea) +50% armor mobility
	MediumArmorExpert, // (75 mea) [C MediumArmorProficiency] +40 def
	MediumArmorMaster, // (100 mea) 5% protection
	// heavy armor
	HeavyArmorProficiency, // (25 hea) +20 def
	HeavyArmorAdjustment, // (50 hea) -50% armor weight, +25% armor mobility
	HeavyArmorExpert, // (75 hea) [C HeavyArmorProficiency] +50 def
	HeavyArmorMaster, // (100 hea) 10% protection
	// athletics
	HealthyDiet, // (25 ath) +50% natural healing, food heal for longer
	Strongman, // (50 ath) +25% carry
	BodyBuilder, // (75 ath) +200 hp
	MiracleDiet, // (100 ath) [C HealthyDiet] +100% natural healing, food instantly heal for more
	// acrobatics
	Sprint, // (25 acro) allows sprint
	Flexible, // (50 acro) +25 mobility
	LongDistanceRunner, // (75 acro) +100 stamina, can longer sprint
	Hyperactive, // (100 acro) faster stamina regen, smaller delay
	// haggle
	TradingContract, // (25 hag) allow allies to use you haggle skill if better (ratio of train skill goes to owner), works with perks below
	ExtraStock, // (50 hag) better/more items in shops
	FreeMerchant, // (75 hag) can sell items anywhere
	MasterMerchant, // (100 hag) buy=sell price
	// literacy
	DecryptingRunes, // (25 lit) allow to use magic scroll

	Max,
	None
};
// saved as byte in mp
static_assert((uint)Perk::None <= 255, "too many Perk");

//-----------------------------------------------------------------------------
enum PerkFlags
{
	PF_SLOW_LERNER = 1 << 0, // -1 apt to all skills
	PF_ASOCIAL = 1 << 1, // worse prices
	PF_HEALTHY_DIET = 1 << 2, // food heal for longer
	PF_MIRACLE_DIET = 1 << 3, // food heal instantly for more
	PF_HEAVY_HITTER = 1 << 4, // +10% power attack
	PF_FINESSE = 1 << 5, // +5% critical chance
	PF_CRITICAL_FOCUS = 1 << 6, // +50% critical damage
	PF_UNLUCKY = 1 << 7, // 1/2 critical hit chance
};

//-----------------------------------------------------------------------------
enum class Attribute;
enum class Skill;
enum class EffectType;
struct TakenPerk;

//-----------------------------------------------------------------------------
struct PerkContext
{
	CreatedCharacter* cc;
	Unit* u;
	StatsX* x;
	int index;
	bool validate, startup, reapply, reapply_statsx;

	PerkContext(CreatedCharacter* cc) : cc(cc), u(nullptr), x(nullptr), validate(false), startup(true), reapply(false), reapply_statsx(false) {}
	PerkContext(Unit* u) : cc(nullptr), u(u), x(nullptr), validate(false), startup(true), reapply(false), reapply_statsx(false) {}
	PerkContext(StatsX* x) : cc(nullptr), u(nullptr), x(x), validate(false), startup(true), reapply(false), reapply_statsx(false) {}
	bool HavePerk(Perk perk);
	TakenPerk* FindPerk(Perk perk);
	TakenPerk* HidePerk(Perk perk, bool hide = true);
	bool CanMod(Attribute attrib);
	bool CanMod(Skill skill);
	void Mod(Attribute attrib, int value, bool mod = true);
	void Mod(Skill skill, int value, bool mod = true);
	bool Have(Attribute attrib, int value);
	bool Have(Skill skill, int value);
	void AddFlag(PerkFlags flag);
	void RemoveFlag(PerkFlags flag);
	void AddEffect(TakenPerk* perk, EffectType effect, float value);
};

//-----------------------------------------------------------------------------
struct PerkInfo
{
	enum Flags
	{
		Flaw = 1 << 0,
		History = 1 << 1,
		RequireFormat = 1 << 2
	};

	enum RequiredValue
	{
		None,
		Attribute,
		Skill
	};

	Perk perk_id, parent;
	cstring id;
	string name, desc;
	int flags;
	RequiredValue required_value;

	PerkInfo(Perk perk_id, cstring id, int flags, Perk parent = Perk::None, RequiredValue required_value = RequiredValue::None) : perk_id(perk_id), id(id), flags(flags),
		parent(parent), required_value(required_value)
	{
	}

	static PerkInfo perks[(int)Perk::Max];
	static void Validate(uint& err);
	static PerkInfo* TryGet(const AnyString& id);
};

//-----------------------------------------------------------------------------
struct TakenPerk
{
	Perk perk;
	int value;
	bool hidden;

	TakenPerk()
	{
	}

	explicit TakenPerk(Perk perk, int value = -1) : perk(perk), value(value), hidden(false)
	{
	}

	void GetDesc(string& s) const;
	cstring FormatName();

	bool Validate();
	bool CanTake(PerkContext& ctx);
	bool Apply(PerkContext& ctx);
	void Remove(PerkContext& ctx);

	static void LoadText();

	static cstring txIncreasedAttrib, txIncreasedSkill, txDecreasedAttrib, txDecreasedSkill, txDecreasedSkills, txPerksRemoved;
};

//-----------------------------------------------------------------------------
inline bool SortPerks(Perk p1, Perk p2)
{
	return strcoll(PerkInfo::perks[(int)p1].name.c_str(), PerkInfo::perks[(int)p2].name.c_str()) < 0;
}
inline bool SortTakenPerks(const std::pair<cstring, int>& tp1, const std::pair<cstring, int>& tp2)
{
	return strcoll(tp1.first, tp2.first) < 0;
}
