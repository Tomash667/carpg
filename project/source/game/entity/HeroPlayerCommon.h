// czêœæ wspólna PlayerController i HeroData
#pragma once

//-----------------------------------------------------------------------------
#include "Class.h"

//-----------------------------------------------------------------------------
struct Unit;

//-----------------------------------------------------------------------------
struct HeroPlayerCommon
{
	Unit* unit;
	int clas;
	string name;
	int credit;
	bool on_credit;
};
