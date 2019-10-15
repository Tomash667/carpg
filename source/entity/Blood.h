#pragma once

//-----------------------------------------------------------------------------
#include "BloodType.h"

//-----------------------------------------------------------------------------
struct Blood
{
	BLOOD type;
	Vec3 pos, normal;
	float size, scale, rot;
	SceneNode* node;

	static const int MIN_SIZE = 33;

	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);
};
