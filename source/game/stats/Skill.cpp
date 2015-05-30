// character skills & gain tables
#include "Pch.h"
#include "Base.h"
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
SkillInfo g_skills[(int)Skill::MAX] = {
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
	SkillInfo(Skill::SPELLCRAFT, "spellcraft", SkillGroup::MAGIC, Attribute::INT, Attribute::WIS),
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
// List of all skill groups
SkillGroupInfo g_skill_groups[(int)SkillGroup::MAX] = {
	SkillGroupInfo(SkillGroup::WEAPON, "weapon"),
	SkillGroupInfo(SkillGroup::ARMOR, "armor"),
	SkillGroupInfo(SkillGroup::MAGIC, "magic"),
	SkillGroupInfo(SkillGroup::OTHER, "other")
};

//-----------------------------------------------------------------------------
// List of all skillset profiles
// Too long but better then writing it per unit >.<
SkillProfile g_skill_profiles[(int)SkillProfileType::MAX] = {
	{
		SkillProfileType::WARRIOR, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		20, 20, 20, 20, 20, 15, 15,
		// armor
		// (shi, lar, mar, har)
		15, 15, 15, 15,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 0, 0, -1, -1, 10, 0, -1, 0, 0, 15, -1
	},
	{
		SkillProfileType::HUNTER, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 20, 10,
		// armor
		// (shi, lar, mar, har)
		10, 20, 10, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		10, -1, -1, -1, 10, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 15, 10, 0, 20, 20, 0, 0, 5, 10, 10, -1
	},
	{
		SkillProfileType::ROGUE, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 20, 15, 10, 10, 15, 10,
		// armor
		// (shi, lar, mar, har)
		0, 15, 0, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 0,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		20, 20, 20, 20, -1, 10, 10, 0, 5, 0, 10, -1
	},
	{
		SkillProfileType::MAGE, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 0, 5, 0, 0,
		// armor
		// (shi, lar, mar, har)
		0, 5, 0, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, 20, 20, 15, 20,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 0, 0, 0, -1, 0, 0, 20, 0, 0, -1, -1
	},
	{
		SkillProfileType::CLERIC, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 5,
		// armor
		// (shi, lar, mar, har)
		10, 10, 15, 10,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, 20, -1, 5, 20, 5,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, 0, 15, 10, 0, 20, 0, -1
	},
	{
		SkillProfileType::BARBARIAN, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		20, 15, 20, 20, 20, 15, 15,
		// armor
		// (shi, lar, mar, har)
		10, 20, 10, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 0, 0, 0, 20, -1, -1, 0, 0, 20, 20
	},
	{
		SkillProfileType::BARD, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 10, 10, 15, 0,
		// armor
		// (shi, lar, mar, har)
		0, 15, 10, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, 0, -1, -1, 0,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		10, 10, 10, 10, -1, 0, 20, 0, 0, 0, 0, -1
	},
	{
		SkillProfileType::DRUID, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 10,
		// armor
		// (shi, lar, mar, har)
		10, 15, 10, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		20, -1, -1, 5, 20, 0,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 0, -1, 20, 15, -1, 10, 0, 20, 0, -1
	},
	{
		SkillProfileType::MONK, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		10, 10, 10, 10, 10, 10, 20,
		// armor
		// (shi, lar, mar, har)
		0, 10, 0, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, 10, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 15, 0, 0, -1, 10, 0, 0, 0, 0, 15, -1
	},
	{
		SkillProfileType::PALADIN, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		20, 10, 20, 15, 15, 10, 5,
		// armor
		// (shi, lar, mar, har)
		20, 10, 15, 20,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, 10, -1, -1, 10, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 10, 10, 0, 0, 10, 10, -1
	},
	{
		SkillProfileType::BLACKSMITH, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		10, 10, 10, 10, 10, 5, 5,
		// armor
		// (shi, lar, mar, har)
		5, 10, 5, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 5,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		5, -1, 0, -1, -1, 0, 10, -1, 20, -1, 5, -1
	},
	{
		SkillProfileType::MERCHANT, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor
		// (shi, lar, mar, har)
		0, 5, 0, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 0,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 0, 20, 0, 5, 0, -1, -1
	},
	{
		SkillProfileType::ALCHEMIST, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor
		// (shi, lar, mar, har)
		0, 5, 0, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 0,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 0, 10, 20, 5, 0, -1, -1
	},
	{
		SkillProfileType::COMMONER, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 0, 5,
		// armor
		// (shi, lar, mar, har)
		0, 5, -1, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 0, 0, -1, 5, 0, -1, -1
	},
	{
		SkillProfileType::CLERK, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor
		// (shi, lar, mar, har)
		0, 5, -1, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, -1, 10, -1, 0, 0, -1, -1
	},
	{
		SkillProfileType::FIGHTER, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 10,
		// armor
		// (shi, lar, mar, har)
		10, 10, 10, 0,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, 0, -1, -1, 5, -1, -1, 5, 0, 5, -1
	},
	{
		SkillProfileType::WORKER, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		10, 5, 5, 10, 10, 0, 5,
		// armor
		// (shi, lar, mar, har)
		0, 10, 5, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 0, -1, -1, -1, 0, 0, -1, 5, 0, 0, -1
	},
	{
		SkillProfileType::TOMASHU, true,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		120, 120, 120, 120, 120, 120, 120,
		// armor
		// (shi, lar, mar, har)
		120, 120, 120, 120,
		// magic
		// (nat, god, mys, spl, con, ide)
		120, 120, 120, 120, 120, 120,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120
	},
	{
		SkillProfileType::UNK, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 0, 0, 0, 0, 0, 15,
		// armor
		// (shi, lar, mar, har)
		0, 0, 0, 0,
		// magic
		// (nat, god, mys, spl, con, ide)
		0, 0, 0, 0, 0, 0,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 15, 0, 0, 0, 15, 0, 0, 0, 0, 15, 0
	},
	{
		SkillProfileType::SHAMAN, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 10, 10, 15, 15, 10, 10,
		// armor
		// (shi, lar, mar, har)
		10, 10, 10, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, 15, -1, 15, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 5, -1, -1, 15, -1, -1, 5, 0, 15, -1
	},
	{
		SkillProfileType::ORC, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 10, 10, 15, 15, 10, 10,
		// armor
		// (shi, lar, mar, har)
		10, 10, 10, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 5, -1, -1, 15, -1, -1, 5, 0, 15, -1
	},
	{
		SkillProfileType::ORC_BLACKSMITH, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 10, 10, 15, 15, 10, 10,
		// armor
		// (shi, lar, mar, har)
		10, 10, 10, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 5, -1, -1, 15, -1, -1, 15, 0, 15, -1
	},
	{
		SkillProfileType::GOBLIN, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 10, 10, 10, 10, 10,
		// armor
		// (shi, lar, mar, har)
		5, 10, -1, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		5, 15, 5, 10, -1, 10, -1, -1, 5, -1, 10, -1
	},
	{
		SkillProfileType::ZOMBIE, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		0, 0, 0, 0, 0, 0, 10,
		// armor
		// (shi, lar, mar, har)
		0, 0, 0, 0,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		SkillProfileType::SKELETON, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 10,
		// armor
		// (shi, lar, mar, har)
		10, 10, 10, 0,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		SkillProfileType::SKELETON_MAGE, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor
		// (shi, lar, mar, har)
		-1, 5, -1, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, 15, -1, 5, 10,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		SkillProfileType::EVIL_BOSS, true,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		90, 50, 60, 70, 90, 50, 60,
		// armor
		// (shi, lar, mar, har)
		90, 50, 60, 90,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, 80, -1, -1, 80, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 50, -1, -1, -1, 50, 70, -1
	},
	{
		SkillProfileType::GOLEM, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		-1, -1, -1, -1, -1, -1, 10,
		// armor
		// (shi, lar, mar, har)
		-1, -1, -1, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		SkillProfileType::ANIMAL, false,
		// combat
		// (1hw, sho, lon, axe, blu, bow, una)
		-1, -1, -1, -1, -1, -1, 15,
		// armor
		// (shi, lar, mar, har)
		-1, -1, -1, -1,
		// magic
		// (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other
		// (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 15, 5, -1, 5, 15, -1, -1, -1, -1, 5, -1
	},
};

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
SkillGroupInfo* SkillGroupInfo::Find(const string& id)
{
	for(SkillGroupInfo& sgi : g_skill_groups)
	{
		if(id == sgi.id)
			return &sgi;
	}

	return NULL;
}

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

	for(int i = 0; i<(int)SkillGroup::MAX; ++i)
	{
		SkillGroupInfo& sgi = g_skill_groups[i];
		if(sgi.group_id != (SkillGroup)i)
		{
			WARN(Format("Skill group %s: id mismatch.", sgi.id));
			++err;
		}
		if(sgi.name.empty())
		{
			WARN(Format("Skill group %s: empty name.", sgi.id));
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

//=================================================================================================
void SkillProfile::Set(int level, int* skills)
{
	assert(skills);
	if(level == 0 || fixed)
	{
		for(int i = 0; i<(int)Skill::MAX; ++i)
			skills[i] = skill[i];
	}
	else
	{
		int unused;
		float lvl = float(level)/5;
		for(int i = 0; i<(int)Skill::MAX; ++i)
			skills[i] = skill[i] + int(SkillInfo::GetModifier(skill[i], unused) * lvl);
	}
}
