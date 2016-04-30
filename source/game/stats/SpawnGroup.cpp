#include "Pch.h"
#include "Base.h"
#include "SpawnGroup.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
SpawnGroup g_spawn_groups[] = {
	SpawnGroup("losowo", "", "", K_E, 0, false),
	SpawnGroup("gobliny", "goblins", nullptr, K_E, 0, true),
	SpawnGroup("orkowie", "orcs", nullptr, K_I, 0, true),
	SpawnGroup("bandyci", "bandits", nullptr, K_I, 0, false),
	SpawnGroup("nieumarli", "undead", nullptr, K_I, -2, false),
	SpawnGroup("nekro", "necros", nullptr, K_I, -1, false),
	SpawnGroup("magowie", "mages", nullptr, K_I, 0, false),
	SpawnGroup("golemy", "golems", nullptr, K_E, -2, false),
	SpawnGroup("magowie&golemy", "mages_n_golems", nullptr, K_I, -1, false),
	SpawnGroup("z³o", "evils", nullptr, K_I, -1, false),
	SpawnGroup("brak", "", "", K_E, -1, false),
	SpawnGroup("unk", "unk", nullptr, K_I, -2, false),
	SpawnGroup("wyzwanie", "", "", K_E, 0, false)
};

//=================================================================================================
UnitData* SpawnGroup::GetSpawnLeader() const
{
	assert(unit_group->leader);
	return unit_group->leader;
}
