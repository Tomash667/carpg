// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "ItemSlot.h"
#include "Item.h"

//-----------------------------------------------------------------------------
union SubprofileInfo
{
	struct
	{
		byte level;
		byte index;
		byte weapon;
		byte armor;
	};
	uint value;
};

//-----------------------------------------------------------------------------
struct StatProfile
{
	struct Subprofile
	{
		static const uint MAX_TAGS = 3;

		string id;
		int weapon_chance[WT_MAX], weapon_total, armor_chance[AT_MAX], armor_total;
		SkillId tag_skills[MAX_TAGS];
		ITEM_TYPE priorities[SLOT_MAX];

		Subprofile() : weapon_chance(), weapon_total(0), armor_chance(), armor_total(0), priorities()
		{
			for(int i = 0; i < MAX_TAGS; ++i)
				tag_skills[i] = SkillId::NONE;
		}
	};

	string id;
	int attrib[(int)AttributeId::MAX];
	int skill[(int)SkillId::MAX];
	vector<Subprofile*> subprofiles;

	~StatProfile() { DeleteElements(subprofiles); }
	bool operator != (const StatProfile& p) const;
	Subprofile* GetSubprofile(const string& id);
	SubprofileInfo GetRandomSubprofile();

	static vector<StatProfile*> profiles;
	static StatProfile* TryGet(Cstring id);
};
