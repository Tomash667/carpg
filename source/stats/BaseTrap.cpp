#include "Pch.h"
#include "GameCore.h"
#include "Trap.h"

//-----------------------------------------------------------------------------
BaseTrap BaseTrap::traps[] = {
	BaseTrap("spear", 250, TRAP_SPEAR, "wlocznie.qmsh", "wlocznie2.qmsh", false, "science_fiction_steampunk_gear_lever.mp3", 1.f, "wlocznie.mp3", 1.5f, "wlocznie2.mp3", 0.5f),
	BaseTrap("arrow", 150, TRAP_ARROW, "przycisk.qmsh", nullptr, false, "click.mp3", 1.f, nullptr, 0.f, nullptr, 0.f),
	BaseTrap("poison", 100, TRAP_POISON, "przycisk.qmsh", nullptr, false, "click.mp3", 1.f, nullptr, 0.f, nullptr, 0.f),
	BaseTrap("fireball", 150, TRAP_FIREBALL, "runa.qmsh", nullptr, true, nullptr, 0.f, nullptr, 0.f, nullptr, 0.f)
};
const uint BaseTrap::n_traps = countof(BaseTrap::traps);
