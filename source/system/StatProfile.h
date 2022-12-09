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
		static const float defaultPriorities[IT_MAX_WEARABLE];
		static const float defaultTagPriorities[TAG_MAX];

		string id;
		int weaponChance[WT_MAX], weaponTotal, armorChance[AT_MAX], armorTotal;
		SkillId tagSkills[MAX_TAGS];
		float priorities[IT_MAX_WEARABLE];
		float tagPriorities[TAG_MAX];
		TakenPerk perks[MAX_PERKS];
		ItemScript* itemScript;

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
