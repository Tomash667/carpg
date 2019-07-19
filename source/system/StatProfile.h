// attributes & skill profiles
#pragma once

//-----------------------------------------------------------------------------
#include "ItemSlot.h"
#include "Item.h"
#include "Perk.h"

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

	bool operator < (const SubprofileInfo& s) const
	{
		return value < s.value;
	}
	SkillId GetSkill(SkillId skill) const;
};

//-----------------------------------------------------------------------------
struct StatProfile
{
	static const uint MAX_TAGS = 4;
	static const uint MAX_PERKS = 2;

	struct Subprofile
	{
		static const float default_priorities[IT_MAX_WEARABLE];
		static const float default_tag_priorities[TAG_MAX];

		string id;
		int weapon_chance[WT_MAX], weapon_total, armor_chance[AT_MAX], armor_total;
		SkillId tag_skills[MAX_TAGS];
		float priorities[IT_MAX_WEARABLE];
		float tag_priorities[TAG_MAX];
		TakenPerk perks[MAX_PERKS];
		ItemScript* item_script;

		Subprofile();
	};

	string id;
	int attrib[(int)AttributeId::MAX];
	int skill[(int)SkillId::MAX];
	vector<Subprofile*> subprofiles;

	~StatProfile() { DeleteElements(subprofiles); }
	bool operator != (const StatProfile& p) const;
	Subprofile* GetSubprofile(const string& id);
	SubprofileInfo GetRandomSubprofile(SubprofileInfo* prev = nullptr);

	static vector<StatProfile*> profiles;
	static StatProfile* TryGet(Cstring id);
};
