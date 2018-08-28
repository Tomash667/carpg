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
	f >> rot;
}

//=================================================================================================
void Blood::Write(BitStreamWriter& f) const
{
	f.WriteCasted<byte>(type);
	f << pos;
	f << normal;
	f << rot;
}

//=================================================================================================
void Blood::Read(BitStreamReader& f)
{
	size = 1.f;
	f.ReadCasted<byte>(type);
	f >> pos;
	f >> normal;
	f >> rot;
}
