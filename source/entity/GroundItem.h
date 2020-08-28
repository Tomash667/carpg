#pragma once

//-----------------------------------------------------------------------------
struct GroundItem : public EntityType<GroundItem>
{
	const Item* item;
	uint count, team_count;
	Vec3 pos;
	Quat rot;

	static const int MIN_SIZE = 23;

	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
