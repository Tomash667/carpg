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
	};

	Perk id;
	cstring name, desc;
	int flags;
};

//-----------------------------------------------------------------------------
extern PerkInfo g_perks[(int)Perk::Max];
