#pragma once

//-----------------------------------------------------------------------------
struct CreatedCharacter;
struct PlayerController;

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
	BadBack, // -5 str, less carry
	ChronicDisease, // -5 end, 50% natural healing
	Sluggish, // -5 dex, slower
	SlowLearner, // -5 int, -1 apt to all skills
	Asocial, // -5 cha, worse prices

	// positive starting perks
	Talent, // +5 attrib, +1 apt
	Skilled, // +3 skill points
	SkillFocus, // +5 skill, +1 apt
	AlchemistApprentice, // more potions
	Wealthy, // +1k gold, better items
	VeryWealthy, // +5k gold, better items
	FilthyRich, // +100k gold, better items
	FamilyHeirloom, // good starting item
	Leader, // start with npc
	//	Poor, // less gold, worse items
	//	MilitaryTraining, // +50 hp, +5 atk/def

	// normal perks
	/*
	STR:
	StrongBack, // (60 str, +x kg)

	CON:
	Healthy, // (60 end, +50? hp)
	FastHealing, // 70 end, faster natural regeneration
	poison resistance
	regeneration
	natural armor
	more hp

	DEX:
	move speed
	*/

	Max,
	None
};

//-----------------------------------------------------------------------------
enum class Attribute;
enum class Skill;
struct TakenPerk;

//-----------------------------------------------------------------------------
struct PerkContext
{
	CreatedCharacter* cc;
	PlayerController* pc;
	int index;
	bool validate, startup;

	PerkContext(CreatedCharacter* cc) : cc(cc), pc(nullptr), validate(false), startup(true) {}
	PerkContext(PlayerController* pc) : cc(nullptr), pc(pc), validate(false), startup(true) {}
	bool HavePerk(Perk perk);
	TakenPerk* FindPerk(Perk perk);
	bool CanMod(Attribute attrib);
	bool CanMod(Skill skill);
	void Mod(Attribute attrib, int value, bool mod = true);
	void Mod(Skill skill, int value, bool mod = true);
};

//-----------------------------------------------------------------------------
enum PerkFlags
{
	PF_SLOW_LERNER = 1 << 0, // -1 apt to all skills
	PF_ASOCIAL = 1 << 1, // worse prices
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

	Perk perk_id;
	cstring id;
	string name, desc;
	int flags;
	RequiredValue required_value;

	PerkInfo(Perk perk_id, cstring id, int flags, RequiredValue required_value = RequiredValue::None) : perk_id(perk_id), id(id), flags(flags),
		required_value(required_value)
	{
	}

	static void Validate(uint& err);
	static PerkInfo* Find(const string& id);
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

	static cstring txIncreasedAttrib, txIncreasedSkill, txDecreasedAttrib, txDecreasedSkill, txDecreasedSkills;
};

//-----------------------------------------------------------------------------
extern PerkInfo g_perks[(int)Perk::Max];

//-----------------------------------------------------------------------------
inline bool SortPerks(Perk p1, Perk p2)
{
	return strcoll(g_perks[(int)p1].name.c_str(), g_perks[(int)p2].name.c_str()) < 0;
}
inline bool SortTakenPerks(const std::pair<cstring, int>& tp1, const std::pair<cstring, int>& tp2)
{
	return strcoll(tp1.first, tp2.first) < 0;
}
