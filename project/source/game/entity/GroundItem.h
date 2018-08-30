#pragma once

//-----------------------------------------------------------------------------
struct GroundItem
{
	int netid;
	const Item* item;
	uint count, team_count;
	Vec3 pos;
	float rot;

	static const int MIN_SIZE = 23;
	static int netid_counter;

	void Save(FileWriter& f);
	void Load(FileReader& f);
};
