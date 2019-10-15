#pragma once

//-----------------------------------------------------------------------------
struct GroundItem : public EntityType<GroundItem>
{
	const Item* item;
	uint count, team_count;
	Vec3 pos;
	float rot;
	SceneNode* node;

	static const int MIN_SIZE = 23;

	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
