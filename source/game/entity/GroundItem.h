// przedmiot na ziemi
#pragma once

struct Item;

//-----------------------------------------------------------------------------
struct GroundItem
{
	VEC3 pos;
	float rot;
	const Item* item;
	uint count, team_count;
	int netid;

	static const int MIN_SIZE = 23;

	void Save(HANDLE file);
	void Load(HANDLE file);
};
