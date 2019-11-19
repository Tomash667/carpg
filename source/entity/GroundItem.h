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

	void CreateNode(Scene* scene);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
