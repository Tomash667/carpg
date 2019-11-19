#pragma once

//-----------------------------------------------------------------------------
struct Portal
{
	Vec3 pos;
	float rot;
	int at_level;
	int target; // portal wyjœciowy
	int target_loc; // docelowa lokacja
	Portal* next_portal;

	static const int MIN_SIZE = 17;

	void Save(FileWriter& f);
	void Load(FileReader& f);
	Vec3 GetSpawnPos() const { return pos + Vec3(sin(rot) * 2, 0, cos(rot) * 2); }
};
