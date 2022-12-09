#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"
#include "StatProfile.h"

//-----------------------------------------------------------------------------
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
	int plus, minus;

	StatInfo() : plus(0), minus(0) {}
	void Mod(int value);
	StatState GetState();
};

//-----------------------------------------------------------------------------
// Units stats
// This is shared between units of same profile/subprofile.
struct UnitStats
{
	static const int NEW_STAT = -2;

	int attrib[(int)AttributeId::MAX];
	int skill[(int)SkillId::MAX];
	const float* priorities;
	const float* tagPriorities;
	SubprofileInfo subprofile;
	bool fixed;

	void Save(GameWriter& f) const
	{
		f << attrib;
		f << skill;
	}
	void Load(GameReader& f)
	{
		f >> attrib;
		f >> skill;
	}
	static void Skip(GameReader& f) { f.Skip(sizeof(attrib) + sizeof(skill)); }

	void Set(StatProfile& stat);
	void SetForNew(StatProfile& stat);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);

	static std::map<pair<StatProfile*, SubprofileInfo>, UnitStats*> sharedStats;
};
