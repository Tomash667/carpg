// bazowa pu³apka
#include "Pch.h"
#include "Base.h"
#include "Trap.h"

//-----------------------------------------------------------------------------
BaseTrap g_traps[] = {
	"spear", 150, TRAP_SPEAR, "wlocznie.qmsh", "wlocznie2.qmsh", NULL, NULL, false, 0.f, 0.f, "science_fiction_steampunk_gear_lever.mp3", "wlocznie.mp3", "wlocznie2.mp3", NULL, NULL, NULL,
	"arrow", 100, TRAP_ARROW, "przycisk.qmsh", NULL, NULL, NULL, false, 0.f, 0.f, "click.mp3", NULL, NULL, NULL, NULL, NULL,
	"poison", 50, TRAP_POISON, "przycisk.qmsh", NULL, NULL, NULL, false, 0.f, 0.f, "click.mp3", NULL, NULL, NULL, NULL, NULL,
	"fireball", 120, TRAP_FIREBALL, "runa.qmsh", NULL, NULL, NULL, true, 0.f, 0.f, NULL, NULL, NULL, NULL, NULL, NULL
};
const uint n_traps = countof(g_traps);
