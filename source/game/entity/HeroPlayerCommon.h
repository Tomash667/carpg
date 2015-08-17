// czêœæ wspólna PlayerController i HeroData
#pragma once

//-----------------------------------------------------------------------------
#include "Class.h"

//-----------------------------------------------------------------------------
struct Unit;

//-----------------------------------------------------------------------------
struct HeroPlayerCommon
{
	Class clas;
	Unit* unit;
	string name;
	int credit;
	bool on_credit;
};
