// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"

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
	StatProfileType type;
	bool fixed;
	int attrib[(int)Attribute::MAX];
	int skill[(int)Skill::MAX];

	void Set(int level, int* attribs, int* skills);
	void SetForNew(int level, int* attribs, int* skills);
};

//-----------------------------------------------------------------------------
extern StatProfile g_stat_profiles[(int)StatProfileType::MAX];
