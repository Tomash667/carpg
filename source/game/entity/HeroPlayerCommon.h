// czêœæ wspólna PlayerController i HeroData
#pragma once

//-----------------------------------------------------------------------------
#include "Class.h"

//-----------------------------------------------------------------------------
struct Unit;

//-----------------------------------------------------------------------------
struct HeroPlayerCommon
{
	CLASS clas;
	Unit* unit;
	string name;
	int kredyt;
	bool na_kredycie;
};
