// ró¿ne statystyki postaci
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
// Typ broni
enum WeaponType
{
	W_NONE,
	W_ONE_HANDED,
	W_BOW
};

//-----------------------------------------------------------------------------
enum class StatState
{
	NORMAL,
	POSITIVE,
	POSITIVE_MIXED,
	MIXED,
	NEGATIVE_MIXED,
	NEGATIVE,
	MAX
};

//-----------------------------------------------------------------------------
namespace old
{
	struct UnitStats
	{
		int attrib[(int)Attribute::MAX];
		int skill[(int)Skill::MAX];
	};
}
