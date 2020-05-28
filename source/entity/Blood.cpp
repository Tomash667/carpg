#include "Pch.h"
#include "Blood.h"

#include "BitStreamFunc.h"
#include "SaveState.h"

//=================================================================================================
void Blood::Save(FileWriter& f) const
{
	f.Write<byte>(type);
	f << pos;
	f << normal;
	f << size;
	f << scale;
	f << rot;
}

//=================================================================================================
void Blood::Load(FileReader& f)
{
	type = (BLOOD)f.Read<byte>();
	f >> pos;
	f >> normal;
	f >> size;
	if(LOAD_VERSION >= V_0_9)
		f >> scale;
	else
		scale = 1.f;
	f >> rot;
}

//=================================================================================================
void Blood::Write(BitStreamWriter& f) const
{
	f.WriteCasted<byte>(type);
	f << pos;
	f << normal;
	f << rot;
	f << scale;
}

//=================================================================================================
void Blood::Read(BitStreamReader& f)
{
	size = 1.f;
	f.ReadCasted<byte>(type);
	f >> pos;
	f >> normal;
	f >> rot;
	f >> scale;
}
