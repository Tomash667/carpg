#include "Pch.h"
#include "Base.h"
#include "SpawnGroup.h"

//-----------------------------------------------------------------------------
SpawnGroup g_spawn_groups[] = {
	"losowo", "", "", K_E, 0, 0, false,
	"gobliny", "goblins", nullptr, K_E, 0, 0, true,
	"orkowie", "orcs", nullptr, K_I, 0, 0, true,
	"bandyci", "bandits", nullptr, K_I, 0, 0, false,
	"nieumarli", "undead", nullptr, K_I, 0, -2, false,
	"nekro", "necros", nullptr, K_I, 0, -1, false,
	"magowie", "mages", nullptr, K_I, 0, 0, false,
	"golemy", "golems", nullptr, K_E, 0, -2, false,
	"magowie&golemy", "mages_n_golems", nullptr, K_I, 0, -1, false,
	"z³o", "evils", nullptr, K_I, 0, -1, false,
	"brak", "", "", K_E, 0, -1, false,
	"unk", "unk", nullptr, K_I, 0, -2, false,
	"wyzwanie", "", "", K_E, 0, 0, false,
};

//=================================================================================================
cstring GetSpawnLeader(SPAWN_GROUP group)
{
	switch(group)
	{
	case SG_BANDYCI:
		return "bandit_hegemon_q";
	case SG_ORKOWIE:
		return "orc_chief_q";
	case SG_GOBLINY:
		return "goblin_chief_q";
	case SG_MAGOWIE:
		return "mage_q";
	case SG_ZLO:
		return "evil_cleric_q";
	default:
		assert(0);
		return "bandit_hegemon_q";
	}
}
