// character skills & gain tables
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"

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
	NONE,
	SPECIAL_WEAPON,
	SPECIAL_ARMOR
};
static_assert((int)SkillId::MAX < 32, "Max 32 skills, send as bit flags!");

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
enum class SkillType
{
	NONE,
	WEAPON,
	ARMOR
};

//-----------------------------------------------------------------------------
struct Skill
{
	SkillId skill_id;
	cstring id;
	string name, desc;
	SkillGroupId group;
	AttributeId attrib, attrib2;
	SkillType type;
	bool locked; // can't learn from trainer

	static const int MIN = 0;
	static const int MAX = 255;
	static const int TAG_BONUS = 10;

	Skill(SkillId skill_id, cstring id, SkillGroupId group, AttributeId attrib, AttributeId attrib2, SkillType type = SkillType::NONE, bool locked = false) :
		skill_id(skill_id), id(id), group(group), attrib(attrib), attrib2(attrib2), type(type), locked(locked)
	{
	}

	static Skill skills[(int)SkillId::MAX];
	static Skill* Find(const string& id);
	static void Validate(uint& err);
	static float GetModifier(int base);
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
