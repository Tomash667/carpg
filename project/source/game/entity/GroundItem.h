// przedmiot na ziemi
#pragma once

struct Item;

//-----------------------------------------------------------------------------
struct GroundItem
{
	int netid;
	const Item* item;
	uint count, team_count;
	Vec3 pos;
	float rot;

	static const int MIN_SIZE = 23;

	void Save(HANDLE file);
	void Load(HANDLE file);
};
