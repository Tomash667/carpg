#pragma once

//-----------------------------------------------------------------------------
#include "Light.h"

//-----------------------------------------------------------------------------
struct GameLight : public Light
{
	Vec3 startPos;
	Vec3 startColor;

	static const int MIN_SIZE = 28;

	void Save(GameWriter& f) const;
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);
};
