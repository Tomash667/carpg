// ró¿ne statystyki postaci
#include "Pch.h"
#include "Base.h"
#include "UnitStats.h"

//-----------------------------------------------------------------------------
// wczytywane ze stats.txt
AttributeInfo g_attribute_info[A_MAX] = {
	"Str", NULL, NULL,
	"Con", NULL, NULL,
	"Dex", NULL, NULL
};

//-----------------------------------------------------------------------------
SkillInfo skill_infos[] = {
	"Weapon", NULL, NULL,
	"Bow", NULL, NULL,
	"LightArmor", NULL, NULL,
	"HeavyArmor", NULL, NULL,
	"Shield", NULL, NULL
};
