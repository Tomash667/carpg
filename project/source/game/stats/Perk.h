#pragma once

//-----------------------------------------------------------------------------
struct CreatedCharacter;
struct PlayerController;

//-----------------------------------------------------------------------------
enum class Perk
{
	Weakness, // -5 attrib
	Strength, // +5 attrib
	Skilled, // +2 skill points
	SkillFocus, // -5*2 skills, +10 one skill
	Talent, // +5 skill
	//CraftingTradition,
	AlchemistApprentice, // more potions
	Wealthy, // +500 gold
	VeryWealthy, // +2k gold
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

//-----------------------------------------------------------------------------
struct PerkInfo
{
	enum Flags
	{
		Flaw = 1 << 0,
		History = 1 << 1,
		Free = 1 << 2,
		Multiple = 1 << 3,
		Check = 1 << 4,
		RequireFormat = 1 << 5,
	};

	enum RequiredValue
	{
		None,
		Attribute,
		Skill
	};

	Perk perk_id, required;
	cstring id;
	string name, desc;
	int flags;
	RequiredValue required_value;

	PerkInfo(Perk perk_id, cstring id, int flags, RequiredValue required_value, Perk required = Perk::None) : perk_id(perk_id), id(id), flags(flags),
		required_value(required_value), required(required)
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

	TakenPerk()
	{
	}

	TakenPerk(Perk perk, int value = -1) : perk(perk), value(value)
	{
	}

	void GetDesc(string& s) const;
	int Apply(CreatedCharacter& cc, bool validate = false) const;
	void Apply(PlayerController& pc) const;
	void Remove(CreatedCharacter& cc, int index) const;
	cstring FormatName();

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
