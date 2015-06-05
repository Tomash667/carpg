// ró¿ne statystyki postaci
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
// Typ broni
enum BRON
{
	B_BRAK,
	B_JEDNORECZNA,
	B_LUK
	/*
	B_DWURECZNA
	B_MIOTANA
	*/
};

//-----------------------------------------------------------------------------
// Przyczyna trenowania
enum TrainWhat
{
	Train_Hit,
	Train_Hurt,
	Train_Block,
	Train_Bash,
	Train_Shot
};

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

inline char StatStateToColor(StatState s)
{
	switch(s)
	{
	default:
	case StatState::NORMAL:
		return 'k';
	case StatState::POSITIVE:
		return 'g';
	case StatState::POSITIVE_MIXED:
		return '0';
	case StatState::MIXED:
		return 'y';
	case StatState::NEGATIVE_MIXED:
		return '1';
	case StatState::NEGATIVE:
		return 'r';
	}
}

struct StatInfo
{
	int value, start, base;
	StatState state;
};

struct UnitStats
{

};
