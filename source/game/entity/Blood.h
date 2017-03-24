#pragma once

#include "BloodType.h"

//-----------------------------------------------------------------------------
// Krew na ziemi
struct Blood
{
	BLOOD type;
	VEC3 pos, normal;
	float size, rot;
	int lights;

	static const int MIN_SIZE = 29;

	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);
};
