#pragma once

//-----------------------------------------------------------------------------
struct Explo
{
	LevelArea* area;
	SceneNode* node;
	Entity<Unit> owner;
	Vec3 pos;
	float size, sizemax, dmg;
	Ability* ability;
	vector<Entity<Unit>> hitted;

	static const int MIN_SIZE = 21;

	bool Update(float dt, LevelArea& area);
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
