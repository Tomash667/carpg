// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"
#include "Perk.h"

//-----------------------------------------------------------------------------
struct UnitData;

//-----------------------------------------------------------------------------
struct SubProfile
{
	struct SkillEntry
	{
		Skill skill;
		float value;
		bool tag;
	};

	struct PerkPoint
	{
		Perk perk;
		int value;
	};

	string id;
	vector<Skill> weapons, armors;
	uint variants;
	vector<SkillEntry> skills;
	vector<PerkPoint> perks;
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
	UnitData* unit_data;
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
	static StatProfile* Get(const AnyString& id)
	{
		auto profile = TryGet(id);
		assert(profile);
		return profile;
	}
};
