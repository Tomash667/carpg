// character skills & gain tables
#include "Pch.h"
#include "GameCore.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
struct StatGain
{
	float value; // skill gain per 5 levels
	int weight; // weight for level calculation
};

//-----------------------------------------------------------------------------
// SkillId gain table. Normalny no class have skill better then 20 but it may happen.
static StatGain gain[] = {
	0.f,	0,	// 0
	1.f,	0,	// 1
	2.f,	0,	// 2
	3.f,	0,	// 3
	4.f,	0,	// 4
	5.f,	1,	// 5
	6.f,	1,	// 6
	7.f,	1,	// 7
	8.f,	1,	// 8
	9.f,	1,	// 9
	10.f,	2,	// 10
	11.f,	2,	// 11
	12.f,	2,	// 12
	13.f,	2,	// 13
	14.f,	2,	// 14
	15.f,	3,	// 15
	15.5f,	3,	// 16
	16.f,	3,	// 17
	16.5f,	3,	// 18
	17.f,	3,	// 19
	17.5f,	4,	// 20
	18.f,	4,	// 21
	18.5f,	4,	// 22
	19.f,	4,	// 23
	19.5f,	4,	// 24
	20.f,	5,	// 25
};

//-----------------------------------------------------------------------------
// List of all skills
Skill Skill::skills[(int)SkillId::MAX] = {
	Skill(SkillId::ONE_HANDED_WEAPON, "one_handed_weapon", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::DEX, SkillPack::WEAPON),
	Skill(SkillId::SHORT_BLADE, "short_blade", SkillGroupId::WEAPON, AttributeId::DEX, AttributeId::NONE, SkillPack::WEAPON),
	Skill(SkillId::LONG_BLADE, "long_blade", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::DEX, SkillPack::WEAPON),
	Skill(SkillId::BLUNT, "blunt", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::NONE, SkillPack::WEAPON),
	Skill(SkillId::AXE, "axe", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::NONE, SkillPack::WEAPON),
	Skill(SkillId::BOW, "bow", SkillGroupId::WEAPON, AttributeId::DEX, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::UNARMED, "unarmed", SkillGroupId::WEAPON, AttributeId::DEX, AttributeId::STR, SkillPack::NONE),
	Skill(SkillId::SHIELD, "shield", SkillGroupId::ARMOR, AttributeId::STR, AttributeId::DEX, SkillPack::NONE),
	Skill(SkillId::LIGHT_ARMOR, "light_armor", SkillGroupId::ARMOR, AttributeId::DEX, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::MEDIUM_ARMOR, "medium_armor", SkillGroupId::ARMOR, AttributeId::END, AttributeId::DEX, SkillPack::NONE),
	Skill(SkillId::HEAVY_ARMOR, "heavy_armor", SkillGroupId::ARMOR, AttributeId::STR, AttributeId::END, SkillPack::NONE),
	Skill(SkillId::NATURE_MAGIC, "nature_magic", SkillGroupId::MAGIC, AttributeId::WIS, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::GODS_MAGIC, "gods_magic", SkillGroupId::MAGIC, AttributeId::WIS, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::MYSTIC_MAGIC, "mystic_magic", SkillGroupId::MAGIC, AttributeId::INT, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::SPELLCRAFT, "spellcraft", SkillGroupId::MAGIC, AttributeId::INT, AttributeId::WIS, SkillPack::NONE),
	Skill(SkillId::CONCENTRATION, "concentration", SkillGroupId::MAGIC, AttributeId::END, AttributeId::WIS, SkillPack::NONE),
	Skill(SkillId::IDENTIFICATION, "identification", SkillGroupId::MAGIC, AttributeId::INT, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::LOCKPICK, "lockpick", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::INT, SkillPack::THIEF),
	Skill(SkillId::SNEAK, "sneak", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::NONE, SkillPack::THIEF),
	Skill(SkillId::TRAPS, "traps", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::INT, SkillPack::THIEF),
	Skill(SkillId::STEAL, "steal", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::CHA, SkillPack::THIEF),
	Skill(SkillId::ANIMAL_EMPATHY, "animal_empathy", SkillGroupId::OTHER, AttributeId::CHA, AttributeId::WIS, SkillPack::NONE),
	Skill(SkillId::SURVIVAL, "survival", SkillGroupId::OTHER, AttributeId::END, AttributeId::WIS, SkillPack::NONE),
	Skill(SkillId::HAGGLE, "haggle", SkillGroupId::OTHER, AttributeId::CHA, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::ALCHEMY, "alchemy", SkillGroupId::OTHER, AttributeId::INT, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::CRAFTING, "crafting", SkillGroupId::OTHER, AttributeId::INT, AttributeId::DEX, SkillPack::NONE),
	Skill(SkillId::HEALING, "healing", SkillGroupId::OTHER, AttributeId::WIS, AttributeId::NONE, SkillPack::NONE),
	Skill(SkillId::ATHLETICS, "athletics", SkillGroupId::OTHER, AttributeId::END, AttributeId::STR, SkillPack::NONE),
	Skill(SkillId::RAGE, "rage", SkillGroupId::OTHER, AttributeId::END, AttributeId::NONE, SkillPack::NONE),
};

//-----------------------------------------------------------------------------
// List of all skill groups
SkillGroup SkillGroup::groups[(int)SkillGroupId::MAX] = {
	SkillGroup(SkillGroupId::WEAPON, "weapon"),
	SkillGroup(SkillGroupId::ARMOR, "armor"),
	SkillGroup(SkillGroupId::MAGIC, "magic"),
	SkillGroup(SkillGroupId::OTHER, "other")
};

//=================================================================================================
Skill* Skill::Find(const string& id)
{
	for(Skill& si : Skill::skills)
	{
		if(id == si.id)
			return &si;
	}

	return nullptr;
}

//=================================================================================================
SkillGroup* SkillGroup::Find(const string& id)
{
	for(SkillGroup& sgi : SkillGroup::groups)
	{
		if(id == sgi.id)
			return &sgi;
	}

	return nullptr;
}

//=================================================================================================
void Skill::Validate(uint& err)
{
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		Skill& si = Skill::skills[i];
		if(si.skill_id != (SkillId)i)
		{
			Warn("Test: SkillId %s: id mismatch.", si.id);
			++err;
		}
		if(si.name.empty())
		{
			Warn("Test: SkillId %s: empty name.", si.id);
			++err;
		}
		if(si.desc.empty())
		{
			Warn("Test: SkillId %s: empty desc.", si.id);
			++err;
		}
	}

	for(int i = 0; i < (int)SkillGroupId::MAX; ++i)
	{
		SkillGroup& sgi = SkillGroup::groups[i];
		if(sgi.group_id != (SkillGroupId)i)
		{
			Warn("Test: SkillId group %s: id mismatch.", sgi.id);
			++err;
		}
		if(sgi.name.empty())
		{
			Warn("Test: SkillId group %s: empty name.", sgi.id);
			++err;
		}
	}
}

//=================================================================================================
float Skill::GetModifier(int base, int& weight)
{
	if(base <= 0)
	{
		weight = 0;
		return 0.f;
	}
	else if(base >= 25)
	{
		weight = 5;
		return 8.75f;
	}
	else
	{
		StatGain& sg = gain[base];
		weight = sg.weight;
		return sg.value;
	}
}
