// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "UnitStats.h"
#include "ItemSlot.h"

//-----------------------------------------------------------------------------
struct StatProfile
{
	struct Subprofile
	{
		string id;
		int weapon_chance[WT_MAX], weapon_total, armor_chance[3], armor_total;
		SkillId tag_skills;
		ITEM_TYPE priorities[SLOT_MAX];
	};

	string id;
	int attrib[(int)AttributeId::MAX];
	int skill[(int)SkillId::MAX];
	vector<Subprofile*> subprofiles;

	~StatProfile() { DeleteElements(subprofiles); }
	bool operator != (const StatProfile& p) const;

	void Set(int level, int* attribs, int* skills) const;
	void Set(int level, UnitStats& stats) const { Set(level, stats.attrib, stats.skill); }
	void SetForNew(int level, int* attribs, int* skills) const;
	void SetForNew(int level, UnitStats& stats) const { SetForNew(level, stats.attrib, stats.skill); }

	static vector<StatProfile*> profiles;
	static StatProfile* TryGet(Cstring id);
};
