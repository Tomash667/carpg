// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"

//-----------------------------------------------------------------------------
struct SubProfile
{
	struct SkillEntry
	{
		Skill skill;
		float value;
		bool tag;
	};

	string id;
	vector<Skill> weapons, armors;
	uint variants;
	vector<SkillEntry> skills;
};

//-----------------------------------------------------------------------------
struct StatProfile
{
	enum Flags
	{
		F_FIXED = 1 << 0,
		F_DOUBLE_WEAPON_TAG_SKILL = 1 << 1
	};

	string id;
	int attrib[(int)Attribute::MAX];
	int skill[(int)Skill::MAX];
	int flags;
	vector<SubProfile*> subprofiles;

	~StatProfile()
	{
		DeleteElements(subprofiles);
	}

	bool operator != (const StatProfile& p) const;

	SubProfile* TryGetSubprofile(const AnyString& id);
	bool ShouldDoubleSkill(Skill s) const;

	static vector<StatProfile*> profiles;
	static StatProfile* TryGet(const AnyString& id);
};
