#include "Pch.h"
#include "GameCore.h"
#include "Blood.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

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
	if(LOAD_VERSION >= V_0_3)
		type = (BLOOD)f.Read<byte>();
	else
		type = (BLOOD)f.Read<int>();
	f >> pos;
	f >> normal;
	f >> size;
	if(LOAD_VERSION >= V_DEV)
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
