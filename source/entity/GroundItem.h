#pragma once

//-----------------------------------------------------------------------------
struct GroundItem : public EntityType<GroundItem>
{
	const Item* item;
	SceneNode* node;
	uint count, team_count;
	Vec3 pos;
	float rot;

	static const int MIN_SIZE = 23;

	SceneNode* CreateNode();
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
