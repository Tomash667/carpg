#include "Pch.h"
#include "Cave.h"

//=================================================================================================
void Cave::Save(GameWriter& f)
{
	SingleInsideLocation::Save(f);

	if(last_visit != -1)
	{
		f << holes;
		f << ext;
	}
}

//=================================================================================================
void Cave::Load(GameReader& f)
{
	SingleInsideLocation::Load(f);

	if(last_visit != -1)
	{
		f >> holes;
		f >> ext;
	}
}
