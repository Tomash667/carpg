// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"

//-----------------------------------------------------------------------------
enum class StatProfileType
{
	BARBARIAN,
	BARD,
	CLERIC,
	DRUID,
	HUNTER,
	MAGE,
	MONK,
	PALADIN,
	ROGUE,
	WARRIOR,

	BLACKSMITH,
	MERCHANT,
	ALCHEMIST,
	COMMONER,
	CLERK,
	FIGHTER,
	WORKER,

	UNK,
	SHAMAN,
	ORC,
	ORC_BLACKSMITH,
	GOBLIN,
	ZOMBIE,
	SKELETON,
	SKELETON_MAGE,
	GOLEM,
	ANIMAL,

	GORUSH_WARRIOR,
	GORUSH_HUNTER,
	EVIL_BOSS,
	TOMASHU,

	MAX
};

//-----------------------------------------------------------------------------
struct StatProfile
{
	string id;
	StatProfileType type;
	bool fixed;
	int attrib[(int)Attribute::MAX];
	int skill[(int)Skill::MAX];

	void Set(int level, int* attribs, int* skills) const;
	void SetForNew(int level, int* attribs, int* skills) const;

	inline void Set(int level, UnitStats& stats) const
	{
		Set(level, stats.attrib, stats.skill);
	}

	inline void SetForNew(int level, UnitStats& stats) const
	{
		Set(level, stats.attrib, stats.skill);
	}
};

//-----------------------------------------------------------------------------
extern StatProfile g_stat_profiles[(int)StatProfileType::MAX];
