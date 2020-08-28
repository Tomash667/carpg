#include "Pch.h"
#include "Portal.h"

//=================================================================================================
void Portal::Save(GameWriter& f)
{
	f << pos;
	f << rot;
	f << at_level;
	f << index;
	f << target_loc;
}

//=================================================================================================
void Portal::Load(GameReader& f)
{
	f >> pos;
	f >> rot;
	f >> at_level;
	f >> index;
	f >> target_loc;
}
