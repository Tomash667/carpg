#include "Pch.h"
#include "GameCore.h"
#include "Trap.h"

//-----------------------------------------------------------------------------
BaseTrap BaseTrap::traps[] = {
	BaseTrap("spear", 250, TRAP_SPEAR, "wlocznie.qmsh", "wlocznie2.qmsh", false, "science_fiction_steampunk_gear_lever.mp3", "wlocznie.mp3", "wlocznie2.mp3"),
	BaseTrap("arrow", 150, TRAP_ARROW, "przycisk.qmsh", nullptr, false, "click.mp3", nullptr, nullptr),
	BaseTrap("poison", 100, TRAP_POISON, "przycisk.qmsh", nullptr, false, "click.mp3", nullptr, nullptr),
	BaseTrap("fireball", 150, TRAP_FIREBALL, "runa.qmsh", nullptr, true, nullptr, nullptr, nullptr)
};
const uint BaseTrap::n_traps = countof(BaseTrap::traps);
