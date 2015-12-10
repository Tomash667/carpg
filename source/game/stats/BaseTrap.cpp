// bazowa pu³apka
#include "Pch.h"
#include "Base.h"
#include "Trap.h"

//-----------------------------------------------------------------------------
BaseTrap g_traps[] = {
	"spear", 150, TRAP_SPEAR, "wlocznie.qmsh", "wlocznie2.qmsh", nullptr, nullptr, false, 0.f, 0.f, "science_fiction_steampunk_gear_lever.mp3", "wlocznie.mp3", "wlocznie2.mp3", nullptr, nullptr, nullptr,
	"arrow", 100, TRAP_ARROW, "przycisk.qmsh", nullptr, nullptr, nullptr, false, 0.f, 0.f, "click.mp3", nullptr, nullptr, nullptr, nullptr, nullptr,
	"poison", 50, TRAP_POISON, "przycisk.qmsh", nullptr, nullptr, nullptr, false, 0.f, 0.f, "click.mp3", nullptr, nullptr, nullptr, nullptr, nullptr,
	"fireball", 120, TRAP_FIREBALL, "runa.qmsh", nullptr, nullptr, nullptr, true, 0.f, 0.f, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
const uint n_traps = countof(g_traps);
