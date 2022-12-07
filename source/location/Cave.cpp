#include "Pch.h"
#include "Cave.h"

//=================================================================================================
void Cave::Save(GameWriter& f)
{
	SingleInsideLocation::Save(f);

	if(lastVisit != -1)
	{
		f << holes;
		f << ext;
	}
}

//=================================================================================================
void Cave::Load(GameReader& f)
{
	SingleInsideLocation::Load(f);

	if(lastVisit != -1)
	{
		f >> holes;
		f >> ext;
	}
}
