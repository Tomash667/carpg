// character skills & gain tables
#include "Pch.h"
#include "Core.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
struct StatGain
{
	float value; // skill gain per 5 levels
	int weight; // weight for level calculation
};

//-----------------------------------------------------------------------------
// Skill gain table. Normalny no class have skill better then 20 but it may happen.
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
// ! order by group/skill_group
SkillInfo SkillInfo::skills[(int)Skill::MAX] = {
	// combat styles
	SkillInfo(Skill::ONE_HANDED_WEAPON, "one_handed_weapon", SkillGroup::COMBAT_STYLE, Attribute::STR, Attribute::DEX, -1),
	// weapons
	SkillInfo(Skill::SHORT_BLADE, "short_blade", SkillGroup::WEAPON, Attribute::DEX, Attribute::NONE, 0),
	SkillInfo(Skill::LONG_BLADE, "long_blade", SkillGroup::WEAPON, Attribute::STR, Attribute::DEX, 0),
	SkillInfo(Skill::BLUNT, "blunt", SkillGroup::WEAPON, Attribute::STR, Attribute::NONE, 0),
	SkillInfo(Skill::AXE, "axe", SkillGroup::WEAPON, Attribute::STR, Attribute::NONE, 0),
	SkillInfo(Skill::BOW, "bow", SkillGroup::WEAPON, Attribute::DEX, Attribute::NONE, -1),
	SkillInfo(Skill::UNARMED, "unarmed", SkillGroup::WEAPON, Attribute::DEX, Attribute::STR, -1),
	// shield & armor
	SkillInfo(Skill::SHIELD, "shield", SkillGroup::ARMOR, Attribute::STR, Attribute::DEX, -1),
	SkillInfo(Skill::LIGHT_ARMOR, "light_armor", SkillGroup::ARMOR, Attribute::DEX, Attribute::NONE, 1),
	SkillInfo(Skill::MEDIUM_ARMOR, "medium_armor", SkillGroup::ARMOR, Attribute::END, Attribute::DEX, 1),
	SkillInfo(Skill::HEAVY_ARMOR, "heavy_armor", SkillGroup::ARMOR, Attribute::STR, Attribute::END, 1),
	// oher
	SkillInfo(Skill::ATHLETICS, "athletics", SkillGroup::OTHER, Attribute::END, Attribute::STR, -1),
	SkillInfo(Skill::ACROBATICS, "acrobatics", SkillGroup::OTHER, Attribute::DEX, Attribute::END, -1),
	SkillInfo(Skill::HAGGLE, "haggle", SkillGroup::OTHER, Attribute::CHA, Attribute::NONE, -1),
	SkillInfo(Skill::LITERACY, "literacy", SkillGroup::OTHER, Attribute::INT, Attribute::NONE, -1)
};

//-----------------------------------------------------------------------------
// List of all skill groups
SkillGroupInfo SkillGroupInfo::groups[(int)SkillGroup::MAX] = {
	SkillGroupInfo(SkillGroup::COMBAT_STYLE, "combat_style"),
	SkillGroupInfo(SkillGroup::WEAPON, "weapon"),
	SkillGroupInfo(SkillGroup::ARMOR, "armor"),
	SkillGroupInfo(SkillGroup::MAGIC, "magic"),
	SkillGroupInfo(SkillGroup::OTHER, "other")
};

//-----------------------------------------------------------------------------
/*SubSkillInfo g_sub_skills[(int)SubSkill::MAX] = {
	SubSkill::FIND_TRAP, Skill::TRAPS,
	SubSkill::SET_TRAP, Skill::TRAPS,
	SubSkill::DISARM_TRAP, Skill::TRAPS,
	SubSkill::HIDE, Skill::SNEAK,
	SubSkill::MOVE_SILENTLY, Skill::SNEAK,
};*/

//=================================================================================================
SkillInfo* SkillInfo::Find(const AnyString& id)
{
	for(SkillInfo& si : skills)
	{
		if(id == si.id)
			return &si;
	}

	return nullptr;
}

//=================================================================================================
SkillGroupInfo* SkillGroupInfo::Find(const string& id)
{
	for(SkillGroupInfo& sgi : groups)
	{
		if(id == sgi.id)
			return &sgi;
	}

	return nullptr;
}

//=================================================================================================
void SkillInfo::Validate(uint& err)
{
	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		SkillInfo& si = skills[i];
		if(si.skill_id != (Skill)i)
		{
			Warn("Test: Skill %s: id mismatch.", si.id);
			++err;
		}
		if(si.name.empty())
		{
			Warn("Test: Skill %s: empty name.", si.id);
			++err;
		}
		if(si.desc.empty())
		{
			Warn("Test: Skill %s: empty desc.", si.id);
			++err;
		}
	}

	for(int i = 0; i < (int)SkillGroup::MAX; ++i)
	{
		SkillGroupInfo& sgi = SkillGroupInfo::groups[i];
		if(sgi.group_id != (SkillGroup)i)
		{
			Warn("Test: Skill group %s: id mismatch.", sgi.id);
			++err;
		}
		if(sgi.name.empty())
		{
			Warn("Test: Skill group %s: empty name.", sgi.id);
			++err;
		}
	}
}

//=================================================================================================
float SkillInfo::GetModifier(int base, int& weight)
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
