#include "Pch.h"
#include "Base.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
SkillInfo g_skills[(int)Skill::MAX] = {
	SkillInfo(Skill::WEAPON, "one_handed_weapon"),
	SkillInfo(Skill::BOW, "bow"),
	SkillInfo(Skill::LIGHT_ARMOR, "light_armor"),
	SkillInfo(Skill::HEAVY_ARMOR, "heavy_armor"),
	SkillInfo(Skill::SHIELD, "shield")
};


//-----------------------------------------------------------------------------
/*SkillInfo g_skills[(int)Skill::MAX] = {
	SkillInfo(Skill::ONE_HANDED_WEAPON, "one_handed_weapon", SkillGroup::WEAPON, Attribute::STR, Attribute::DEX),
	SkillInfo(Skill::SHORT_BLADE, "short_blade", SkillGroup::WEAPON, Attribute::DEX),
	SkillInfo(Skill::LONG_BLADE, "long_blade", SkillGroup::WEAPON, Attribute::STR, Attribute::DEX),
	SkillInfo(Skill::BLUNT, "blunt", SkillGroup::WEAPON, Attribute::STR),
	SkillInfo(Skill::AXE, "axe", SkillGroup::WEAPON, Attribute::STR), 
	SkillInfo(Skill::BOW, "bow", SkillGroup::WEAPON, Attribute::DEX),
	SkillInfo(Skill::UNARMED, "unarmed", SkillGroup::WEAPON, Attribute::DEX, Attribute::STR),
	SkillInfo(Skill::SHIELD, "shield", SkillGroup::ARMOR, Attribute::STR, Attribute::DEX),
	SkillInfo(Skill::LIGHT_ARMOR, "light_armor", SkillGroup::ARMOR, Attribute::DEX),
	SkillInfo(Skill::MEDIUM_ARMOR, "medium_armor", SkillGroup::ARMOR, Attribute::STR, Attribute::DEX),
	SkillInfo(Skill::HEAVY_ARMOR, "heavy_armor", SkillGroup::ARMOR, Attribute::STR),
	SkillInfo(Skill::NATURE_MAGIC, "nature_magic", SkillGroup::MAGIC, Attribute::WIS),
	SkillInfo(Skill::GODS_MAGIC, "gods_magic", SkillGroup::MAGIC, Attribute::WIS),
	SkillInfo(Skill::MYSTIC_MAGIC, "mystic_magic", SkillGroup::MAGIC, Attribute::INT),
	SkillInfo(Skill::CONCENTRATION, "concentration", SkillGroup::MAGIC, Attribute::CON, Attribute::WIS),
	SkillInfo(Skill::IDENTIFICATION, "identification", SkillGroup::MAGIC, Attribute::INT),
	SkillInfo(Skill::LOCKPICK, "lockpick", SkillGroup::OTHER, Attribute::DEX, Attribute::INT),
	SkillInfo(Skill::SNEAK, "sneak", SkillGroup::OTHER, Attribute::DEX),
	SkillInfo(Skill::TRAPS, "traps", SkillGroup::OTHER, Attribute::DEX, Attribute::INT),
	SkillInfo(Skill::STEAL, "steal", SkillGroup::OTHER, Attribute::DEX, Attribute::CHA),
	SkillInfo(Skill::ANIMAL_EMPATHY, "animal_empathy", SkillGroup::OTHER, Attribute::CHA, Attribute::WIS),
	SkillInfo(Skill::SURVIVAL, "survival", SkillGroup::OTHER, Attribute::CON, Attribute::WIS),
	SkillInfo(Skill::PERSUASION, "persuasion", SkillGroup::OTHER, Attribute::CHA),
	SkillInfo(Skill::ALCHEMY, "alchemy", SkillGroup::OTHER, Attribute::INT),
	SkillInfo(Skill::CRAFTING, "crafting", SkillGroup::OTHER, Attribute::INT, Attribute::DEX),
	SkillInfo(Skill::HEALING, "healing", SkillGroup::OTHER, Attribute::WIS),
	SkillInfo(Skill::ATHLETICS, "athletics", SkillGroup::OTHER, Attribute::CON, Attribute::STR),
	SkillInfo(Skill::RAGE, "rage", SkillGroup::OTHER, Attribute::CON)
};

//-----------------------------------------------------------------------------
SkillGroupInfo g_skill_groups[(int)SkillGroup::MAX] = {
	SkillGroupInfo(SkillGroup::WEAPON, "weapon"),
	SkillGroupInfo(SkillGroup::ARMOR, "armor"),
	SkillGroupInfo(SkillGroup::MAGIC, "magic"),
	SkillGroupInfo(SkillGroup::OTHER, "other")
};*/

//=================================================================================================
SkillInfo* SkillInfo::Find(const string& id)
{
	for(SkillInfo& si : g_skills)
	{
		if(id == si.id)
			return &si;
	}

	return NULL;
}

//=================================================================================================
/*SkillGroupInfo* SkillGroupInfo::Find(const string& id)
{
	for(SkillGroupInfo& sgi : g_skill_groups)
	{
		if(id == sgi.id)
			return &sgi;
	}

	return NULL;
}*/

//=================================================================================================
void SkillInfo::Validate(int& err)
{
	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		SkillInfo& si = g_skills[i];
		if(si.skill_id != (Skill)i)
		{
			WARN(Format("Skill %s: id mismatch.", si.id));
			++err;
		}
		if(si.name.empty())
		{
			WARN(Format("Skill %s: empty name.", si.id));
			++err;
		}
		if(si.desc.empty())
		{
			WARN(Format("Skill %s: empty desc.", si.id));
			++err;
		}
	}

	/*for(int i = 0; i<(int)SkillGroup::MAX; ++i)
	{
		SkillGroupInfo& sgi = g_skill_groups[i];
		if(sgi.group_id != (SkillGroup)i)
			WARN(Format("Skill group %s: id mismatch.", sgi.id));
		if(sgi.name.empty())
			WARN(Format("Skill group %s: empty name.", sgi.id));
	}*/
}
