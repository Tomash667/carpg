#include "Pch.h"
#include "Portal.h"

//=================================================================================================
void Portal::Save(GameWriter& f)
{
	f << pos;
	f << rot;
	f << atLevel;
	f << index;
	f << targetLoc;
}

//=================================================================================================
void Portal::Load(GameReader& f)
{
	f >> pos;
	f >> rot;
	f >> atLevel;
	f >> index;
	f >> targetLoc;
}
