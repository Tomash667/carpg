#pragma once

//-----------------------------------------------------------------------------
enum class Perk
{
	Weakness,
	Strength,
	Skilled,
	SkillFocus,
	Talent,
	CraftingTradition,
	Max
};

//-----------------------------------------------------------------------------
struct TakenPerk
{
	Perk perk;
	int value;

	inline TakenPerk()
	{

	}

	inline TakenPerk(Perk perk, int value = -1) : perk(perk), value(value)
	{

	}
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
		Validate = 1 << 4,
	};

	Perk perk_id;
	cstring id;
	string name, desc;
	int flags;

	inline PerkInfo(Perk perk_id, cstring id, int flags) : perk_id(perk_id), id(id), flags(flags)
	{

	}

	static PerkInfo* Find(const string& id);
};

//-----------------------------------------------------------------------------
extern PerkInfo g_perks[(int)Perk::Max];
