#pragma once

//-----------------------------------------------------------------------------
#include "BloodType.h"

//-----------------------------------------------------------------------------
struct Blood
{
	BLOOD type;
	Vec3 pos, normal;
	float size, scale, rot;
	array<Light*, 3> lights;

	static const int MIN_SIZE = 33;

	void Save(GameWriter& f) const;
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);
};
