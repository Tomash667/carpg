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
	Class clas;
	string name;
	int credit;
	float split_gold;
	bool on_credit;
};
