#include "Pch.h"
#include "GameLight.h"

#include "BitStreamFunc.h"

//=================================================================================================
void GameLight::Save(GameWriter& f) const
{
	f << startPos;
	f << startColor;
	f << range;
}

//=================================================================================================
void GameLight::Load(GameReader& f)
{
	f >> startPos;
	f >> startColor;
	f >> range;
}

//=================================================================================================
void GameLight::Write(BitStreamWriter& f) const
{
	f << startPos;
	f << startColor;
	f << range;
}

//=================================================================================================
void GameLight::Read(BitStreamReader& f)
{
	f >> startPos;
	f >> startColor;
	f >> range;
}
