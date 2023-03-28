#pragma once

//-----------------------------------------------------------------------------
struct DestroyedObject
{
	SceneNode* node;
	BaseUsable* base;
	Vec3 pos;
	float rot, timer;

	static const int MIN_SIZE = 24;

	DestroyedObject() : node(nullptr) {}
	SceneNode* CreateSceneNode();
	bool Update(float dt);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f);
};
