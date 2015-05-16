#include "Pch.h"
#include "Base.h"
#include "SpawnGroup.h"

//-----------------------------------------------------------------------------
SpawnGroup g_spawn_groups[] = {
	"losowo", "", "", K_E, 0, 0, false,
	"gobliny", "goblins", NULL, K_E, 0, 0, true,
	"orkowie", "orcs", NULL, K_I, 0, 0, true,
	"bandyci", "bandits", NULL, K_I, 0, 0, false,
	"nieumarli", "undead", NULL, K_I, 0, -2, false,
	"nekro", "necros", NULL, K_I, 0, -1, false,
	"magowie", "mages", NULL, K_I, 0, 0, false,
	"golemy", "golems", NULL, K_E, 0, -2, false,
	"magowie&golemy", "mages_n_golems", NULL, K_I, 0, -1, false,
	"z³o", "evils", NULL, K_I, 0, -1, false,
	"brak", "", "", K_E, 0, -1, false,
	"unk", "unk", NULL, K_I, 0, -2, false,
	"wyzwanie", "", "", K_E, 0, 0, false,
};
