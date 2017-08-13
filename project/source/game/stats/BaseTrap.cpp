// bazowa pu³apka
#include "Pch.h"
#include "Core.h"
#include "Trap.h"

//-----------------------------------------------------------------------------
BaseTrap g_traps[] = {
	BaseTrap("spear", 150, TRAP_SPEAR, "wlocznie.qmsh", "wlocznie2.qmsh", false, "science_fiction_steampunk_gear_lever.mp3", "wlocznie.mp3", "wlocznie2.mp3"),
	BaseTrap("arrow", 100, TRAP_ARROW, "przycisk.qmsh", nullptr, false, "click.mp3", nullptr, nullptr),
	BaseTrap("poison", 50, TRAP_POISON, "przycisk.qmsh", nullptr, false, "click.mp3", nullptr, nullptr),
	BaseTrap("fireball", 120, TRAP_FIREBALL, "runa.qmsh", nullptr, true, nullptr, nullptr, nullptr)
};
const uint n_traps = countof(g_traps);
