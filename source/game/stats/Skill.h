// character skills & gain tables
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"

//-----------------------------------------------------------------------------
enum class OldSkill
{
	WEAPON,
	BOW,
	LIGHT_ARMOR,
	HEAVY_ARMOR,
	SHIELD,
	MAX
};

//-----------------------------------------------------------------------------
enum class Skill
{
	// weapon
	ONE_HANDED_WEAPON,
	SHORT_BLADE,
	LONG_BLADE,
	BLUNT,
	AXE,
	BOW,
	UNARMED,
	// shield & armor
	SHIELD,
	LIGHT_ARMOR,
	MEDIUM_ARMOR,
	HEAVY_ARMOR,
	// magic
	NATURE_MAGIC,
	GODS_MAGIC,
	MYSTIC_MAGIC,
	SPELLCRAFT,
	CONCENTRATION,
	IDENTIFICATION,
	// other
	LOCKPICK,
	SNEAK,
	TRAPS,
	STEAL,
	ANIMAL_EMPATHY,
	SURVIVAL,
	PERSUASION,
	ALCHEMY,
	CRAFTING,
	HEALING,
	ATHLETICS,
	RAGE,

	MAX,
	NONE
};

//-----------------------------------------------------------------------------
enum class SkillGroup
{
	WEAPON,
	ARMOR,
	MAGIC,
	OTHER,

	MAX,
	NONE
};

//-----------------------------------------------------------------------------
struct SkillInfo
{
	Skill skill_id;
	cstring id;
	string name, desc;
	SkillGroup group;
	Attribute attrib, attrib2;

	static const int MAX = 255;

	inline SkillInfo(Skill skill_id, cstring id, SkillGroup group, Attribute attrib, Attribute attrib2 = Attribute::NONE) : skill_id(skill_id), id(id), group(group), attrib(attrib),
		attrib2(attrib2)
	{

	}

	static SkillInfo* Find(const string& id);
	static void Validate(int& err);
	static float GetModifier(int base, int& weight);
};

//-----------------------------------------------------------------------------
struct SkillGroupInfo
{
	SkillGroup group_id;
	cstring id;
	string name;

	inline SkillGroupInfo(SkillGroup group_id, cstring id) : group_id(group_id), id(id)
	{

	}

	static SkillGroupInfo* Find(const string& id);
};



//-----------------------------------------------------------------------------
extern SkillInfo g_skills[(int)Skill::MAX];
extern SkillGroupInfo g_skill_groups[(int)SkillGroup::MAX];
