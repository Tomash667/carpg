#pragma once

//-----------------------------------------------------------------------------
struct CreatedCharacter;
struct PlayerController;
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
	// negative starting perks
	BadBack, // -5 str, -25% carry
	ChronicDisease, // -5 end, 50% natural healing
	Sluggish, // -5 dex, slower
	SlowLearner, // -5 int, -1 apt to all skills
	Asocial, // -5 cha, worse prices
	Poor, // 10% gold, worse items

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

	// normal perks
	StrongBack, // (60 str) +25% carry
	StrongerBack, // (80 str) [C] +50% carry
	Aggressive, // (60 str) +10 atk
	VeryAggressive, // (80 str) [C] +30 atk
	Berserker, // (100 str) [C] +60 atk
	Careful, // (60 dex) +10 def
	Tought, // (60 end) +100 hp
	Toughter, // (80 end) [C] +250 hp
	Toughtest, // (100 end) [C] +500 hp
	HardSkin, // (60 end) +10 def
	IronSkin, // (80 end) [C] +30 def
	DiamondSkin, // (100 end) [C] +60 def
	ExtraordinaryHealth, // (75 end) +1 hp reg
	PerfectHealth, // (100 end) [C] +5 hp reg

	Strongman, // (50 ath) +25% carry
	BodyBuilder, // (75 ath) +200 hp
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
	PF_ASOCIAL = 1 << 1 // worse prices
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
	PlayerController* pc;
	int index;
	bool validate, startup, reapply;

	PerkContext(CreatedCharacter* cc) : cc(cc), pc(nullptr), validate(false), startup(true), reapply(false) {}
	PerkContext(PlayerController* pc) : cc(nullptr), pc(pc), validate(false), startup(true), reapply(false) {}
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

	TakenPerk(Perk perk, int value = -1) : perk(perk), value(value), hidden(false)
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
