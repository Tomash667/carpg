// character skills & gain tables
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"

//-----------------------------------------------------------------------------
namespace old
{
	namespace v0
	{
		// pre 0.4
		enum class Skill
		{
			WEAPON,
			BOW,
			LIGHT_ARMOR,
			HEAVY_ARMOR,
			SHIELD,
			MAX
		};
	}
	namespace v1
	{
		// pre 0.7
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
	}
}

//-----------------------------------------------------------------------------
enum class Skill
{
	// combat styles
	ONE_HANDED_WEAPON,
	// weapons
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
	// other
	ATHLETICS,
	ACROBATICS,
	HAGGLE,
	LITERACY,

	MAX,
	NONE,

	WEAPON_PROFILE,
	ARMOR_PROFILE
};

//-----------------------------------------------------------------------------
enum class SkillGroup
{
	COMBAT_STYLE,
	WEAPON,
	ARMOR,
	MAGIC,
	OTHER,

	MAX,
	NONE
};

//-----------------------------------------------------------------------------
/*enum class SkillPack
{
	THIEF,
	WEAPON,

	MAX,
	NONE
};*/

//-----------------------------------------------------------------------------
struct SkillInfo
{
	Skill skill_id;
	cstring id;
	string name, desc;
	SkillGroup group;
	Attribute attrib, attrib2;
	//SkillPack pack;

	static const int MAX = 255;

	SkillInfo(Skill skill_id, cstring id, SkillGroup group, Attribute attrib, Attribute attrib2) : skill_id(skill_id), id(id), group(group), attrib(attrib),
		attrib2(attrib2)
	{
	}

	static SkillInfo skills[(int)Skill::MAX];
	static SkillInfo* Find(const string& id);
	static void Validate(uint& err);
	static float GetModifier(int base, int& weight);
};

//-----------------------------------------------------------------------------
struct SkillGroupInfo
{
	SkillGroup group_id;
	cstring id;
	string name;

	SkillGroupInfo(SkillGroup group_id, cstring id) : group_id(group_id), id(id)
	{
	}

	static SkillGroupInfo groups[(int)SkillGroup::MAX];
	static SkillGroupInfo* Find(const string& id);
};

//-----------------------------------------------------------------------------
/*enum class SubSkill
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
	Skill skill;
};

//-----------------------------------------------------------------------------
extern SubSkillInfo g_sub_skills[(int)SubSkill::MAX];
*/
