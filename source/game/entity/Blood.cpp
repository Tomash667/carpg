#include "Pch.h"
#include "Base.h"
#include "Blood.h"
#include "SaveState.h"

//=================================================================================================
void Blood::Save(File& f) const
{
	f.Write<byte>(type);
	f << pos;
	f << normal;
	f << size;
	f << rot;
}

//=================================================================================================
void Blood::Load(File& f)
{
	if(LOAD_VERSION >= V_0_3)
		type = (BLOOD)f.Read<byte>();
	else
		type = (BLOOD)f.Read<int>();
	f >> pos;
	f >> normal;
	f >> size;
	f >> rot;
}
