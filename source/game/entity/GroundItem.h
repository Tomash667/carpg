// przedmiot na ziemi
#pragma once

//-----------------------------------------------------------------------------
struct GroundItem
{
	VEC3 pos;
	float rot;
	const Item* item;
	uint count, team_count;
	int netid;

	void Save(HANDLE file);
	void Load(HANDLE file);
};
