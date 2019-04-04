#pragma once

//-----------------------------------------------------------------------------
namespace old
{
	enum class Perk
	{
		Weakness, // -5 attribute
		Strength, // +5 attribute
		Skilled, // +2 sp
		SkillFocus, // -5 to two skills, +10 to one skill
		Talent, // +5 skill
		AlchemistApprentice, // more potions
		Wealthy, // +1k gold
		VeryWealthy, // +2.5k gold
		FamilyHeirloom, // good starting item
		Leader, // start with npc

		Max,
		None
	};
}

//-----------------------------------------------------------------------------
enum class Perk
{
	None = -1,

	// negative starting perks
	BadBack, // -5 str, -25% carry
	ChronicDisease, // -5 end, 50% natural healing
	Sluggish, // -5 dex, -25 mobility
	SlowLearner, // -5 int, -1 apt to all skills
	Asocial, // -5 cha, worse prices
	Poor, // no gold, worse items

	// positive starting perks
	Talent, // +5 attrib, +1 apt
	Skilled, // +3 skill points
	SkillFocus, // +5 skill, +1 apt (only for skills with value >= 0)
	AlchemistApprentice, // more potions
	Wealthy, // +2.5k gold
	VeryWealthy, // [C] +50k gold
	FamilyHeirloom, // good starting item
	Leader, // start with npc

	// normal perks
	StrongBack, // (60 str) +25% carry
	Aggressive, // (60 str) +10 melee attack
	Mobility, // (60 dex) +20 mobility
	Finesse, // (60 dex) +10 ranged attack
	Tough, // (60 end) +hp
	HardSkin, // (60 end) +10 def
	Adaptation, // (75 end) poison resistance
	PerfectHealth, // (100 end) +5 hp reg
	Energetic, // (60 dex/end) +100 stamina

	Max
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

	enum ValueType
	{
		None,
		Attribute,
		Skill
	};

	Perk perk_id;
	cstring id;
	string name, desc;
	int flags, cost;
	ValueType value_type;

	PerkInfo(Perk perk_id, cstring id, int flags, int cost, ValueType value_type = None)
		: perk_id(perk_id), id(id), flags(flags), cost(cost), value_type(value_type)
	{
	}

	static PerkInfo perks[(int)Perk::Max];
	static void Validate(uint& err);
	static PerkInfo* Find(const string& id);
};

//-----------------------------------------------------------------------------
struct PerkContext
{
	CreatedCharacter* cc;
	PlayerController* pc;
	bool startup, validate, check_remove, upgrade;

	PerkContext(CreatedCharacter* cc, PlayerController* pc, bool startup)
		: cc(cc), pc(pc), startup(startup), validate(false), check_remove(false), upgrade(false) {}
	PerkContext(CreatedCharacter* cc) : PerkContext(cc, nullptr, true) {}
	PerkContext(PlayerController* pc, bool startup) : PerkContext(nullptr, pc, startup) {}
	bool CanMod(AttributeId a, bool positive);
	bool HavePerk(Perk perk);
	bool Have(AttributeId a, int value);
	void AddEffect(Perk perk, EffectId effect, float power = 0);
	void RemoveEffect(Perk perk);
	void Mod(AttributeId a, int value, bool mod);
	void Mod(SkillId s, int value, bool mod);
	void AddPerk(Perk perk);
	void RemovePerk(Perk perk);
	void AddRequired(AttributeId a);
	void RemoveRequired(AttributeId a);
};

//-----------------------------------------------------------------------------
struct TakenPerk
{
	Perk perk;
	int value;

	TakenPerk()
	{
	}

	TakenPerk(Perk perk, int value = -1) : perk(perk), value(value)
	{
	}

	void GetDesc(string& s) const;
	void Apply(PerkContext& ctx);
	void Remove(PerkContext& ctx);
	cstring FormatName();
	bool CanTake(PerkContext& ctx);

	static void LoadText();

	static cstring txIncreasedAttrib, txIncreasedSkill, txDecreasedAttrib, txDecreasedSkill;
};

//-----------------------------------------------------------------------------
inline bool SortPerks(Perk p1, Perk p2)
{
	return strcoll(PerkInfo::perks[(int)p1].name.c_str(), PerkInfo::perks[(int)p2].name.c_str()) < 0;
}
inline bool SortTakenPerks(const pair<cstring, int>& tp1, const pair<cstring, int>& tp2)
{
	return strcoll(tp1.first, tp2.first) < 0;
}
