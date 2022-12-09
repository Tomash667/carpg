#include "Pch.h"
#include "BaseTrap.h"

//-----------------------------------------------------------------------------
BaseTrap BaseTrap::traps[] = {
	BaseTrap("spear", 250, TRAP_SPEAR, "spear_trap.qmsh", "science_fiction_steampunk_gear_lever.mp3", 1.f, "wlocznie.mp3", 1.5f, "wlocznie2.mp3", 0.5f),
	BaseTrap("arrow", 150, TRAP_ARROW, "przycisk.qmsh", "click.mp3", 1.f, nullptr, 0.f, nullptr, 0.f),
	BaseTrap("poison", 100, TRAP_POISON, "przycisk.qmsh", "click.mp3", 1.f, nullptr, 0.f, nullptr, 0.f),
	BaseTrap("fireball", 150, TRAP_FIREBALL, "runa.qmsh", nullptr, 0.f, nullptr, 0.f, nullptr, 0.f),
	BaseTrap("bear", 300, TRAP_BEAR, "bear_trap.qmsh", "bear_trap.mp3", 1.5f, nullptr, 0.f, nullptr, 0.f)
};
const uint BaseTrap::nTraps = countof(BaseTrap::traps);

//=================================================================================================
BaseTrap* BaseTrap::Get(const string& id)
{
	for(int i = 0; i < TRAP_MAX; ++i)
	{
		if(id == traps[i].id)
			return &traps[i];
	}
	return nullptr;
}
