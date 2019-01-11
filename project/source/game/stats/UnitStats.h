#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"
#include "StatProfile.h"

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
// Units stats
// This is shared between units of same profile/subprofile.
struct UnitStats
{
	static const int NEW_STAT = -2;

	int attrib[(int)AttributeId::MAX];
	int skill[(int)SkillId::MAX];
	const ITEM_TYPE* priorities;
	SubprofileInfo subprofile;
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

	void Set(StatProfile& stat);
	void SetForNew(StatProfile& stat);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);

	static std::map<std::pair<StatProfile*, SubprofileInfo>, UnitStats*> shared_stats;
};
