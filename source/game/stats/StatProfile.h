// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"

//-----------------------------------------------------------------------------
struct StatProfile
{
	string id;
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

	inline bool operator != (const StatProfile& p) const
	{
		bool result = false;
		if(fixed != p.fixed)
			result = true;
		for(int i = 0; i < (int)Attribute::MAX; ++i)
		{
			if(attrib[i] != p.attrib[i])
			{
				LOG(Format("Attribute %s: %d and %d.", g_attributes[i].id, attrib[i], p.attrib[i]));
				result = true;
			}
		}
		for(int i = 0; i < (int)Skill::MAX; ++i)
		{
			if(skill[i] != p.skill[i])
			{
				LOG(Format("Skill %s: %d and %d.", g_skills[i].id, skill[i], p.skill[i]));
				result = true;
			}
		}
		return result;
	}
};
