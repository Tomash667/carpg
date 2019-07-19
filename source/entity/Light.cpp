#include "Pch.h"
#include "GameCore.h"
#include "Light.h"
#include "BitStreamFunc.h"

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
void Light::Write(BitStreamWriter& f) const
{
	f << pos;
	f << color;
	f << range;
}

//=================================================================================================
void Light::Read(BitStreamReader& f)
{
	f >> pos;
	f >> color;
	f >> range;
}
