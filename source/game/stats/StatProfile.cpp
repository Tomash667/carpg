// attributes & skill profiles
#include "Pch.h"
#include "Base.h"
#include "StatProfile.h"

//-----------------------------------------------------------------------------
// List of all attributes & skill profiles
// Too long but better then writing it per unit >.<
StatProfile g_stat_profiles[(int)StatProfileType::MAX] = {
	{
		StatProfileType::BARBARIAN, false,
		// attributes (str, con, dex, int, wis, cha)
		70, 70, 65, 40, 50, 40,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		20, 15, 20, 20, 20, 15, 15,
		// armor (shi, lar, mar, har)
		10, 20, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 0, 0, 0, 20, -1, -1, 0, 0, 20, 20
	},
	{
		StatProfileType::BARD, false,
		// attributes (str, con, dex, int, wis, cha)
		55, 55, 65, 69, 50, 65,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 10, 10, 15, 0,
		// armor (shi, lar, mar, har)
		0, 15, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, 0, -1, -1, 0,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		10, 10, 10, 10, -1, 0, 20, 0, 0, 0, 0, -1
	},
	{
		StatProfileType::CLERIC, false,
		// attributes (str, con, dex, int, wis, cha)
		60, 60, 45, 50, 70, 55,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 5,
		// armor (shi, lar, mar, har)
		10, 10, 15, 10,
		// magic (nat, god, mys, spl, con, ide)
		-1, 20, -1, 5, 20, 5,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, 0, 15, 10, 0, 20, 0, -1
	},
	{
		StatProfileType::DRUID, false,
		// attributes (str, con, dex, int, wis, cha)
		60, 60, 55, 50, 70, 40,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 10,
		// armor (shi, lar, mar, har)
		10, 15, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		20, -1, -1, 5, 20, 0,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 0, -1, 20, 15, -1, 10, 0, 20, 0, -1
	},
	{
		StatProfileType::HUNTER, false,
		// attributes (str, con, dex, int, wis, cha)
		60, 65, 65, 50, 60, 45,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 20, 10,
		// armor (shi, lar, mar, har)
		10, 20, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		10, -1, -1, -1, 10, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 15, 10, 0, 20, 20, 0, 0, 5, 10, 10, -1
	},
	{
		StatProfileType::MAGE, false,
		// attributes (str, con, dex, int, wis, cha)
		45, 40, 50, 70, 50, 50,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 0, 5, 0, 0,
		// armor (shi, lar, mar, har)
		0, 5, 0, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, 20, 20, 15, 20,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 0, 0, 0, -1, 0, 0, 20, 0, 0, -1, -1
	},
	{
		StatProfileType::MONK, false,
		// attributes (str, con, dex, int, wis, cha)
		60, 60, 65, 50, 55, 45,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		10, 10, 10, 10, 10, 10, 20,
		// armor (shi, lar, mar, har)
		0, 10, 0, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, 10, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 15, 0, 0, -1, 10, 0, 0, 0, 0, 15, -1
	},
	{
		StatProfileType::PALADIN, false,
		// attributes (str, con, dex, int, wis, cha)
		65, 65, 45, 50, 60, 55,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		20, 10, 20, 15, 15, 10, 5,
		// armor (shi, lar, mar, har)
		20, 10, 15, 20,
		// magic (nat, god, mys, spl, con, ide)
		-1, 10, -1, -1, 10, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 10, 10, 0, 0, 10, 10, -1
	},
	{
		StatProfileType::ROGUE, false,
		// attributes (str, con, dex, int, wis, cha)
		55, 55, 70, 60, 50, 55,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 20, 15, 10, 10, 15, 10,
		// armor (shi, lar, mar, har)
		0, 15, 0, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 0,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		20, 20, 20, 20, -1, 10, 10, 0, 5, 0, 10, -1
	},
	{
		StatProfileType::WARRIOR, false,
		// attributes (str, con, dex, int, wis, cha)
		65, 65, 55, 50, 50, 50,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		20, 20, 20, 20, 20, 15, 15,
		// armor (shi, lar, mar, har)
		15, 15, 15, 15,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 0, 0, -1, -1, 10, 0, -1, 0, 0, 15, -1
	},
	{
		StatProfileType::BLACKSMITH, false,
		// attributes (str, con, dex, int, wis, cha)
		60, 50, 55, 55, 50, 50,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		10, 10, 10, 10, 10, 5, 5,
		// armor (shi, lar, mar, har)
		5, 10, 5, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 5,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		5, -1, 0, -1, -1, 0, 10, -1, 20, -1, 5, -1
	},
	{
		StatProfileType::MERCHANT, false,
		// attributes (str, con, dex, int, wis, cha)
		50, 50, 50, 55, 55, 55,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor (shi, lar, mar, har)
		0, 5, 0, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 0,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 0, 20, 0, 5, 0, -1, -1
	},
	{
		StatProfileType::ALCHEMIST, false,
		// attributes (str, con, dex, int, wis, cha)
		45, 45, 50, 60, 50, 50,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor (shi, lar, mar, har)
		0, 5, 0, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, 0,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 0, 10, 20, 5, 0, -1, -1
	},
	{
		StatProfileType::COMMONER, false,
		// attributes (str, con, dex, int, wis, cha)
		50, 50, 50, 50, 50, 50,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 0, 5,
		// armor (shi, lar, mar, har)
		0, 5, -1, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 0, 0, -1, 5, 0, -1, -1
	},
	{
		StatProfileType::CLERK, false,
		// attributes (str, con, dex, int, wis, cha)
		50, 50, 50, 55, 55, 55,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor (shi, lar, mar, har)
		0, 5, -1, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, -1, 10, -1, 0, 0, -1, -1
	},
	{
		StatProfileType::FIGHTER, false,
		// attributes (str, con, dex, int, wis, cha)
		60, 60, 55, 50, 50, 50,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 10,
		// armor (shi, lar, mar, har)
		10, 10, 10, 0,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, 0, -1, -1, 5, -1, -1, 5, 0, 5, -1
	},
	{
		StatProfileType::WORKER, false,
		// attributes (str, con, dex, int, wis, cha)
		55, 55, 55, 50, 50, 50,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		10, 5, 5, 10, 10, 0, 5,
		// armor (shi, lar, mar, har)
		0, 10, 5, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 0, -1, -1, -1, 0, 0, -1, 5, 0, 0, -1
	},	
	{
		StatProfileType::UNK, false,
		// attributes (str, con, dex, int, wis, cha)
		70, 70, 70, 0, 0, 0,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 0, 0, 0, 0, 0, 15,
		// armor (shi, lar, mar, har)
		0, 0, 0, 0,
		// magic (nat, god, mys, spl, con, ide)
		0, 0, 0, 0, 0, 0,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 15, 0, 0, 0, 15, 0, 0, 0, 0, 15, 0
	},
	{
		StatProfileType::SHAMAN, false,
		// attributes (str, con, dex, int, wis, cha)
		60, 55, 55, 60, 45, 30,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 10, 10, 15, 15, 10, 10,
		// armor (shi, lar, mar, har)
		10, 10, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, 15, -1, 15, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 5, -1, -1, 15, -1, -1, 5, 0, 15, -1
	},
	{
		StatProfileType::ORC, false,
		// attributes (str, con, dex, int, wis, cha)
		65, 60, 55, 30, 40, 30,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 10, 10, 15, 15, 10, 10,
		// armor (shi, lar, mar, har)
		10, 10, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 5, -1, -1, 15, -1, -1, 5, 0, 15, -1
	},
	{
		StatProfileType::ORC_BLACKSMITH, false,
		// attributes (str, con, dex, int, wis, cha)
		70, 60, 55, 50, 40, 30,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 10, 10, 15, 15, 10, 10,
		// armor (shi, lar, mar, har)
		10, 10, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, 5, -1, -1, 15, -1, -1, 15, 0, 15, -1
	},
	{
		StatProfileType::GOBLIN, false,
		// attributes (str, con, dex, int, wis, cha)
		45, 50, 65, 30, 40, 30,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 10, 10, 10, 10, 10,
		// armor (shi, lar, mar, har)
		5, 10, -1, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		5, 15, 5, 10, -1, 10, -1, -1, 5, -1, 10, -1
	},
	{
		StatProfileType::ZOMBIE, false,
		// attributes (str, con, dex, int, wis, cha)
		65, 65, 30, 0, 0, 0,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		0, 0, 0, 0, 0, 0, 10,
		// armor (shi, lar, mar, har)
		0, 0, 0, 0,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		StatProfileType::SKELETON, false,
		// attributes (str, con, dex, int, wis, cha)
		55, 50, 60, 0, 0, 0,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 10, 10,
		// armor (shi, lar, mar, har)
		10, 10, 10, 0,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		StatProfileType::SKELETON_MAGE, false,
		// attributes (str, con, dex, int, wis, cha)
		50, 50, 60, 60, 0, 0,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		5, 5, 5, 5, 5, 5, 5,
		// armor (shi, lar, mar, har)
		-1, 5, -1, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, 15, -1, 5, 10,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		StatProfileType::GOLEM, false,
		// attributes (str, con, dex, int, wis, cha)
		75, 75, 30, 0, 0, 0,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		-1, -1, -1, -1, -1, -1, 10,
		// armor (shi, lar, mar, har)
		-1, -1, -1, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	},
	{
		StatProfileType::ANIMAL, false,
		60, 70, 55, 10, 25, 10,
		// attributes (str, con, dex, int, wis, cha)
		// combat (1hw, sho, lon, axe, blu, bow, una)
		-1, -1, -1, -1, -1, -1, 15,
		// armor (shi, lar, mar, har)
		-1, -1, -1, -1,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 15, 5, -1, 5, 15, -1, -1, -1, -1, 5, -1
	},
	{
		StatProfileType::GORUSH_WARRIOR, false,
		// attributes (str, con, dex, int, wis, cha)
		70, 70, 55, 40, 50, 40,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		20, 20, 20, 20, 20, 15, 15,
		// armor (shi, lar, mar, har)
		15, 15, 15, 15,
		// magic (nat, god, mys, spl, con, ide)
		-1, -1, -1, -1, -1, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, 0, 0, -1, -1, 10, 0, -1, 0, 0, 15, -1
	},
	{
		StatProfileType::GORUSH_HUNTER, false,
		// attributes (str, con, dex, int, wis, cha)
		65, 65, 65, 40, 60, 35,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		15, 15, 15, 15, 15, 20, 10,
		// armor (shi, lar, mar, har)
		10, 20, 10, -1,
		// magic (nat, god, mys, spl, con, ide)
		10, -1, -1, -1, 10, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		0, 15, 10, 0, 20, 20, 0, 0, 5, 10, 10, -1
	},
	{
		StatProfileType::EVIL_BOSS, true,
		// attributes (str, con, dex, int, wis, cha)
		90, 90, 65, 60, 90, 60,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		90, 50, 60, 70, 90, 50, 60,
		// armor (shi, lar, mar, har)
		90, 50, 60, 90,
		// magic (nat, god, mys, spl, con, ide)
		-1, 80, -1, -1, 80, -1,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		-1, -1, -1, -1, -1, 50, -1, -1, -1, 50, 70, -1
	},
	{
		StatProfileType::TOMASHU, true,
		// attributes (str, con, dex, int, wis, cha)
		120, 120, 120, 120, 120, 120,
		// combat (1hw, sho, lon, axe, blu, bow, una)
		120, 120, 120, 120, 120, 120, 120,
		// armor (shi, lar, mar, har)
		120, 120, 120, 120,
		// magic (nat, god, mys, spl, con, ide)
		120, 120, 120, 120, 120, 120,
		// other (loc, sne, tra, stl, ani, sur, per, alc, cra, hea, ath, rag)
		120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120
	}
};

//=================================================================================================
void StatProfile::Set(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);

	if(level == 0 || fixed)
	{
		for(int i = 0; i<(int)Attribute::MAX; ++i)
			attribs[i] = attrib[i];
		for(int i = 0; i<(int)Skill::MAX; ++i)
			skills[i] = max(0, skill[i]);
	}
	else
	{
		int unused;
		float lvl = float(level)/5;
		for(int i = 0; i<(int)Attribute::MAX; ++i)
			attribs[i] = attrib[i] + int(AttributeInfo::GetModifier(attrib[i], unused) * lvl);
		for(int i = 0; i<(int)Skill::MAX; ++i)
		{
			int val = max(0, skill[i]);
			skills[i] = val + int(SkillInfo::GetModifier(val, unused) * lvl);
		}
	}
}

//=================================================================================================
void StatProfile::SetForNew(int level, int* attribs, int* skills) const
{
	assert(skills && attribs);

	if(level == 0 || fixed)
	{
		for(int i = 0; i<(int)Attribute::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i];
		}
		for(int i = 0; i<(int)Skill::MAX; ++i)
		{
			if(skills[i] == -1)
				skills[i] = max(0, skill[i]);
		}
	}
	else
	{
		int unused;
		float lvl = float(level)/5;
		for(int i = 0; i<(int)Attribute::MAX; ++i)
		{
			if(attribs[i] == -1)
				attribs[i] = attrib[i] + int(AttributeInfo::GetModifier(attrib[i], unused) * lvl);
		}
		for(int i = 0; i<(int)Skill::MAX; ++i)
		{
			if(skills[i] == -1)
			{
				int val = max(0, skill[i]);
				skills[i] = val + int(SkillInfo::GetModifier(val, unused) * lvl);
			}
		}
	}
}
