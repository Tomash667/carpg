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
enum class SkillId
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
enum class SkillGroupId
{
	WEAPON,
	ARMOR,
	MAGIC,
	OTHER,

	MAX,
	NONE
};

//-----------------------------------------------------------------------------
enum class SkillPack
{
	THIEF,
	WEAPON,

	MAX,
	NONE
};

//-----------------------------------------------------------------------------
struct Skill
{
	SkillId skill_id;
	cstring id;
	string name, desc;
	SkillGroupId group;
	AttributeId attrib, attrib2;
	SkillPack pack;

	static const int MIN = 0;
	static const int MAX = 255;

	Skill(SkillId skill_id, cstring id, SkillGroupId group, AttributeId attrib, AttributeId attrib2, SkillPack pack) : skill_id(skill_id), id(id), group(group), attrib(attrib),
		attrib2(attrib2), pack(pack)
	{
	}

	static Skill skills[(int)SkillId::MAX];
	static Skill* Find(const string& id);
	static void Validate(uint& err);
	static float GetModifier(int base, int& weight);
};

//-----------------------------------------------------------------------------
struct SkillGroup
{
	SkillGroupId group_id;
	cstring id;
	string name;

	SkillGroup(SkillGroupId group_id, cstring id) : group_id(group_id), id(id)
	{
	}

	static SkillGroup groups[(int)SkillGroupId::MAX];
	static SkillGroup* Find(const string& id);
};

//-----------------------------------------------------------------------------
enum class SubSkill
{
	// TRAPS
	FIND_TRAP,
	SET_TRAP,
	DISARM_TRAP,

	// SNEAK
	HIDE,
	MOVE_SILENTLY,

	// SURVIVAL

	MAX
};

//-----------------------------------------------------------------------------
struct SubSkillInfo
{
	SubSkill id;
	SkillId skill;
};

//-----------------------------------------------------------------------------
extern SubSkillInfo g_sub_skills[(int)SubSkill::MAX];
