#pragma once

enum class Perk
{
	Example,
	Max
};

struct PerkInfo
{
	Perk id;
	cstring name, desc;
};

extern PerkInfo g_perks[(int)Perk::Max];
