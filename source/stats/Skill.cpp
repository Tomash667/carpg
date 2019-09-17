// character skills & gain tables
#include "Pch.h"
#include "GameCore.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
// List of all skills
Skill Skill::skills[(int)SkillId::MAX] = {
	Skill(SkillId::ONE_HANDED_WEAPON, "one_handed_weapon", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::DEX),
	Skill(SkillId::SHORT_BLADE, "short_blade", SkillGroupId::WEAPON, AttributeId::DEX, AttributeId::NONE, SkillType::WEAPON),
	Skill(SkillId::LONG_BLADE, "long_blade", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::DEX, SkillType::WEAPON),
	Skill(SkillId::BLUNT, "blunt", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::NONE, SkillType::WEAPON),
	Skill(SkillId::AXE, "axe", SkillGroupId::WEAPON, AttributeId::STR, AttributeId::NONE, SkillType::WEAPON),
	Skill(SkillId::BOW, "bow", SkillGroupId::WEAPON, AttributeId::DEX, AttributeId::NONE),
	Skill(SkillId::UNARMED, "unarmed", SkillGroupId::WEAPON, AttributeId::DEX, AttributeId::STR),
	Skill(SkillId::SHIELD, "shield", SkillGroupId::ARMOR, AttributeId::STR, AttributeId::DEX),
	Skill(SkillId::LIGHT_ARMOR, "light_armor", SkillGroupId::ARMOR, AttributeId::DEX, AttributeId::NONE, SkillType::ARMOR),
	Skill(SkillId::MEDIUM_ARMOR, "medium_armor", SkillGroupId::ARMOR, AttributeId::END, AttributeId::DEX, SkillType::ARMOR),
	Skill(SkillId::HEAVY_ARMOR, "heavy_armor", SkillGroupId::ARMOR, AttributeId::STR, AttributeId::END, SkillType::ARMOR),
	Skill(SkillId::NATURE_MAGIC, "nature_magic", SkillGroupId::MAGIC, AttributeId::WIS, AttributeId::NONE),
	Skill(SkillId::GODS_MAGIC, "gods_magic", SkillGroupId::MAGIC, AttributeId::CHA, AttributeId::NONE),
	Skill(SkillId::MYSTIC_MAGIC, "mystic_magic", SkillGroupId::MAGIC, AttributeId::INT, AttributeId::NONE),
	Skill(SkillId::SPELLCRAFT, "spellcraft", SkillGroupId::MAGIC, AttributeId::INT, AttributeId::WIS),
	Skill(SkillId::CONCENTRATION, "concentration", SkillGroupId::MAGIC, AttributeId::WIS, AttributeId::END),
	Skill(SkillId::IDENTIFICATION, "identification", SkillGroupId::MAGIC, AttributeId::INT, AttributeId::NONE),
	Skill(SkillId::LOCKPICK, "lockpick", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::INT),
	Skill(SkillId::SNEAK, "sneak", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::NONE),
	Skill(SkillId::TRAPS, "traps", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::INT),
	Skill(SkillId::STEAL, "steal", SkillGroupId::OTHER, AttributeId::DEX, AttributeId::CHA),
	Skill(SkillId::ANIMAL_EMPATHY, "animal_empathy", SkillGroupId::OTHER, AttributeId::CHA, AttributeId::WIS),
	Skill(SkillId::SURVIVAL, "survival", SkillGroupId::OTHER, AttributeId::END, AttributeId::WIS),
	Skill(SkillId::HAGGLE, "haggle", SkillGroupId::OTHER, AttributeId::CHA, AttributeId::NONE),
	Skill(SkillId::ALCHEMY, "alchemy", SkillGroupId::OTHER, AttributeId::INT, AttributeId::NONE),
	Skill(SkillId::CRAFTING, "crafting", SkillGroupId::OTHER, AttributeId::INT, AttributeId::DEX),
	Skill(SkillId::HEALING, "healing", SkillGroupId::OTHER, AttributeId::WIS, AttributeId::NONE),
	Skill(SkillId::ATHLETICS, "athletics", SkillGroupId::OTHER, AttributeId::END, AttributeId::STR),
	Skill(SkillId::RAGE, "rage", SkillGroupId::OTHER, AttributeId::END, AttributeId::NONE),
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
float Skill::GetModifier(int base)
{
	if(base < 0)
		return 0;
	return float(base) / 5;
}
