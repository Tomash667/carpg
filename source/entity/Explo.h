#pragma once

//-----------------------------------------------------------------------------
struct Explo
{
	SceneNode* node;
	Entity<Unit> owner;
	Vec3 pos;
	float size, sizemax, dmg;
	Ability* ability;
	vector<Entity<Unit>> hitted;

	static const int MIN_SIZE = 36;

	bool Update(float dt, LevelArea& area);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
