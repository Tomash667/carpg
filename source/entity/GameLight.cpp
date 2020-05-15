#include "Pch.h"
#include "GameLight.h"

#include "BitStreamFunc.h"

//=================================================================================================
void GameLight::Save(FileWriter& f) const
{
	f << start_pos;
	f << start_color;
	f << range;
}

//=================================================================================================
void GameLight::Load(FileReader& f)
{
	f >> start_pos;
	f >> start_color;
	f >> range;
}

//=================================================================================================
void GameLight::Write(BitStreamWriter& f) const
{
	f << start_pos;
	f << start_color;
	f << range;
}

//=================================================================================================
void GameLight::Read(BitStreamReader& f)
{
	f >> start_pos;
	f >> start_color;
	f >> range;
}
