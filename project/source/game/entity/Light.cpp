#include "Pch.h"
#include "Base.h"
#include "Light.h"

//=================================================================================================
void Light::Save(FileWriter& f) const
{
	f << pos;
	f << color;
	f << range;
}

//=================================================================================================
void Light::Load(FileReader& f)
{
	f >> pos;
	f >> color;
	f >> range;
}

//=================================================================================================
void Light::Write(BitStream& stream) const
{
	stream.Write(pos);
	stream.Write(color);
	stream.Write(range);
}

//=================================================================================================
bool Light::Read(BitStream& stream)
{
	return stream.Read(pos)
		&& stream.Read(color)
		&& stream.Read(range);
}
