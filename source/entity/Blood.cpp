#include "Pch.h"
#include "Blood.h"

#include "BitStreamFunc.h"

//=================================================================================================
void Blood::Save(GameWriter& f) const
{
	f.Write<byte>(type);
	f << pos;
	f << normal;
	f << size;
	f << scale;
	f << rot;
}

//=================================================================================================
void Blood::Load(GameReader& f)
{
	type = (BLOOD)f.Read<byte>();
	f >> pos;
	f >> normal;
	f >> size;
	f >> scale;
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
