#pragma once

//-----------------------------------------------------------------------------
#include "Light.h"

//-----------------------------------------------------------------------------
struct GameLight : public Light
{
	Vec3 start_pos;
	Vec3 start_color;

	static const int MIN_SIZE = 28;

	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);
};
