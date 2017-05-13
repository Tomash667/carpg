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

	bool operator != (const StatProfile& p) const;

	void Set(int level, int* attribs, int* skills) const;
	void Set(int level, UnitStats& stats) const { Set(level, stats.attrib, stats.skill); }
	void SetForNew(int level, int* attribs, int* skills) const;
	void SetForNew(int level, UnitStats& stats) const { Set(level, stats.attrib, stats.skill); }
};
