#pragma once

class ExploSystem
{
public:
	Explo* Add(LevelArea* area, Ability* ability, const Vec3& pos);
	void Update(float dt);
	void RecreateScene();
	void Reset();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void LoadOld(GameReader& f);

private:
	vector<Explo*> explos;
};
