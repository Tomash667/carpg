#pragma once

//-----------------------------------------------------------------------------
struct Portal
{
	Vec3 pos;
	float rot;
	int atLevel;
	int index;
	int targetLoc;
	Portal* nextPortal;

	static const int MIN_SIZE = 17;

	void Save(GameWriter& f);
	void Load(GameReader& f);

	Vec3 GetSpawnPos() const
	{
		return pos + Vec3(sin(rot) * 2, 0, cos(rot) * 2);
	}
};
