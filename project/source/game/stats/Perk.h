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
	Weakness, // -5 attrib
	Strength, // +5 attrib
	Skilled, // +3 skill points
	SkillFocus, // -5*2 skills, +10 one skill
	Talent, // +5 skill
	AlchemistApprentice, // more potions
	Wealthy, // +1k gold, better items
	VeryWealthy, // +5k gold, better items
	FilthyRich, // +100k gold, better items
	FamilyHeirloom, // good starting item
	Leader, // start with npc

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

//enum class Perk
//{
//	// negative starting perks
//	SlowLerner, // -1 apt to all skills
//	Poor, // less gold, worse items
//	BadBack, // -5 str, less carry
//	ChronicDisease, // -5 end, 50% natural healing
//	Sluggish, // -5 dex, slower
//	Autistic, // -5 cha, worse prices
//	
//	// positive starting perks
//	Wealthy, // +1k gold, better items
//	VeryWealthy, // +5k gold, better items
//	FilthyRich, // +100k gold, better items
//	FamilyHeirloom, // good starting item
//	Skilled, // +3 skill points
//	AlchemistApprentice, // more potions
//	Leader, // start with npc
//	MilitaryTraining, // +50 hp, +5 atk/def
//	SkillFocus, // +5 skill, +2 apt
//
//	// normal perks
//	StrongBack, // +carry
//	StrongerBack, // +carry
//
//	/*
//	CON:
//	Healthy, // (60 end, +50? hp)
//	FastHealing, // 70 end, faster natural regeneration
//	poison resistance
//	regeneration
//	natural armor
//	more hp
//
//	DEX:
//	move speed
//	*/
//
//	Max,
//	None
//};

//-----------------------------------------------------------------------------
struct PerkContext;

//-----------------------------------------------------------------------------
struct PerkInfo
{
	enum Flags
	{
		Flaw = 1 << 0,
		History = 1 << 1,
		Multiple = 1 << 2,
		RequireFormat = 1 << 3
	};
	
	Perk perk_id;
	cstring id;
	string name, desc;
	int flags;
	delegate<void(PerkContext&)> func;

	PerkInfo(Perk perk_id, cstring id, int flags, delegate<void(PerkContext&)> func = nullptr) : perk_id(perk_id), id(id), flags(flags), func(func)
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

	TakenPerk(Perk perk, int value = -1) : perk(perk), value(value)
	{
	}

	void GetDesc(string& s) const;
	void Remove(CreatedCharacter& cc, int index) const;
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
