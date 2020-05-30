#include "Pch.h"
#include "Portal.h"

//=================================================================================================
void Portal::Save(FileWriter& f)
{
	f << pos;
	f << rot;
	f << at_level;
	f << index;
	f << target_loc;
}

//=================================================================================================
void Portal::Load(FileReader& f)
{
	f >> pos;
	f >> rot;
	f >> at_level;
	f >> index;
	f >> target_loc;
}
