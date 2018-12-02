// character skills & gain tables
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"

//-----------------------------------------------------------------------------
namespace old
{
	enum class SkillId
	{
		WEAPON,
		BOW,
		LIGHT_ARMOR,
		HEAVY_ARMOR,
		SHIELD,
		MAX
	};
}

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
	HAGGLE,
	ALCHEMY,
	CRAFTING,
	HEALING,
	ATHLETICS,
	RAGE,

	MAX,
	NONE
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
struct Skill
{
	SkillId skill_id;
	cstring id;
	string name, desc;
	SkillGroupId group;
	AttributeId attrib, attrib2;

	static const int MIN = 0;
	static const int MAX = 255;

	Skill(SkillId skill_id, cstring id, SkillGroupId group, AttributeId attrib, AttributeId attrib2) : skill_id(skill_id), id(id), group(group),
		attrib(attrib), attrib2(attrib2)
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
