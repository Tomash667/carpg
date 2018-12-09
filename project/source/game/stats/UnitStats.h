#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
// Typ broni
enum WeaponType
{
	W_NONE,
	W_ONE_HANDED,
	W_BOW
};

//-----------------------------------------------------------------------------
enum class StatState
{
	NORMAL,
	POSITIVE,
	POSITIVE_MIXED,
	MIXED,
	NEGATIVE_MIXED,
	NEGATIVE,
	MAX
};

//-----------------------------------------------------------------------------
struct StatInfo
{
	int value, unmod, base;
	StatState state;
};

//-----------------------------------------------------------------------------
struct UnitStats
{
	int attrib[(int)AttributeId::MAX];
	int skill[(int)SkillId::MAX];
	bool fixed;

	void Save(FileWriter& f) const
	{
		f << attrib;
		f << skill;
	}
	void Load(FileReader& f)
	{
		f >> attrib;
		f >> skill;
	}
	static void Skip(FileReader& f) { f.Skip(sizeof(attrib) + sizeof(skill)); }

	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);

	static std::map<std::pair<StatProfile*, int>, UnitStats*> shared_stats;
};
