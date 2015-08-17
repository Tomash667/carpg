#include "Pch.h"
#include "Base.h"
#include "Blood.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

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

//=================================================================================================
void Blood::Write(BitStream& s)
{
	s.WriteCasted<byte>(type);
	WriteStruct(s, pos);
	WriteStruct(s, normal);
	s.Write(rot);
}

//=================================================================================================
bool Blood::Read(BitStream& s)
{
	size = 1.f;
	return s.ReadCasted<byte>(type) &&
		ReadStruct(s, pos) &&
		ReadStruct(s, normal) &&
		s.Read(rot);
}
