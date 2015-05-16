// klasy gracza i npc
#include "Pch.h"
#include "Base.h"
#include "Class.h"

//-----------------------------------------------------------------------------
Class g_classes[] = {
	NULL, "base_warrior", NULL,
	NULL, "base_hunter", NULL,
	NULL, "base_rogue", NULL
};
const uint n_classes = countof(g_classes);
